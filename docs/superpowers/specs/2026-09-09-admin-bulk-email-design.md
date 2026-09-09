# Admin (centre-independent) Bulk Email — Design

**Date:** 2026-09-09
**Status:** Draft for review
**Author:** Claude + vinay

## 1. Goal

Let **administrators** schedule bulk emails to applicants **by criteria across all
centres (or a chosen subset)** — with **no single-centre restriction**. The content
(subject + HTML body + sender identity) is composed once per blast and used once.
This is a **self-contained subsystem**: it deliberately does **not** reuse the
centre letter flow (`dh_letter`, `dh_send_letter`, merge fields, per-centre
From/Reply-to, S3 attachments). Those stay untouched.

## 2. Decisions (agreed)

| Topic | Decision |
|---|---|
| Content storage | **Inline, no library.** Subject/body/From live on the blast row; used once. No letter CRUD. |
| Sender identity | **From name + From email + Reply-to entered fresh every time** (required; no prefill, no global default setting). Stored per blast. |
| Targeting | **Centre multi-select added as one more criterion** on the shared form (blank = all centres) + existing search criteria; WHERE built **without** an `a_center='X'` pin. |
| Shared form | **Reuse the exact same `dh_manageapp_search_form`** (not a copy) so every future form improvement reaches admin bulk email automatically. |
| Code reuse | **Common logic factored into shared functions** used by BOTH centre bulk email and admin bulk email (recipient-query builder, criteria cleaner, enqueue helper). |
| Inline images | **Supported**, reusing the existing letters CID machinery (`_dh_letter_embed_inline_images`) + a CKEditor image-upload endpoint. |
| Attachments | **Multiple document attachments per blast** (child table + Mailgun multi-attachment), with a total-size guard. |
| Access | **New dedicated permission `admin mass mail`.** No new letter permission (no library). |
| Safety | **Recipient count + explicit confirm** before scheduling. **No** monthly cap in v1. |
| Test send | **Yes** — send one email to a typed address using the exact composed content, before scheduling. |
| Queue/consumer | **Same `mail` queue + a `type` field.** `dipi_mail_consumer_callback` branches; existing centre path unchanged. |
| Unsubscribe | **Global** unsubscribe link + suppression list; admin sends skip globally-opted-out addresses. |
| Personalisation | **None** (no merge fields) — same subject/body to every recipient. |

## 3. Why this shape (vs. reusing the centre flow)

- Never touches `dh_send_letter`/`dh_get_letter`, so the send-time `l_center` scoping
  (letters.inc:891), per-centre From (`dh_center_setting`), tokens, and S3
  attachments are all irrelevant. The fragile, heavily-used centre send path is not
  modified.
- One new table + one dedicated send function = easy to test, easy to roll back,
  cannot harm centre bulk mail.
- Matches "one letter, used once" — no template library to build or maintain.

## 4. Data model (new tables)

Schema is dump-managed (no `.install`/`hook_schema` in this project — see CLAUDE.md).
Tables are created by running the CREATE statements on local first, then prod at
deploy. Charset `utf8mb4`/`utf8mb4_general_ci` (project standard).

### 4.1 `dh_admin_bulk_mail`
One row per blast (draft → scheduled → sent).

| Column | Type | Notes |
|---|---|---|
| `abm_id` | INT PK AI | |
| `abm_name` | VARCHAR(255) | admin-facing batch name |
| `abm_subject` | VARCHAR(1024) | email subject |
| `abm_body` | LONGTEXT | HTML body (CKEditor) |
| `abm_from_name` | VARCHAR(255) | sender display name |
| `abm_from_email` | VARCHAR(255) | sender address (drives Mailgun domain) |
| `abm_reply_to` | VARCHAR(255) | reply-to (optional) |
| `abm_query` | LONGTEXT | built recipient SELECT (resolves distinct `a_email`) |
| `abm_criteria` | LONGTEXT | JSON snapshot of the criteria (redisplay/audit) |
| `abm_centres` | VARCHAR(1024) | chosen centre ids CSV, or empty = all (audit/redisplay) |
| `abm_count` | INT | recipient count at schedule time (matched) |
| `abm_sent` | INT default 0 | running tally: successfully sent |
| `abm_failed` | INT default 0 | running tally: send failures |
| `abm_skipped_unsub` | INT default 0 | running tally: skipped (globally unsubscribed) |
| `abm_skipped_invalid` | INT default 0 | running tally: skipped (invalid email) |
| `abm_schedule_date` | DATE | send date (cron picks `= CURDATE()`) |
| `abm_processed` | TINYINT | 0=draft, 1=pending, 2=in-progress, 3=done, 4=failed, 5=cancelled |
| `abm_test_email` | VARCHAR(255) | last test-send address (optional) |
| `abm_test_sent_at` | DATETIME NULL | last test send time (optional) |
| `abm_deleted` | TINYINT default 0 | soft delete |
| `abm_created`, `abm_updated` | DATETIME | |
| `abm_created_by`, `abm_updated_by` | INT | uids |

