# Administrator Bulk Email — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let administrators schedule criteria-based bulk emails to applicants across all centres (or a chosen subset), with content composed once per blast, inline images + attachments, test-send, and scalable logging — as a self-contained subsystem that never touches the centre letter/send flow.

**Architecture:** New `dh_admin_bulk_mail*` tables + a dedicated Mailgun send function. Targeting reuses the existing `dh_manageapp_search_form` in an `admin mass mail`-gated `$admin_mode` (centre multi-select, cross-centre WHERE). A two-step flow (search → compose/test/schedule) writes a blast row; a new cron enqueues to the existing RabbitMQ `mail` queue with a `type:'admin'` discriminator; the existing consumer callback gains an additive admin branch. Common recipient/criteria logic is factored into a shared include used by both centre and admin flows.

**Tech Stack:** Drupal 7.89 (procedural PHP), MySQL (bare `db_query`), Mailgun PHP SDK, RabbitMQ (php-amqplib), CKEditor. PHP 7.4 locally, 7.3 on prod.

**Spec:** `docs/superpowers/specs/2026-09-09-admin-bulk-email-design.md` (read it alongside this plan)

## Global Constraints

- **Local-first, no push.** Build and verify every task on `http://dipi.localhost` (PHP 7.4). NEVER push/deploy to prod until the user gives an explicit go-ahead. All commits in this plan are **local only**.
- **No automated test suite exists** for this custom code. "Test" = `php7.4 -l` lint, cache flush, functional check on dipi.localhost (browser/DB), and headless-chrome screenshots. Each task ends with a concrete local verification.
- **Style:** match surrounding legacy code — procedural functions, direct `db_query()` with **bare** table names (no `{curly}` wrapper), inline HTML building. Do not introduce new frameworks/patterns.
- **DB standard:** `utf8mb4` / `utf8mb4_general_ci` for all new tables. No `.install`/`hook_schema` — schema is applied via raw SQL (dump-managed).
- **Cache:** after editing `.module`/`hook_menu`/`hook_permission`, flush caches (`admin/config/development/performance`, or the scratchpad `flush.php`, or `drush cc all`) so routes/menu register.
- **CWD assumption:** `vendor/autoload.php` is loaded with a relative path; root cron scripts must run from the Drupal root.
- **Never write test data to prod.** Use `mode_test`/`mode_test_emails` for any real send test.

## File Structure

**Create**
- `sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc` — shared helpers (recipient query, criteria cleaner, enqueue).
- `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` — all admin bulk-mail logic (targeting draft, compose, send fn, consumer helper, unsubscribe, list/log, image-upload, attachments).
- `cron-admin-bulk-mail.php` — cron entry point (repo root).

**Modify (additively)**
- `sites/all/modules/dh_manageapp/dh_manageapp.module` — `include_once` the two new incs; `hook_permission()` add `admin mass mail`; `hook_menu()` routes + `abm_id_load()`; `dipi_mail_consumer_callback()` admin branch.
- `sites/all/modules/dh_manageapp/inc/search.inc` — `$admin_mode` in `dh_manageapp_search_form` (centre multi-select, cross-centre WHERE, compose action).
- `sites/all/modules/dh_manageapp/inc/bulk-mail.inc` — refactor `dh_create_edit_bulk_mail_schedule` to call the shared helpers (behaviour preserved).
- `cron-bulk-mail.php` — refactor the enqueue loop to the shared helper (behaviour preserved).
- `sites/all/modules/dh_manageapp/css/manageapp.css` — minor styling for the compose/list pages (scoped).

**SQL** (run on local via mysql, then prod at deploy): 4 CREATE TABLEs (Task 1).

---

### Task 1: Schema — create the 4 tables

**Files:**
- Create: `docs/superpowers/plans/sql/admin-bulk-mail.sql` (keep the DDL in the repo for the deploy step)

**Interfaces:**
- Produces: tables `dh_admin_bulk_mail`, `dh_admin_bulk_mail_log`, `dh_admin_bulk_mail_attachment`, `dh_admin_bulk_mail_unsubscribe`.

- [ ] **Step 1: Write the DDL file**

```sql
-- docs/superpowers/plans/sql/admin-bulk-mail.sql
CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail (
  abm_id INT AUTO_INCREMENT PRIMARY KEY,
  abm_name VARCHAR(255) NOT NULL DEFAULT '',
  abm_subject VARCHAR(1024) NOT NULL DEFAULT '',
  abm_body LONGTEXT,
  abm_from_name VARCHAR(255) NOT NULL DEFAULT '',
  abm_from_email VARCHAR(255) NOT NULL DEFAULT '',
  abm_reply_to VARCHAR(255) NOT NULL DEFAULT '',
  abm_query LONGTEXT,
  abm_criteria LONGTEXT,
  abm_centres VARCHAR(1024) NOT NULL DEFAULT '',
  abm_count INT NOT NULL DEFAULT 0,
  abm_sent INT NOT NULL DEFAULT 0,
  abm_failed INT NOT NULL DEFAULT 0,
  abm_skipped_unsub INT NOT NULL DEFAULT 0,
  abm_skipped_invalid INT NOT NULL DEFAULT 0,
  abm_schedule_date DATE DEFAULT NULL,
  abm_processed TINYINT NOT NULL DEFAULT 0,
  abm_test_email VARCHAR(255) NOT NULL DEFAULT '',
  abm_test_sent_at DATETIME DEFAULT NULL,
  abm_deleted TINYINT NOT NULL DEFAULT 0,
  abm_created DATETIME DEFAULT NULL,
  abm_updated DATETIME DEFAULT NULL,
  abm_created_by INT DEFAULT NULL,
  abm_updated_by INT DEFAULT NULL,
  KEY idx_processed_date (abm_processed, abm_schedule_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail_log (
  abml_id INT AUTO_INCREMENT PRIMARY KEY,
  abml_abm INT NOT NULL,
  abml_applicant INT DEFAULT NULL,
  abml_name VARCHAR(255) NOT NULL DEFAULT '',
  abml_email VARCHAR(255) NOT NULL DEFAULT '',
  abml_center INT DEFAULT NULL,
  abml_status VARCHAR(32) NOT NULL DEFAULT '',
  abml_error VARCHAR(1024) NOT NULL DEFAULT '',
  abml_created DATETIME DEFAULT NULL,
  KEY idx_abm (abml_abm),
  KEY idx_created (abml_created)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail_attachment (
  abma_id INT AUTO_INCREMENT PRIMARY KEY,
  abma_abm INT NOT NULL,
  abma_uri VARCHAR(1024) NOT NULL DEFAULT '',
  abma_name VARCHAR(512) NOT NULL DEFAULT '',
  abma_size INT NOT NULL DEFAULT 0,
  abma_created DATETIME DEFAULT NULL,
  KEY idx_abm (abma_abm)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail_unsubscribe (
  abu_id INT AUTO_INCREMENT PRIMARY KEY,
  abu_email VARCHAR(255) NOT NULL,
  abu_created DATETIME DEFAULT NULL,
  UNIQUE KEY uniq_email (abu_email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
```