> Note on `abm_processed` codes: this mirrors `dh_bulk_mail.bm_processed` but adds
> `0=draft` (before the compose step is finalised) and `5=cancelled`. Cron only ever
> picks `abm_processed=1` (pending).

### 4.2 `dh_admin_bulk_mail_log` — **exceptions only** (scales for very large blasts)
Rather than one row per recipient (which would grow ≈ total emails ever sent), this
table holds a row **only for the "interesting" minority**: `failed`,
`skipped-unsub`, `skipped-invalid`. **Successful sends are not logged per-row** — they
are counted in the `dh_admin_bulk_mail` summary tallies (§4.1) and traceable via
Mailgun variables (below). For a 100k-recipient blast with a 2–5% problem rate this
writes ~2–5k rows instead of 100k.

Columns: `abml_id` PK, `abml_abm` (FK→abm_id), `abml_applicant` (a_id),
`abml_name`, `abml_email`, `abml_center` (recipient's a_center),
`abml_status` (`failed` / `skipped-unsub` / `skipped-invalid`),
`abml_error` (failure reason / Mailgun error, nullable), `abml_created`.
**Indexes:** `(abml_abm)`, `(abml_created)`.

**Retention:** an optional purge (in the admin cron) deletes rows older than N months
(default keep 6). Successful-delivery lookups for an individual use Mailgun (below),
which is the system of record for successes.

**Tracing successes without per-row logs:** every message carries Mailgun variables
`v:abm-id` and `v:app-id` (the letter send already attaches `v:app-id`). Mailgun's
own logs/dashboard can then look up any recipient by `abm-id` for its retention
window, and a future bounce/complaint webhook (out of v1) can log problems *at
webhook time* — carrying `abm-id` + email — without pre-writing a row per success.

### 4.3 `dh_admin_bulk_mail_attachment` (documents to attach)
One row per uploaded document on a blast (added/removed during compose).
`abma_id` PK, `abma_abm` (FK→abm_id), `abma_uri` (stored file URI,
`private://admin-bulk-mail/{abm_id}/…`), `abma_name` (original filename),
`abma_size` (bytes), `abma_created`. Index `(abma_abm)`.

> Inline **images** are NOT stored here — they live in `public://letters/images/`
> (shared with the letters editor) and are embedded as CID at send time by the
> existing `_dh_letter_embed_inline_images()`. This table is only for **document
> attachments** (PDF/DOC/etc.) sent as real Mailgun attachments.

### 4.4 `dh_admin_bulk_mail_unsubscribe` (global opt-out)
`abu_id` PK, `abu_email` VARCHAR(255) UNIQUE, `abu_created` DATETIME.
Global (no centre column) — an address here is suppressed from **all** admin blasts.

## 5. New settings (variables)

**None.** The From name / From email / Reply-to are **entered fresh on every blast**
(required fields on the compose form) and stored on the row — there is no global
default and no prefill. The existing Mailgun variables (`mailgun_key`,
`mailgun_primary_domain`, `mode_test`/`mode_test_emails`) are reused as-is.

## 6. Permission & routes

New permission **`admin mass mail`** in `dh_manageapp_permission()`.

All routes are **global** (no `%centre_id` loader), gated by `admin mass mail`:

| Route | Callback | Purpose |
|---|---|---|
| `admin-bulk-mail` | `dh_admin_bulk_mail_list` | schedule list (DataTables) |
| `admin-bulk-mail/new` | `drupal_get_form('dh_manageapp_search_form')` in **admin mode** | criteria/targeting (reuses search form) |
| `admin-bulk-mail/%abm_id/compose` | `dh_admin_bulk_mail_compose` | compose subject/body/From, test-send, schedule |
| `admin-bulk-mail/image-upload` | `dh_admin_bulk_mail_image_upload` | CKEditor inline-image upload (JSON) |
| `admin-bulk-mail/%abm_id/attach` | `dh_admin_bulk_mail_attach` | upload document attachment(s) |
| `admin-bulk-mail/%abm_id/attach-delete/%abma_id` | `dh_admin_bulk_mail_attach_delete` | remove one attachment |
| `admin-bulk-mail/%abm_id/delete` | `dh_admin_bulk_mail_delete` | soft delete / cancel |
| `admin-bulk-mail/%abm_id/show-log` | `dh_admin_bulk_mail_show_log` | log page |
| `admin-bulk-mail/%abm_id/get-log` | `dh_admin_bulk_mail_get_log` | SSP JSON for the log |
| `admin-bulk-mail/unsubscribe/%token` | `dh_admin_bulk_mail_unsubscribe` | global opt-out landing (encrypted email) |

`%abm_id` gets an `abm_id_load()` loader that also enforces `admin mass mail`.

All new code lives in a **new include** `inc/admin-bulk-mail.inc`, `include_once`'d in
`dh_manageapp.module` (per the module's convention). Routes registered in
`hook_menu()`.

## 7. Flow

### 7.1 Step 1 — Targeting (the SAME search form, in admin mode)
- Entry `admin-bulk-mail/new` renders the **exact same `dh_manageapp_search_form`**
  with a new `$admin_mode` flag (detected from the route). It is the same function —
  not a copy — so any future criterion added to the search form is automatically
  available to admin bulk email.
- **Centre multi-select is added as one more criterion** (`select2`), options = all
  centres. It appears in admin mode; **blank = all centres**. (For the ordinary
  single-centre routes the centre stays pinned as today.)
- WHERE building changes only in admin mode:
  - centres chosen → `a_center IN (id,id,…)`
  - none chosen → **no `a_center` clause at all** (true cross-centre).
  - i.e. the `$where = "a_center='$centre_id'"` seed (search.inc:1820) is skipped in
    admin mode; every other criterion clause is built by the **same** code path.
- The centre **Bulk-Mail** fieldset is replaced (in admin mode) by a single
  **"Compose bulk email for these N recipients"** action on the results view.
- Submit → results table (full-width) + recipient count. The compose action
  **creates a draft** `dh_admin_bulk_mail` row (`abm_query`, `abm_criteria`,
  `abm_centres`, `abm_count`, `abm_processed=0`) and redirects to the compose page.

> The admin-mode branch is **gated on `admin mass mail`** and is additive: the
> ordinary centre search and the existing centre Bulk-Mail section behave exactly as
> today when the flag is off.

### 7.2 Step 2 — Compose & schedule (`admin-bulk-mail/%abm_id/compose`)
Form fields: Name, Subject, Body (CKEditor, reusing the letters editor assets),
**From name / From email** (required, entered fresh each time — no prefill),
Reply-to (optional), Schedule date. Plus:
- **Inline images** in the body via the CKEditor image button → an admin image-upload
  endpoint that stores to `public://letters/images/` (same dir the existing
  `_dh_letter_embed_inline_images()` reads), so images are embedded as CID at send.
- **Document attachments** — a multi-file upload (add/remove individual files); each
  saved to `private://admin-bulk-mail/{abm_id}/` and recorded in
  `dh_admin_bulk_mail_attachment`. A **total-size guard** (validated on the form,
  e.g. ≤ ~20 MB combined) keeps the message under Mailgun's limit.
- **Send test email** button → reads current form values, calls
  `dh_send_admin_bulk_mail()` synchronously to a typed address, shows result. Does
  not require saving first (optionally saves draft).
- **Confirm & Schedule** → validation (subject, body, From email, date, count>0),
  shows **"About to email N recipients across M centres — Confirm"**, then sets
  From/subject/body/schedule on the row and `abm_processed=1` (pending).

Editing an existing pending blast reuses this page (content + schedule + From).
Changing the *criteria* means starting a new blast from Step 1 (v1 simplification).

### 7.3 Step 3 — Cron (`cron-admin-bulk-mail.php`, new root script)
Mirrors `cron-bulk-mail.php`: CLI guard + `flock`, bootstrap, then:
- Select due: `... where abm_processed=1 and DATE(abm_schedule_date)=CURDATE() and abm_deleted=0`.
- Mark `abm_processed=2` (in-progress).
- Run `abm_query` → for each recipient push
  `push_to_queue('Dipi','mail', json_encode(['type'=>'admin','abm_id'=>…,'app_id'=>…,'email'=>…,'name'=>…]), ttl)`.
- Push completion sentinel `['type'=>'admin','abm_id'=>…,'completed'=>1]`.
- No monthly cap in v1.

Add to the cron schedule (`scripts/cron-curl.sh`/crontab) at deploy.

### 7.4 Step 4 — Consumer (change flagged by user)
`dipi_mail_consumer_callback($msg)` (dh_manageapp.module:2994) gains a branch:
```
$data = json_decode($msg->body);
if (isset($data->type) && $data->type === 'admin') {
    // completion sentinel → UPDATE dh_admin_bulk_mail SET abm_processed=3 (done)
    // recipient →
    //   invalid a_email        → abm_skipped_invalid++ ; log row (skipped-invalid)
    //   in global unsub table   → abm_skipped_unsub++   ; log row (skipped-unsub)
    //   else send:
    //     footer = unsubscribe link (encrypted email → admin-bulk-mail/unsubscribe/{token})
    //     dh_send_admin_bulk_mail(email,name,subject,body,from_name,from_email,reply_to,footer)
    //     success → abm_sent++   (NO per-row log)
    //     failure → abm_failed++ ; log row (failed, with error)
    return;
}
// … existing centre path unchanged …
```
- Existing messages carry **no** `type`, so the centre path is byte-for-byte unchanged.
- **Ops:** the long-running `mail` worker must be **restarted after deploy** to load
  the new callback code.

### 7.5 Dedicated send function
`dh_send_admin_bulk_mail($to_email, $to_name, $subject, $body, $from_name, $from_email, $reply_to, $footer='', $is_test=false)`
in `inc/admin-bulk-mail.inc`:
- Thin Mailgun SDK wrapper: `from` = `"$from_name <$from_email>"`, `to`, `subject`,
  `html` = embedded body + `$footer`, `h:Reply-To` = `$reply_to`, plus tracking vars
  **`v:abm-id`** and **`v:app-id`** (for Mailgun-side tracing / future bounce webhook).
- **Inline images:** run the body through the existing
  `_dh_letter_embed_inline_images()` (letters.inc:940) → `$options['inline'][]`.
- **Attachments:** load the blast's `dh_admin_bulk_mail_attachment` rows and add each
  as `$options['attachment'][] = ['filePath'=>drupal_realpath($uri), 'filename'=>name]`.
- Reuse `get_mailgun_domain($from)` (letters.inc:1355) to pick the domain.
- Honour test-mode redirect (`variable_get('mode_test')` / `mode_test_emails`).
- No merge tokens, no S3, no per-centre settings.
- Returns `['result'=>bool,'res_id'=>…,'msg'=>…]`.

### 7.6 Unsubscribe
- Footer includes an **Unsubscribe** link with the recipient email encrypted
  (mirror the existing footer at dh_manageapp.module:3015-3019).
- `admin-bulk-mail/unsubscribe/%token` decrypts and inserts into
  `dh_admin_bulk_mail_unsubscribe` (idempotent), shows a confirmation page.
- Consumer skips any address already in that table (logs `skipped-unsub`).

## 8. Recipient resolution details
- De-dup by `a_email` (`group by a_email`, `select max(a_id)` as representative),
  same as the centre flow (bulk-mail.inc:7,19).
- `a_email` validated at send (`FILTER_VALIDATE_EMAIL`); invalid → logged
  `skipped-invalid`, not sent.
- Joins mirror the centre recipient query but with **no centre filter** unless
  `abm_centres` is set (`a_center IN (…)`).

## 8.1 Shared functions (used by BOTH centre and admin bulk email)

To avoid duplicating logic, common pieces move into a shared include
`inc/bulk-mail-common.inc` and are called by both `bulk-mail.inc` (centre) and
`admin-bulk-mail.inc` (admin). The **content model and send path stay separate**
(centre = `dh_letter` + `dh_send_letter`; admin = inline content +
`dh_send_admin_bulk_mail`) — only the recipient/criteria plumbing is shared.

Extracted (refactor of existing centre logic, behaviour preserved):
- `dh_bulk_mail_build_recipient_query($where)` → returns `[sql, count]`. The distinct
  `a_email` recipient SELECT + `count(distinct a_email)` (currently inline in
  `dh_create_edit_bulk_mail_schedule`, bulk-mail.inc:7-31). Parameterised purely by
  `$where` — centre passes a WHERE containing `a_center='X'`, admin passes one with
  `a_center IN (…)` or none.
- `dh_bulk_mail_clean_criteria($criteria)` → cleaned JSON snapshot (the implode /
  unset-internal-keys logic, bulk-mail.inc:41-60).
- `dh_bulk_mail_enqueue_recipients($query, $build_msg_callback)` → runs the recipient
  query and pushes one queue message per recipient (+ completion sentinel). The
  centre cron and the admin cron pass different `$build_msg_callback`s (the admin one
  adds `type:'admin'`).

Reused **as-is** from the letters subsystem (no change to letters code):
- `_dh_letter_embed_inline_images($html)` (letters.inc:940) — CID inline-image
  embedding, called by `dh_send_admin_bulk_mail`.
- The CKEditor image-upload handler logic (letters.inc `dh_letter_image_upload`) —
  an admin route reuses it (storing to the same `public://letters/images/`).

The **search WHERE builder** is shared automatically by reusing
`dh_manageapp_search_form` (§7.1). Centre bulk email is refactored to call the
extracted helpers so both paths share one implementation; the centre flow's observable
behaviour is unchanged and re-verified after the refactor.

## 9. Files touched / added

**Added**
- `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` — admin bulk-mail logic
  (list, compose, `dh_send_admin_bulk_mail`, cron helpers, unsubscribe, log SSP).
- `sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc` — shared helpers (§8.1).
- `cron-admin-bulk-mail.php` — cron entry point.
- SQL: `CREATE TABLE dh_admin_bulk_mail`, `dh_admin_bulk_mail_log`,
  `dh_admin_bulk_mail_attachment`, `dh_admin_bulk_mail_unsubscribe` (run local → prod).

**Modified (additively)**
- `dh_manageapp.module` — `include_once` the two new incs; `hook_menu()` routes +
  `abm_id_load()`; `hook_permission()` `admin mass mail`;
  **`dipi_mail_consumer_callback()`** admin branch. (No new settings.)
- `inc/search.inc` — `$admin_mode` in `dh_manageapp_search_form`: centre
  multi-select criterion, cross-centre WHERE (no `a_center` pin), compose action on
  results. All gated on `admin mass mail`; existing behaviour unchanged when off.
- `inc/bulk-mail.inc` — refactored to call the shared helpers in
  `bulk-mail-common.inc` (behaviour preserved; re-verified).

## 10. Implementation phases (each built + tested on LOCAL first; deploy only on go-ahead)

1. **Schema** — create the 4 tables on local.
2. **Shared helpers + routes/perm** — extract `bulk-mail-common.inc` from the centre
   flow (recipient-query builder, criteria cleaner, enqueue helper), re-verify centre
   bulk email is unchanged; add `admin mass mail` + route skeleton.
3. **Admin targeting** — `$admin_mode` in the search form: centre multi-select,
   cross-centre WHERE, count, "compose" action → draft row.
4. **Compose page** — form, validation, count-confirm, schedule (pending); inline
   images (CKEditor upload endpoint) + multi-document attachments (upload/remove,
   total-size guard).
5. **Send function + test-send** — `dh_send_admin_bulk_mail` (with CID inline images
   + attachments), test button.
6. **Cron + consumer branch + logging** — `cron-admin-bulk-mail.php`, admin branch
   in the consumer, `dh_admin_bulk_mail_log`.
7. **Global unsubscribe** — table, footer link, unsubscribe route, suppression.
8. **Schedule list + edit + delete + log pages.**

## 11. Ops / deploy notes
- Apply the 4 CREATE TABLE statements on prod at deploy (dump-managed schema).
- **Restart the `mail` queue consumer** after deploying the consumer change.
- Add `cron-admin-bulk-mail.php` to the cron schedule.
- Grant `admin mass mail` to the intended admin role only.
- Ensure the file dirs exist and are writable: `private://admin-bulk-mail/`
  (attachments) and `public://letters/images/` (inline images, shared with letters).

## 12. Risks & mitigations
- **Consumer change touches live sending.** Mitigation: additive `type` branch;
  no-`type` messages hit the unchanged path; test on local queue first; restart
  worker as a deliberate step.
- **Large blasts.** Mitigation: recipient-count confirm; queue rate-limits delivery;
  test-send before scheduling; (monthly cap deferred — can add later).
- **Attachments multiply bandwidth.** A document is re-sent with every recipient
  (N recipients × file size uploaded to Mailgun). Mitigation: total-size guard on the
  form; the compose UI can note that for very large audiences a link to a hosted
  document is lighter than an attachment. (Attachment I/O per send mirrors the centre
  flow's per-recipient S3 PDF read.)
- **Stored SQL in `abm_query`** follows the existing `bm_query` pattern (same
  injection surface as today; not made worse). Criteria come from an authenticated
  admin form.
- **Cross-centre From identity** — single global default avoids per-centre
  inconsistency; overridable per blast.

## 13. Out of scope (v1)
- Merge-field personalisation, saved letter/template library, per-centre From,
  SMS/WhatsApp, monthly cap, editing criteria of an existing blast
  in place (start a new blast instead). (Inline images and multi-document
  attachments ARE in scope — see §4.3, §7.2, §7.5.)
- Mailgun bounce/complaint **webhook ingestion** and open/click tracking (the
  `v:abm-id`/`v:app-id` variables are attached now so this can be added later).