- [ ] **Step 2: Apply on local** — run against the local DIPI DB (get creds from `sites/default/settings.php` `$databases`):

```bash
mysql -u <db_user> -p<db_pass> <db_name> < docs/superpowers/plans/sql/admin-bulk-mail.sql
```

- [ ] **Step 3: Verify** — all 4 tables exist with utf8mb4:

```bash
mysql -u <db_user> -p<db_pass> <db_name> -e "SHOW TABLES LIKE 'dh_admin_bulk_mail%'; SHOW CREATE TABLE dh_admin_bulk_mail\G" | grep -i "dh_admin_bulk_mail\|utf8mb4"
```
Expected: 4 tables listed; charset `utf8mb4`.

- [ ] **Step 4: Commit (local only)**

```bash
git add docs/superpowers/plans/sql/admin-bulk-mail.sql
git commit -m "Admin bulk email: schema DDL (4 tables)"
```

---

### Task 2: Shared helpers + refactor centre flow

**Files:**
- Create: `sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc`
- Modify: `sites/all/modules/dh_manageapp/dh_manageapp.module` (add `include_once`)
- Modify: `sites/all/modules/dh_manageapp/inc/bulk-mail.inc:4-77` (`dh_create_edit_bulk_mail_schedule`)
- Modify: `cron-bulk-mail.php:48-56` (enqueue loop)

**Interfaces:**
- Produces:
  - `dh_bulk_mail_build_recipient_query($where)` → `array('query'=>string, 'count'=>int)`
  - `dh_bulk_mail_clean_criteria(array $criteria)` → JSON string
  - `dh_bulk_mail_enqueue($query, callable $build_msg, array $done_msg)` — `$build_msg($applicant_id)` returns the per-recipient message array.

- [ ] **Step 1: Create the shared include**

```php
<?php
// sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc
// Shared recipient/criteria/enqueue helpers used by BOTH centre bulk email
// (bulk-mail.inc) and admin bulk email (admin-bulk-mail.inc).

/**
 * Build the recipient SELECT (one representative a_id per distinct a_email) and
 * the recipient count for a given WHERE fragment. The WHERE decides scope:
 * centre passes "a_center='X' and ...", admin passes "a_center in (...)"/"1=1".
 */
function dh_bulk_mail_build_recipient_query($where) {
  $joins = "
    dh_applicant
    left join dh_applicant_course ac on (a_id=ac_applicant)
    left join dh_applicant_attended aa on (a_id=aa_applicant)
    left join dh_applicant_extra ae on (a_id=ae_applicant)
    left join dh_course c on (a_course=c.c_id)
    left join dh_country co on (a_country=co.c_code)
    left join dh_city ci on a_city=ci.c_id
    left join dh_state s on (a_state=s.s_code and a_country=s.s_country)
    left join dh_center ce on (a_center=ce.c_id)";
  $query = "select max(a_id) as id from $joins where $where group by a_email";
  $count = db_query("select count(distinct a_email) as id from $joins where $where")->fetchField();
  return array('query' => $query, 'count' => (int) $count);
}

/**
 * Clean raw search form values into a JSON-ready criteria snapshot:
 * implode multi-selects, drop internal keys, drop empties.
 */
function dh_bulk_mail_clean_criteria($criteria) {
  foreach (array('status', 'lang_discourse', 'state', 'country') as $k) {
    if (isset($criteria[$k]) && is_array($criteria[$k])) {
      $criteria[$k] = implode(', ', $criteria[$k]);
    }
  }
  unset(
    $criteria['form_build_id'], $criteria['form_token'], $criteria['form_id'],
    $criteria['bulk-mail'], $criteria['db_type'], $criteria['op'],
    $criteria['bulk_mail_name'], $criteria['letters'],
    $criteria['bulk-mail-schedule'], $criteria['bm_id']
  );
  foreach ($criteria as $key => $value) {
    if (!$criteria[$key]) {
      unset($criteria[$key]);
    }
  }
  return json_encode($criteria);
}

/**
 * Run a recipient query and push one 'mail' queue message per recipient, then a
 * completion sentinel. $build_msg($applicant_id) returns the per-recipient array.
 */
function dh_bulk_mail_enqueue($query, callable $build_msg, array $done_msg) {
  $res = db_query($query);
  while ($r = $res->fetchAssoc()) {
    push_to_queue('Dipi', 'mail', json_encode($build_msg($r['id'])), 86400000);
  }
  push_to_queue('Dipi', 'mail', json_encode($done_msg), 86400000);
}
```

- [ ] **Step 2: Include it** — in `dh_manageapp.module`, next to the other `include_once` lines at the top, add:

```php
include_once 'inc/bulk-mail-common.inc';
```

- [ ] **Step 3: Refactor `dh_create_edit_bulk_mail_schedule`** (bulk-mail.inc:4-77) to use the helpers. Replace the recipient-query/count block (lines 7-35) and the criteria-clean block (lines 41-60) so the function body becomes:

```php
function dh_create_edit_bulk_mail_schedule($bulk_mail_name, $where, $letter, $schedule_date, $criteria, $bm_id=0)
{
  global $user;
  $rq = dh_bulk_mail_build_recipient_query($where);

  $bm['bm_name'] = $bulk_mail_name;
  $bm['bm_center'] = arg(1);
  $bm['bm_count'] = $rq['count'];
  $bm['bm_query'] = $rq['query'];
  $bm['bm_letter'] = $letter;
  $bm['bm_schedule_date'] = date('Y-m-d H:i:s', strtotime($schedule_date));
  $bm['bm_criteria'] = dh_bulk_mail_clean_criteria($criteria);

  if ($bm_id) {
    $bm['bm_updated'] = date('Y-m-d H:i:s');
    $bm['bm_updated_by'] = $user->uid;
    db_update('dh_bulk_mail')->fields($bm)->condition('bm_id', $bm_id)->execute();
  }
  else {
    $bm['bm_created_by'] = $user->uid;
    $bm['bm_updated_by'] = $user->uid;
    db_insert('dh_bulk_mail')->fields($bm)->execute();
  }
}
```

- [ ] **Step 4: Refactor the centre cron enqueue loop** (cron-bulk-mail.php:48-56) to use the helper:

```php
     db_query($q);
     $bm_id = $row['bm_id'];
     $bm_letter = $row['bm_letter'];
     dh_bulk_mail_enqueue(
       $row['bm_query'],
       function ($app_id) use ($bm_id, $bm_letter) {
         return array('bulk_mail_id' => $bm_id, 'applicant_id' => $app_id, 'letter_id' => $bm_letter, 'completed' => 0);
       },
       array('bulk_mail_id' => $bm_id, 'completed' => 1)
     );
```

- [ ] **Step 5: Lint**

```bash
php7.4 -l sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc && \
php7.4 -l sites/all/modules/dh_manageapp/inc/bulk-mail.inc && \
php7.4 -l cron-bulk-mail.php
```
Expected: "No syntax errors detected" for all three.

- [ ] **Step 6: Verify centre flow unchanged** — flush cache; on dipi.localhost create a **centre** bulk-mail schedule (search-app/{centre} → tick Bulk-Mail → letter + name → schedule); confirm a row is inserted with a non-empty `bm_query` and correct `bm_count`:

```bash
mysql -u <u> -p<p> <db> -e "select bm_id,bm_center,bm_count,left(bm_query,40) q,bm_criteria from dh_bulk_mail order by bm_id desc limit 1"
```
Expected: newest row has sane `bm_count`, a `bm_query` starting `select max(a_id)`, and JSON criteria — identical shape to before the refactor.

- [ ] **Step 7: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc sites/all/modules/dh_manageapp/dh_manageapp.module sites/all/modules/dh_manageapp/inc/bulk-mail.inc cron-bulk-mail.php
git commit -m "Bulk email: extract shared recipient/criteria/enqueue helpers; refactor centre flow"
```

---

### Task 3: Permission, include, route skeleton, loaders

**Files:**
- Create: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (stub callbacks)
- Modify: `sites/all/modules/dh_manageapp/dh_manageapp.module` (`include_once`, `hook_permission`, `hook_menu`, `abm_id_load`)

**Interfaces:**
- Produces: permission `admin mass mail`; routes `admin-bulk-mail`, `admin-bulk-mail/new`, `admin-bulk-mail/%abm_id/compose`, `admin-bulk-mail/image-upload`, `admin-bulk-mail/%abm_id/attach`, `admin-bulk-mail/%abm_id/attach-delete/%abma_id`, `admin-bulk-mail/%abm_id/delete`, `admin-bulk-mail/%abm_id/show-log`, `admin-bulk-mail/%abm_id/get-log`, and public `abm-unsubscribe/%`; loader `abm_id_load($id)`.

> **Correction to spec §6:** the unsubscribe landing must be **public** (recipients click it from an email, unauthenticated). It is therefore a separate top-level route `abm-unsubscribe/%` with `access callback => TRUE`, NOT under the `admin mass mail`-gated `admin-bulk-mail/*` prefix.

- [ ] **Step 1: Add the permission** — in `dh_manageapp_permission()` (dh_manageapp.module, near the existing `$perms['mass mail']` at ~line 76):

```php
  $perms['admin mass mail'] = array('title' => t('Send administrator (cross-centre) bulk emails'));
```

- [ ] **Step 2: Create the include with stub callbacks**

```php
<?php
// sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
// Administrator (centre-independent) bulk email — self-contained subsystem.

function dh_admin_bulk_mail_list() { return 'TODO list'; }
function dh_admin_bulk_mail_compose($abm_id) { return 'TODO compose '.$abm_id; }
function dh_admin_bulk_mail_image_upload() { drupal_json_output(array()); }
function dh_admin_bulk_mail_attach($abm_id) { return 'TODO attach '.$abm_id; }
function dh_admin_bulk_mail_attach_delete($abm_id, $abma_id) { return 'TODO detach'; }
function dh_admin_bulk_mail_delete($abm_id) { return 'TODO delete '.$abm_id; }
function dh_admin_bulk_mail_show_log($abm_id) { return 'TODO log '.$abm_id; }
function dh_admin_bulk_mail_get_log($abm_id) { drupal_json_output(array()); }
function dh_admin_bulk_mail_unsubscribe($token) { return 'TODO unsub'; }
```

- [ ] **Step 3: Include it** — in `dh_manageapp.module` top includes:

```php
include_once 'inc/admin-bulk-mail.inc';
```

- [ ] **Step 4: Add `abm_id_load`** — near the other `*_id_load` loaders (dh_manageapp.module ~940):

```php
function abm_id_load($id) {
  if (!user_access('admin mass mail')) { return FALSE; }
  $row = db_query("select abm_id from dh_admin_bulk_mail where abm_id=:id and abm_deleted=0", array(':id' => $id))->fetchField();
  return $row ? $id : FALSE;
}
```

- [ ] **Step 5: Register routes** — in `hook_menu()` (dh_manageapp.module):

```php
  $items['admin-bulk-mail'] = array(
    'title' => 'Admin Bulk Email',
    'page callback' => 'dh_admin_bulk_mail_list',
    'access arguments' => array('admin mass mail'),
    'type' => MENU_NORMAL_ITEM,
  );
  $items['admin-bulk-mail/new'] = array(
    'title' => 'New Admin Bulk Email',
    'page callback' => 'drupal_get_form',
    'page arguments' => array('dh_manageapp_search_form'),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/image-upload'] = array(
    'page callback' => 'dh_admin_bulk_mail_image_upload',
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/%abm_id/compose'] = array(
    'page callback' => 'dh_admin_bulk_mail_compose',
    'page arguments' => array(1),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/%abm_id/attach'] = array(
    'page callback' => 'dh_admin_bulk_mail_attach',
    'page arguments' => array(1),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/%abm_id/attach-delete/%'] = array(
    'page callback' => 'dh_admin_bulk_mail_attach_delete',
    'page arguments' => array(1, 3),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/%abm_id/delete'] = array(
    'page callback' => 'dh_admin_bulk_mail_delete',
    'page arguments' => array(1),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/%abm_id/show-log'] = array(
    'page callback' => 'dh_admin_bulk_mail_show_log',
    'page arguments' => array(1),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['admin-bulk-mail/%abm_id/get-log'] = array(
    'page callback' => 'dh_admin_bulk_mail_get_log',
    'page arguments' => array(1),
    'access arguments' => array('admin mass mail'),
    'type' => MENU_CALLBACK,
  );
  $items['abm-unsubscribe/%'] = array(
    'page callback' => 'dh_admin_bulk_mail_unsubscribe',
    'page arguments' => array(1),
    'access callback' => TRUE,      // public: recipients click from email
    'type' => MENU_CALLBACK,
  );
```

- [ ] **Step 6: Lint + flush + verify routes resolve**

```bash
php7.4 -l sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc && \
php7.4 -l sites/all/modules/dh_manageapp/dh_manageapp.module && \
php7.4 /tmp/claude-1000/-dhamma-web-dipinew/aebfb087-cb04-43ca-aec5-82b00a10b4de/scratchpad/flush.php
```
Then load `http://dipi.localhost/admin-bulk-mail` as admin (uid 1) → shows "TODO list" (not 404/403). Confirm `admin mass mail` appears at `admin/people/permissions` and grant it to the admin role.

- [ ] **Step 7: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc sites/all/modules/dh_manageapp/dh_manageapp.module
git commit -m "Admin bulk email: permission, include, routes skeleton, abm_id_load"
```

---

### Task 4: Admin targeting via the shared search form (`$admin_mode`) + draft creation

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/search.inc` (`dh_manageapp_search_form`: `$centre_id` block ~1750, WHERE seed ~1817-1820, centre dropdown ~1997-2007, bulk-mail gate ~2110, submit ~1968)
- Modify: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (add `dh_admin_bulk_mail_create_draft`)

**Interfaces:**
- Consumes: `dh_bulk_mail_build_recipient_query()` (Task 2).
- Produces: `dh_admin_bulk_mail_create_draft($where, $criteria, $centres_csv)` → `abm_id`; a draft row (`abm_processed=0`).

- [ ] **Step 1: Detect admin mode** — at the top of `dh_manageapp_search_form` (after `$centre_id` is set, ~search.inc:1752) add:

```php
  $admin_mode = (arg(0) == 'admin-bulk-mail') && user_access('admin mass mail');
```

- [ ] **Step 2: Cross-centre WHERE seed** — replace the seed at search.inc:1817-1820:

```php
    if ($admin_mode) {
      $sel = isset($storage['abm_centres']) && is_array($storage['abm_centres']) ? array_filter($storage['abm_centres']) : array();
      if (!empty($sel)) {
        $where = "a_center in (" . implode(',', array_map('intval', $sel)) . ")";
      }
      else {
        $where = "1=1"; // all centres
      }
    }
    else {
      if ($centre_id == '') { $centre_id = $storage['centre']; }
      $where = "a_center='" . $centre_id . "'";
    }
```

- [ ] **Step 3: Centre multi-select criterion (admin mode only)** — in the form-build branch, add near the Course field (~search.inc:2039):

```php
    if ($admin_mode) {
      $all_centres = db_query("select c_id, c_name from dh_center where c_id<>0 order by c_name")->fetchAllKeyed();
      $form['abm_centres'] = array(
        '#title' => 'Centres', '#type' => 'select', '#multiple' => 'multiple',
        '#options' => $all_centres, '#weight' => 0, '#validated' => TRUE,
        '#attributes' => array('id' => 'edit-abm-centres'),
        '#description' => 'Leave blank to target ALL centres.',
      );
    }
```
(Init select2 for `#edit-abm-centres` in the inline JS block alongside the other select2 inits.)

- [ ] **Step 4: Compose action instead of centre Bulk-Mail section** — the centre Bulk-Mail fieldset is gated `if (user_access('mass mail') && $centre_id <> '')` (search.inc:2110). Leave that unchanged; it will not render in admin mode (no `$centre_id`). In the **results** output (search.inc submit branch, where `$out` is built ~1958), add for admin mode a button that posts the current criteria to create a draft:

```php
    if ($admin_mode) {
      $abm_id = dh_admin_bulk_mail_create_draft($where, $storage, implode(',', isset($storage['abm_centres']) ? array_map('intval', array_filter((array)$storage['abm_centres'])) : array()));
      $out = '<h2>' . l('Back to Admin Bulk Email', 'admin-bulk-mail') . '</h2>'
           . '<p><a class="btn btn-primary" href="/admin-bulk-mail/' . $abm_id . '/compose">Compose bulk email for these ' . db_query("select abm_count from dh_admin_bulk_mail where abm_id=:id", array(':id'=>$abm_id))->fetchField() . ' recipients</a></p>';
      $out .= dh_manageapp_search_results($db_type, $where, $centre_id);
      $form['out'] = array('#markup' => $out, '#weight' => 100);
      return $form;
    }
```
> Place this **before** the existing `if ($storage['bulk-mail'])` centre block so admin mode short-circuits cleanly.

- [ ] **Step 5: Draft creator** — in admin-bulk-mail.inc:

```php
function dh_admin_bulk_mail_create_draft($where, $criteria, $centres_csv) {
  global $user;
  $rq = dh_bulk_mail_build_recipient_query($where);
  $now = date('Y-m-d H:i:s');
  return db_insert('dh_admin_bulk_mail')->fields(array(
    'abm_query' => $rq['query'],
    'abm_count' => $rq['count'],
    'abm_criteria' => dh_bulk_mail_clean_criteria($criteria),
    'abm_centres' => $centres_csv,
    'abm_processed' => 0,   // draft
    'abm_created' => $now,
    'abm_updated' => $now,
    'abm_created_by' => $user->uid,
    'abm_updated_by' => $user->uid,
  ))->execute();
}
```

- [ ] **Step 6: Lint + flush**

```bash
php7.4 -l sites/all/modules/dh_manageapp/inc/search.inc && \
php7.4 -l sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc && \
php7.4 /tmp/claude-1000/-dhamma-web-dipinew/aebfb087-cb04-43ca-aec5-82b00a10b4de/scratchpad/flush.php
```

- [ ] **Step 7: Verify targeting** — on dipi.localhost load `admin-bulk-mail/new`: confirm the **Centres** multi-select shows; run a search with (a) no centre → results across centres, (b) two centres → only those. After submitting, a draft row is created:

```bash
mysql -u <u> -p<p> <db> -e "select abm_id,abm_count,abm_centres,abm_processed from dh_admin_bulk_mail order by abm_id desc limit 1"
```
Expected: `abm_processed=0`, `abm_count` matches the results count, `abm_centres` reflects the picked centres (or empty for all). Also re-check the **centre** search (`search-app/{centre}`) still pins one centre (unchanged).

- [ ] **Step 8: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/search.inc sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
git commit -m "Admin bulk email: admin-mode targeting (centre multi-select, cross-centre WHERE) + draft creation"
```

---

### Task 5: Compose page — content, From, schedule, count-confirm

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (`dh_admin_bulk_mail_compose` → render `drupal_get_form('dh_admin_bulk_mail_compose_form', $abm_id)`; add the form + validate + submit)
- Modify: `sites/all/modules/dh_manageapp/css/manageapp.css` (minor compose styling, scoped to `#dh-admin-bulk-mail-compose-form`)

**Interfaces:**
- Consumes: draft row from Task 4.
- Produces: `dh_admin_bulk_mail_compose_form`, `_validate`, `_submit`; on schedule sets `abm_processed=1` (pending) with subject/body/from/schedule.

- [ ] **Step 1: Compose form** — reuse the letters CKEditor. Model the CKEditor attach on `dh_letters_form` in `letters.inc` (same `#format`/editor assets). Fields:

```php
function dh_admin_bulk_mail_compose($abm_id) {
  return drupal_get_form('dh_admin_bulk_mail_compose_form', $abm_id);
}

function dh_admin_bulk_mail_compose_form($form, &$form_state, $abm_id) {
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id'=>$abm_id))->fetchAssoc();
  $form_state['abm_id'] = $abm_id;
  $form['info'] = array('#markup' => '<p>This blast targets <b>'.$abm['abm_count'].'</b> recipients'.($abm['abm_centres'] ? ' in centres '.check_plain($abm['abm_centres']) : ' across ALL centres').'.</p>');
  $form['abm_name'] = array('#title'=>'Batch name', '#type'=>'textfield', '#required'=>TRUE, '#default_value'=>$abm['abm_name']);
  $form['abm_from_name'] = array('#title'=>'From name', '#type'=>'textfield', '#required'=>TRUE, '#default_value'=>$abm['abm_from_name']);
  $form['abm_from_email'] = array('#title'=>'From email', '#type'=>'textfield', '#required'=>TRUE, '#default_value'=>$abm['abm_from_email']);
  $form['abm_reply_to'] = array('#title'=>'Reply-to (optional)', '#type'=>'textfield', '#default_value'=>$abm['abm_reply_to']);
  $form['abm_subject'] = array('#title'=>'Subject', '#type'=>'textfield', '#required'=>TRUE, '#default_value'=>$abm['abm_subject']);
  $form['abm_body'] = array('#title'=>'Body', '#type'=>'text_format', '#format'=>'full_html', '#required'=>TRUE, '#default_value'=>$abm['abm_body']);
  $form['abm_schedule_date'] = array('#title'=>'Send date', '#type'=>'date_popup', '#date_format'=>'Y-m-d', '#default_value'=>$abm['abm_schedule_date'] ?: date('Y-m-d'));
  // test send
  $form['test_email'] = array('#title'=>'Send a test to', '#type'=>'textfield', '#default_value'=>$abm['abm_test_email']);
  $form['test'] = array('#type'=>'submit', '#value'=>'Send test email', '#submit'=>array('dh_admin_bulk_mail_test_submit'), '#limit_validation_errors'=>array());
  $form['schedule'] = array('#type'=>'submit', '#value'=>'Confirm & Schedule');
  $form['#attributes']['onsubmit'] = "if(this.clk&&this.clk.value=='Confirm & Schedule'){return confirm('Schedule this email to ".$abm['abm_count']." recipients?');}return true;";
  return $form;
}
```
> The count-confirm is a JS `confirm()` on the Schedule button; keep the test button exempt.

- [ ] **Step 2: Validate** — require valid From email + subject + body + future/today date:

```php
function dh_admin_bulk_mail_compose_form_validate($form, &$form_state) {
  $v = $form_state['values'];
  if ($form_state['triggering_element']['#value'] == 'Send test email') { return; }
  if (!filter_var($v['abm_from_email'], FILTER_VALIDATE_EMAIL)) { form_set_error('abm_from_email', 'Valid From email required.'); }
  if (trim(strip_tags($v['abm_body']['value'])) == '') { form_set_error('abm_body', 'Body is required.'); }
}
```

- [ ] **Step 3: Submit (schedule)** — save content + set pending:

```php
function dh_admin_bulk_mail_compose_form_submit($form, &$form_state) {
  global $user;
  $v = $form_state['values'];
  db_update('dh_admin_bulk_mail')->fields(array(
    'abm_name'=>$v['abm_name'], 'abm_subject'=>$v['abm_subject'],
    'abm_body'=>$v['abm_body']['value'], 'abm_from_name'=>$v['abm_from_name'],
    'abm_from_email'=>$v['abm_from_email'], 'abm_reply_to'=>$v['abm_reply_to'],
    'abm_schedule_date'=>$v['abm_schedule_date'], 'abm_processed'=>1,
    'abm_updated'=>date('Y-m-d H:i:s'), 'abm_updated_by'=>$user->uid,
  ))->condition('abm_id', $form_state['abm_id'])->execute();
  drupal_set_message('Bulk email scheduled.');
  $form_state['redirect'] = 'admin-bulk-mail';
}
```
> `dh_admin_bulk_mail_test_submit` is implemented in Task 7 (stub it as an empty function for now so the button exists).

- [ ] **Step 4: Lint + flush**, then on dipi.localhost go through Task 4 to reach a draft, open `admin-bulk-mail/{abm_id}/compose`, fill fields, click **Confirm & Schedule** (accept the confirm), and verify:

```bash
mysql -u <u> -p<p> <db> -e "select abm_id,abm_processed,abm_from_email,abm_subject,abm_schedule_date from dh_admin_bulk_mail order by abm_id desc limit 1"
```
Expected: `abm_processed=1`, subject/from/date saved.

- [ ] **Step 5: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc sites/all/modules/dh_manageapp/css/manageapp.css
git commit -m "Admin bulk email: compose page (content, From, schedule, count-confirm)"
```

---

### Task 6: Inline images + document attachments

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (`dh_admin_bulk_mail_image_upload`, `_attach`, `_attach_delete`; add attachment list to the compose form)

**Interfaces:**
- Consumes: `dh_admin_bulk_mail_attachment` table; `dh_letter_image_upload` logic (letters.inc), `_dh_letter_embed_inline_images` (letters.inc:940, used at send in Task 7).
- Produces: uploaded images in `public://letters/images/`; attachment rows in `dh_admin_bulk_mail_attachment`.

- [ ] **Step 1: Image-upload endpoint** — mirror `dh_letter_image_upload` (letters.inc; find it via `grep -n 'function dh_letter_image_upload' sites/all/modules/dh_manageapp/inc/letters.inc`). It must save the uploaded file to `public://letters/images/` and return the CKEditor JSON `{"uploaded":1,"fileName":..,"url":..}`. Reuse the exact same save logic so `_dh_letter_embed_inline_images()` (which looks for `public://letters/images/`) will embed them. Point the CKEditor `filebrowserUploadUrl` in the compose body config to `/admin-bulk-mail/image-upload`.

- [ ] **Step 2: Attachment upload + list + delete**:

```php
function dh_admin_bulk_mail_attach($abm_id) {
  $max_total = 20 * 1024 * 1024; // ~20MB guard
  $cur = (int) db_query("select coalesce(sum(abma_size),0) from dh_admin_bulk_mail_attachment where abma_abm=:id", array(':id'=>$abm_id))->fetchField();
  $dir = 'private://admin-bulk-mail/' . $abm_id;
  file_prepare_directory($dir, FILE_CREATE_DIRECTORY);
  $validators = array();
  $file = file_save_upload('abm_attachment', $validators, $dir);
  if ($file) {
    if ($cur + $file->filesize > $max_total) {
      file_delete($file);
      drupal_set_message('Attachment exceeds the total size limit (~20MB).', 'error');
    }
    else {
      file_usage_add($file, 'dh_manageapp', 'abm', $abm_id);
      db_insert('dh_admin_bulk_mail_attachment')->fields(array(
        'abma_abm'=>$abm_id, 'abma_uri'=>$file->uri, 'abma_name'=>$file->filename,
        'abma_size'=>$file->filesize, 'abma_created'=>date('Y-m-d H:i:s'),
      ))->execute();
    }
  }
  drupal_goto('admin-bulk-mail/' . $abm_id . '/compose');
}

function dh_admin_bulk_mail_attach_delete($abm_id, $abma_id) {
  $row = db_query("select abma_uri from dh_admin_bulk_mail_attachment where abma_id=:a and abma_abm=:b", array(':a'=>$abma_id, ':b'=>$abm_id))->fetchField();
  if ($row) {
    db_delete('dh_admin_bulk_mail_attachment')->condition('abma_id', $abma_id)->execute();
    @drupal_unlink($row);
  }
  drupal_goto('admin-bulk-mail/' . $abm_id . '/compose');
}
```

- [ ] **Step 3: Show attachments on the compose form** — add to `dh_admin_bulk_mail_compose_form` a file field (`'#type'=>'file', name `abm_attachment`, posting to `admin-bulk-mail/{id}/attach`) and a list of current attachments each with a delete link to `admin-bulk-mail/{id}/attach-delete/{abma_id}`.

- [ ] **Step 4: Lint + flush.** On dipi.localhost compose page: insert an inline image via CKEditor (confirm it lands in `sites/default/files/letters/images/`), and upload 1–2 documents (confirm rows + files):

```bash
mysql -u <u> -p<p> <db> -e "select abma_id,abma_name,abma_size from dh_admin_bulk_mail_attachment order by abma_id desc limit 3"
ls -la sites/default/files/letters/images/ | tail -3
```
Expected: attachment rows present; image file saved. Try exceeding 20MB → rejected with the error message.

- [ ] **Step 5: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
git commit -m "Admin bulk email: inline images + document attachments"
```

---

### Task 7: Dedicated send function + test-send

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (`dh_send_admin_bulk_mail`, `dh_admin_bulk_mail_test_submit`)

**Interfaces:**
- Consumes: `_dh_letter_embed_inline_images()` (letters.inc:940), `get_mailgun_domain()` (letters.inc:1355), `Mailgun` SDK, `dh_admin_bulk_mail_attachment`.
- Produces: `dh_send_admin_bulk_mail(array $abm, $to_email, $to_name, $footer='', $app_id=0)` → `array('result'=>bool, 'res_id'=>string)` (throws on Mailgun error).

- [ ] **Step 1: Send function** (models letters.inc:1051-1097; note `use Mailgun\Mailgun;` is already imported at the top of letters.inc which is include_once'd — reference `\Mailgun\Mailgun` fully-qualified to be safe):

```php
function dh_send_admin_bulk_mail(array $abm, $to_email, $to_name, $footer = '', $app_id = 0) {
  $from = $abm['abm_from_name'] . ' <' . $abm['abm_from_email'] . '>';
  $mg = \Mailgun\Mailgun::create(variable_get('mailgun_key', ''));
  $test_mode = variable_get('mode_test', '0');
  $to = $test_mode ? variable_get('mode_test_emails', '') : $to_email;

  $embedded = _dh_letter_embed_inline_images($abm['abm_body']);
  $html = $embedded['html'] . $footer;

  $options = array(
    'from' => $from,
    'to' => $to,
    'subject' => $abm['abm_subject'],
    'html' => $html,
    'v:abm-id' => $abm['abm_id'],
    'v:app-id' => $app_id,
    'v:test-mode' => $test_mode,
  );
  if (!empty($abm['abm_reply_to'])) { $options['h:Reply-To'] = $abm['abm_reply_to']; }

  $atts = db_query("select abma_uri, abma_name from dh_admin_bulk_mail_attachment where abma_abm=:id", array(':id'=>$abm['abm_id']));
  foreach ($atts as $a) {
    $path = drupal_realpath($a->abma_uri);
    if ($path && file_exists($path)) { $options['attachment'][] = array('filePath'=>$path, 'filename'=>$a->abma_name); }
  }
  if (!empty($embedded['inline'])) { foreach ($embedded['inline'] as $img) { $options['inline'][] = $img; } }

  $domain = get_mailgun_domain($from);
  $res = $mg->messages()->send($domain, $options);
  return array('result' => TRUE, 'res_id' => $res->getId());
}
```

- [ ] **Step 2: Test-send submit** (replace the Task 5 stub) — save current content to the row, then send one email using the same function:

```php
function dh_admin_bulk_mail_test_submit($form, &$form_state) {
  global $user;
  $v = $form_state['values'];
  $abm_id = $form_state['abm_id'];
  db_update('dh_admin_bulk_mail')->fields(array(
    'abm_name'=>$v['abm_name'], 'abm_subject'=>$v['abm_subject'], 'abm_body'=>$v['abm_body']['value'],
    'abm_from_name'=>$v['abm_from_name'], 'abm_from_email'=>$v['abm_from_email'], 'abm_reply_to'=>$v['abm_reply_to'],
    'abm_test_email'=>$v['test_email'], 'abm_test_sent_at'=>date('Y-m-d H:i:s'),
    'abm_updated'=>date('Y-m-d H:i:s'), 'abm_updated_by'=>$user->uid,
  ))->condition('abm_id', $abm_id)->execute();
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id'=>$abm_id))->fetchAssoc();
  if (!filter_var($v['test_email'], FILTER_VALIDATE_EMAIL)) { drupal_set_message('Enter a valid test address.', 'error'); }
  else {
    try {
      dh_send_admin_bulk_mail($abm, $v['test_email'], 'Test', "<br><small>[test]</small>", 0);
      drupal_set_message('Test email sent to ' . check_plain($v['test_email']) . '.');
    } catch (\Throwable $e) {
      drupal_set_message('Test send failed: ' . check_plain($e->getMessage()), 'error');
    }
  }
  $form_state['rebuild'] = TRUE;
}
```

- [ ] **Step 3: Lint + flush.** On dipi.localhost, with `mode_test=1` and `mode_test_emails` set to a safe address you control (verify these vars first), open a compose page, enter a test address, click **Send test email**. Confirm the test arrives (correct From/subject/body, inline image rendered, attachments present). Verify no real recipients are touched.

```bash
mysql -u <u> -p<p> <db> -e "select name,value from variable where name in ('mode_test','mode_test_emails')"
```

- [ ] **Step 4: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
git commit -m "Admin bulk email: dedicated Mailgun send function + test-send"
```

---

### Task 8: Cron + consumer branch + logging + global unsubscribe

**Files:**
- Create: `cron-admin-bulk-mail.php`
- Modify: `sites/all/modules/dh_manageapp/dh_manageapp.module` (`dipi_mail_consumer_callback` admin branch)
- Modify: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (`dh_admin_bulk_mail_consume`, `dh_admin_bulk_mail_log_row`, `dh_admin_bulk_mail_unsub_footer`, `dh_admin_bulk_mail_unsubscribe`)

**Interfaces:**
- Consumes: `dh_bulk_mail_enqueue()` (Task 2), `dh_send_admin_bulk_mail()` (Task 7).
- Produces: `type:'admin'` queue messages `{type:'admin', abm_id, app_id, email, name, center}` + sentinel `{type:'admin', abm_id, completed:1}`; consumer admin branch; summary tallies + exception log rows; global unsubscribe.

- [ ] **Step 1: Cron enqueue script** (models cron-bulk-mail.php; the recipient query already selects `max(a_id) as id` per email, so re-select applicant fields when enqueuing):

```php
<?php
// cron-admin-bulk-mail.php
if (isset($_SERVER) && isset($_SERVER['REMOTE_ADDR'])) { echo "I wont run from the web\n"; exit(1); }
$lock_file = "cron-admin-bulk-mail.lock";
$f = fopen($lock_file, 'w') or die("Cannot open lock file\n");
if (!flock($f, LOCK_EX | LOCK_NB)) { die("not able to lock, exiting\n"); }
define('DRUPAL_ROOT', getcwd());
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$rows = db_query("select abm_id, abm_query from dh_admin_bulk_mail where DATE(abm_schedule_date)=CURDATE() and abm_processed=1 and abm_deleted=0");
foreach ($rows as $row) {
  db_query("update dh_admin_bulk_mail set abm_processed=2, abm_updated=:u where abm_id=:id", array(':u'=>date('Y-m-d H:i:s'), ':id'=>$row->abm_id));
  $abm_id = $row->abm_id;
  dh_bulk_mail_enqueue(
    $row->abm_query,
    function ($app_id) use ($abm_id) {
      $a = db_query("select concat(ifnull(a_f_name,''),' ',ifnull(a_l_name,'')) name, a_email, a_center from dh_applicant where a_id=:id", array(':id'=>$app_id))->fetchAssoc();
      return array('type'=>'admin', 'abm_id'=>$abm_id, 'app_id'=>$app_id, 'email'=>$a['a_email'], 'name'=>$a['name'], 'center'=>$a['a_center']);
    },
    array('type'=>'admin', 'abm_id'=>$abm_id, 'completed'=>1)
  );
  watchdog('AdminBulkMail', 'Enqueued blast @id', array('@id'=>$abm_id), WATCHDOG_INFO);
}
```

- [ ] **Step 2: Consumer branch** — at the very top of `dipi_mail_consumer_callback` (dh_manageapp.module:2996, right after `$data = json_decode($msg->body);`) insert the admin branch so the existing centre code is untouched:

```php
      if (isset($data->type) && $data->type === 'admin') {
        dh_admin_bulk_mail_consume($data);
        $msg->ack();
        return;
      }
```

- [ ] **Step 3: Consumer helper + logging + footer** (admin-bulk-mail.inc):

```php
function dh_admin_bulk_mail_log_row($abm_id, $app_id, $name, $email, $center, $status, $error) {
  db_insert('dh_admin_bulk_mail_log')->fields(array(
    'abml_abm'=>$abm_id, 'abml_applicant'=>$app_id, 'abml_name'=>$name, 'abml_email'=>$email,
    'abml_center'=>$center, 'abml_status'=>$status, 'abml_error'=>substr((string)$error, 0, 1000),
    'abml_created'=>date('Y-m-d H:i:s'),
  ))->execute();
}

function dh_admin_bulk_mail_unsub_footer($email) {
  $pass = variable_get('bulk_mail_auth_pass', '');
  $authcode = bin2hex(openssl_encrypt($email, "AES-128-ECB", $pass));
  $url = url('abm-unsubscribe/' . $authcode, array('absolute' => TRUE));
  return "<br><br><small>If you don't want to receive these emails, <a href='$url'>Unsubscribe</a></small>";
}

function dh_admin_bulk_mail_consume($data) {
  if (!empty($data->completed)) {
    db_query("update dh_admin_bulk_mail set abm_processed=3, abm_updated=:u where abm_id=:id", array(':u'=>date('Y-m-d H:i:s'), ':id'=>$data->abm_id));
    return;
  }
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id'=>$data->abm_id))->fetchAssoc();
  if (!$abm) { return; }
  $email = $data->email; $name = $data->name; $app_id = $data->app_id; $center = isset($data->center) ? $data->center : NULL;

  if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
    db_query("update dh_admin_bulk_mail set abm_skipped_invalid=abm_skipped_invalid+1 where abm_id=:id", array(':id'=>$abm['abm_id']));
    dh_admin_bulk_mail_log_row($abm['abm_id'], $app_id, $name, $email, $center, 'skipped-invalid', '');
    return;
  }
  if (db_query("select abu_id from dh_admin_bulk_mail_unsubscribe where abu_email=:e", array(':e'=>$email))->fetchField()) {
    db_query("update dh_admin_bulk_mail set abm_skipped_unsub=abm_skipped_unsub+1 where abm_id=:id", array(':id'=>$abm['abm_id']));
    dh_admin_bulk_mail_log_row($abm['abm_id'], $app_id, $name, $email, $center, 'skipped-unsub', '');
    return;
  }
  try {
    dh_send_admin_bulk_mail($abm, $email, $name, dh_admin_bulk_mail_unsub_footer($email), $app_id);
    db_query("update dh_admin_bulk_mail set abm_sent=abm_sent+1 where abm_id=:id", array(':id'=>$abm['abm_id']));
  } catch (\Throwable $e) {
    db_query("update dh_admin_bulk_mail set abm_failed=abm_failed+1 where abm_id=:id", array(':id'=>$abm['abm_id']));
    dh_admin_bulk_mail_log_row($abm['abm_id'], $app_id, $name, $email, $center, 'failed', $e->getMessage());
    watchdog('AdminBulkMail', 'send failed abm=@a email=@e: @m', array('@a'=>$abm['abm_id'], '@e'=>$email, '@m'=>$e->getMessage()), WATCHDOG_ERROR);
  }
}
```
> Note: the admin branch wraps the send in its own try/catch so one bad recipient never kills the worker (the existing centre path keeps its outer try/catch).

- [ ] **Step 4: Unsubscribe landing** (replace the Task 3 stub):

```php
function dh_admin_bulk_mail_unsubscribe($token) {
  $pass = variable_get('bulk_mail_auth_pass', '');
  $email = openssl_decrypt(pack('H*', $token), "AES-128-ECB", $pass);
  if ($email && filter_var($email, FILTER_VALIDATE_EMAIL)) {
    $exists = db_query("select abu_id from dh_admin_bulk_mail_unsubscribe where abu_email=:e", array(':e'=>$email))->fetchField();
    if (!$exists) {
      db_insert('dh_admin_bulk_mail_unsubscribe')->fields(array('abu_email'=>$email, 'abu_created'=>date('Y-m-d H:i:s')))->execute();
    }
    return '<h2>Unsubscribed</h2><p>' . check_plain($email) . ' will no longer receive these emails.</p>';
  }
  return '<h2>Invalid link</h2>';
}
```

- [ ] **Step 5: Lint** all touched files. Flush.

```bash
php7.4 -l cron-admin-bulk-mail.php && php7.4 -l sites/all/modules/dh_manageapp/dh_manageapp.module && php7.4 -l sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
```

- [ ] **Step 6: Verify end-to-end on local (test mode).** Ensure `mode_test=1`/`mode_test_emails` set. Schedule a small admin blast for **today** (Tasks 4–5, pick 1–2 centres to keep it tiny). Run the cron and the consumer once locally:

```bash
cd /dhamma/web/dipinew && php7.4 cron-admin-bulk-mail.php
# then run the consumer briefly to drain the queue (Ctrl-C after it processes):
#   drush php-eval "dipi_mail_consumer('Dipi','mail');"   (or the project's usual consumer runner)
mysql -u <u> -p<p> <db> -e "select abm_id,abm_processed,abm_count,abm_sent,abm_failed,abm_skipped_unsub,abm_skipped_invalid from dh_admin_bulk_mail order by abm_id desc limit 1"
mysql -u <u> -p<p> <db> -e "select abml_status,count(*) from dh_admin_bulk_mail_log group by abml_status"
```
Expected: `abm_processed=3` (done) after the sentinel; `abm_sent` ≈ count; only exception rows in the log (successes not logged). Confirm test emails all went to `mode_test_emails`. Click an Unsubscribe link → address appears in `dh_admin_bulk_mail_unsubscribe`; re-run → that address is `skipped-unsub`.

- [ ] **Step 7: Verify centre bulk mail STILL works** (no `type` → unchanged path): schedule a tiny centre blast, run `php7.4 cron-bulk-mail.php`, drain, confirm it sends via the existing path and logs to `dh_bulk_mail_log` as before.

- [ ] **Step 8: Commit (local only)**

```bash
git add cron-admin-bulk-mail.php sites/all/modules/dh_manageapp/dh_manageapp.module sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
git commit -m "Admin bulk email: cron + consumer admin branch + summary/exception logging + global unsubscribe"
```

---

### Task 9: Schedule list + log + delete pages

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc` (`dh_admin_bulk_mail_list`, `dh_admin_bulk_mail_delete`, `dh_admin_bulk_mail_show_log`, `dh_admin_bulk_mail_get_log`)

**Interfaces:**
- Consumes: `dh_admin_bulk_mail`, `dh_admin_bulk_mail_log`.
- Produces: the admin schedule list (with status + tallies + links), a delete/cancel action, and a DataTables SSP log page (exceptions only).

- [ ] **Step 1: List page** — a table of non-deleted blasts (draft/pending/in-progress/done/failed), each row showing name, count, sent/failed/skipped tallies, schedule date, status label, and links: Compose/Edit (if draft/pending), Show-log, Delete/Cancel. Model the status-label mapping and table rendering on `dh_show_bulk_mail_schedule` (bulk-mail.inc:80+). Include a prominent "New admin bulk email" link to `admin-bulk-mail/new`. Status labels: 0 Draft, 1 Pending, 2 In-progress, 3 Done, 4 Failed, 5 Cancelled.

```php
function dh_admin_bulk_mail_list() {
  $rows = array();
  $res = db_query("select * from dh_admin_bulk_mail where abm_deleted=0 order by abm_id desc");
  $labels = array(0=>'Draft',1=>'Pending',2=>'In-progress',3=>'Done',4=>'Failed',5=>'Cancelled');
  foreach ($res as $r) {
    $links = l('Compose', 'admin-bulk-mail/'.$r->abm_id.'/compose') . ' | ' . l('Log', 'admin-bulk-mail/'.$r->abm_id.'/show-log') . ' | ' . l('Delete', 'admin-bulk-mail/'.$r->abm_id.'/delete');
    $rows[] = array(check_plain($r->abm_name), $r->abm_count, $r->abm_sent, $r->abm_failed, ($r->abm_skipped_unsub + $r->abm_skipped_invalid), check_plain($r->abm_schedule_date), $labels[$r->abm_processed], $links);
  }
  $out = '<p>'.l('+ New admin bulk email', 'admin-bulk-mail/new').'</p>';
  $out .= theme('table', array('header'=>array('Name','Recipients','Sent','Failed','Skipped','Send date','Status','Actions'), 'rows'=>$rows));
  return $out;
}
```

- [ ] **Step 2: Delete/cancel**:

```php
function dh_admin_bulk_mail_delete($abm_id) {
  db_query("update dh_admin_bulk_mail set abm_deleted=1, abm_updated=:u where abm_id=:id", array(':u'=>date('Y-m-d H:i:s'), ':id'=>$abm_id));
  drupal_set_message('Bulk email deleted.');
  drupal_goto('admin-bulk-mail');
}
```

- [ ] **Step 3: Log page + SSP** — `dh_admin_bulk_mail_show_log($abm_id)` renders the summary counts header + a DataTables table whose ajax source is `admin-bulk-mail/{abm_id}/get-log`; `dh_admin_bulk_mail_get_log($abm_id)` returns SSP JSON of `dh_admin_bulk_mail_log` filtered by `abml_abm`. Model on `dh_show_log_bulk_mail_schedule`/`dh_get_log_bulk_mail_schedule` (bulk-mail.inc:364+) which already use the DataTables SSP helper (`dh_atportal` `ssp.class.php`). Columns: name, email, centre, status, error, time.

- [ ] **Step 4: Lint + flush.** On dipi.localhost: `admin-bulk-mail` lists the blasts from earlier tasks with correct status/tallies; open a blast's log → shows only exception rows; delete a draft → disappears from the list; row stays in DB with `abm_deleted=1`.

- [ ] **Step 5: Commit (local only)**

```bash
git add sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
git commit -m "Admin bulk email: schedule list, delete, log page (SSP, exceptions)"
```

---

## Deploy (separate, user-gated — NOT part of the build)

Only after the user gives an explicit go-ahead:
1. Apply the 4 CREATE TABLEs on prod (`docs/superpowers/plans/sql/admin-bulk-mail.sql`).
2. `git push origin master` → `ssh vri-1 'cd /dhamma/web/apps/dipi && sudo git pull && sudo drush cc all'`.
3. **Restart the `mail` queue consumer** on prod so the new `dipi_mail_consumer_callback` code loads.
4. Add `cron-admin-bulk-mail.php` to the prod cron schedule.
5. Ensure `private://admin-bulk-mail/` and `public://letters/images/` are writable.
6. Grant `admin mass mail` to the intended admin role only.
7. Verify: a tiny test blast (with `mode_test` on) end-to-end on prod, then a real small blast.

## Self-Review notes
- Spec coverage: targeting (Task 4), compose/test/schedule (5,7), inline+attachments (6), send fn (7), cron+consumer+logging+unsubscribe (8), list/log (9), shared helpers (2), schema (1), permission/routes (3). All spec sections mapped.
- The unsubscribe route was corrected to a **public** top-level `abm-unsubscribe/%` (spec §6 had it under the gated prefix).
- Type consistency: `dh_send_admin_bulk_mail(array $abm, …)` is defined in Task 7 and called in Tasks 7–8; `dh_bulk_mail_build_recipient_query`/`_clean_criteria`/`_enqueue` defined in Task 2 and used in 2/4/8; message shape `{type:'admin', abm_id, app_id, email, name, center}` produced in Task 8 cron and consumed in Task 8 consumer.
