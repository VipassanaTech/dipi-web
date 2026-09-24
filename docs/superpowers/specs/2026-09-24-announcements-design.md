# Announcements: stored, dated, archived, auto-expiring on dashboards

Date: 2026-09-24. Status: approved design, built on local first.

## Problem

Announcements today are a single Drupal variable, `important_notice`, holding one
block of hand-written HTML. Admins edit it in the "Urgent Important Notice" textarea
on *Manage Applicant Settings* (`admin/dh_manageapp`). It is printed at the top of the
centre Manage page (`inc/centre.inc`, `dh_manage_centre()`). On prod it holds six
stacked "New:" boxes (about 3 KB). The problems:

- There is no history. A removed announcement is gone, and users can't find old ones.
- Nothing is dated, so nobody can tell what is stale.
- Nothing expires. The stack only grows until someone hand-edits the HTML.
- Posting means writing styled HTML, and a slip can break the Manage page for everyone.
- Assistant Teachers (AT Portal) never see any announcement.

## Decisions (agreed with the user)

| Topic | Decision |
|---|---|
| Audience | Two separate audiences: **centre** (all centre staff, Manage page) and **AT** (all ATs, AT Portal dashboard). Neither sees the other's announcements. |
| Authors | Site admins only (`administer manageapp`), for both audiences. No new permissions. |
| Dashboard lifetime | One setting: show for **N days** after posting (default 10; the user chose 10 at deploy), for both audiences. After that it appears only in the archive. No per-item override, no pin. |
| Writing | Title plus a rich-text body using CKEditor (the letters-editor library). The system renders the standard green box. |
| Approach | A custom `dh_announcement` table plus pages in `dh_manageapp` (option A), following DIPI conventions. |

## Data

```sql
CREATE TABLE dh_announcement (
  an_id         bigint(20)   NOT NULL AUTO_INCREMENT,
  an_audience   varchar(10)  NOT NULL,              -- 'centre' | 'at'
  an_title      varchar(200) NOT NULL,
  an_body       mediumtext   NOT NULL,              -- CKEditor HTML (sanitised on output)
  an_posted     datetime     NOT NULL,              -- drives order + dashboard window
  an_deleted    tinyint(1)   NOT NULL DEFAULT 0,    -- soft delete
  an_created    timestamp    NOT NULL DEFAULT current_timestamp(),
  an_created_by bigint(20)   NOT NULL,              -- 0 = migrated
  an_updated    timestamp    NULL DEFAULT NULL,
  an_updated_by bigint(20)   DEFAULT NULL,
  PRIMARY KEY (an_id),
  KEY an_audience_posted (an_audience, an_deleted, an_posted)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
```

- `an_posted` is set once, at creation or by the migration, and is never edited in the UI.
  Fixing a typo does not restart the N-day clock, and migrated items keep their real dates.
- Dashboard: audience matches, `an_deleted = 0`, and `an_posted >= now − N days`, newest first.
- Archive: audience matches and `an_deleted = 0`, newest first.
- Setting: the Drupal variable `announcement_dashboard_days` (default 10, positive integer),
  on *Manage Applicant Settings*. It replaces the "Urgent Important Notice" textarea.

## Pages

All code lives in the new `sites/all/modules/dh_manageapp/inc/announcements.inc`, which is
`include_once`'d by `dh_manageapp.module`. Routes are declared in `dh_manageapp_menu()`.

| Route | Access | Purpose |
|---|---|---|
| `announcements/manage` | `administer manageapp` | Admin list. Filter All / Centre / AT (`?audience=`); columns Posted, Audience, Title, Dashboard ("Showing until …" / "Archive only"), Last edited, Edit / Delete. Has a **+ New announcement** button. |
| `announcements/manage/add` | `administer manageapp` | Add form |
| `announcements/manage/%announcement_id/edit` | `administer manageapp` | Edit form. Changes audience, title and body; records `an_updated(_by)`; leaves `an_posted` alone. |
| `announcements/manage/%announcement_id/delete` | `administer manageapp` | Drupal `confirm_form`, then soft delete |
| `announcements` | `access centre` | Centre archive for the main-menu link (no centre context, no back link). This takes over the old View's path; see "Old node-based announcements". |
| `centre/%centre_id/announcements` | `access centre` (plus the centre loader) | Centre archive, with a back link to that centre's Manage page |
| `at-portal/announcements` | `access at portal` | AT archive, with a back link to the AT Portal |

**Deviation from the first draft:** the admin pages were planned under `admin/announcements`.
Overlay is enabled and `admin/*` uses the Seven admin theme (checked on local and prod), so
those pages would open in an overlay with a different look. They live under
`announcements/manage` instead, in the normal site theme, like the letters editor.

The form has an Audience radio (Centre staff / Assistant Teachers), a Title (required, 200 max),
and a Body (a textarea with CKEditor attached manually, as in `inc/letters.inc`). The compact
toolbar has Bold, Italic, Underline, bulleted and numbered lists, Link/Unlink, Remove format and
Source. There is **no text-colour button** and no image upload: `filter_xss_admin()` strips
`style` attributes on output (`includes/common.inc`), so colours would be lost, and
announcements can link to wiki pages instead of embedding images. An empty-looking body
(`<p>&nbsp;</p>`) is rejected. After saving, a message says how long it will show on the dashboard.

## Dashboard block and display

`dh_announcements_block($audience, $archive_path)` is shared by both dashboards:

- **It keeps the old banner's look (the user's preference).** There is no heading. Each recent
  announcement is one compact green box sized to its text (`display: table`), reading
  "📣 **New:** body", newest first, with no count cap. The title and posted date appear in the
  box's hover tooltip. A CKEditor `<p>` body stays on the same line as the label, and the label
  has a right margin because the whitespace collapses inside `display: table`.
- A small grey line under the boxes, "All announcements · Manage" (Manage only for admins),
  always shows, even when nothing is recent.
- **Centre:** it replaces the `variable_get('important_notice')` line in `dh_manage_centre()`.
- **AT:** it is inserted under the heading in `dh_atportal_dashboard()`, guarded by `function_exists()`.
  `dh_atportal` already calls `dh_manageapp` functions, so the two modules are always enabled together.
- Archive pages hold 25 per page via `PagerDefault` and `theme('pager')`. They are styled for
  reading (user feedback): a centred 700px column (about 90 characters per line), white cards with
  dark body text (`#2b2f2c`, line height 1.6), a green accent bar on the left, dark-green titles
  with a grey date, and a year heading whenever the year changes. The dashboard keeps its
  compact green look.
- Styles are in the new `css/announcements.css`: block width with a max width, and full width on phones.

**Safety:** the title goes through `check_plain()`. The body goes through
`_filter_htmlcorrector(filter_xss_admin($body))`, which strips script tags, `on*` handlers and
`style` attributes, then balances any unclosed tags, so a bad paste cannot break the page.

## Old node-based announcements (found during the build, merged at the user's choice)

DIPI already had an older announcements system, abandoned after Feb 2023:

- A content type `announcements` (body only). Prod has 29 published nodes, dated 2019-02 to 2023-02.
- A View `announcements`, whose page at `/announcements` has 10 per page and access `access content`
  (effectively public).
- A main-menu link "Announcements" under WIKI/Help, with a static "new" marker (a link option,
  not a real count).

**Decision: merge into the new system.** The migration copies every published node into
`dh_announcement` as a **centre** announcement, keeping its real `created` date (`an_posted`) and
author (`an_created_by`). Each body is stored rendered through its own text format
(`check_markup(body_value, body_format)`), so it looks exactly as it did on the old page.
The copy is idempotent on audience + title + posted date. The nodes themselves stay untouched.

The old View is then **disabled** with `ctools_export_set_status('views_view', 'announcements', TRUE)`
(the `views_defaults` variable). That frees `/announcements`, which becomes the new centre archive
route, and the existing main-menu link keeps working unchanged. It must stay disabled: an enabled
View page would override the route through Views' `hook_menu_alter`. Access tightens from
`access content` to `access centre`, and Drupal hides the menu link from users without access.
Rollback re-enables the View (status `FALSE`) and flushes caches.

The `node/add/announcements` link and the content type remain. Posting new nodes there would
show nowhere, but retiring them can be a later clean-up.

## Migration

A one-time, idempotent PHP script (FULL bootstrap, `php7.4`) creates the table if it is missing
and inserts the six current prod boxes as **centre** announcements. Each entry is written out
explicitly in the script (not parsed from the variable), with inline styles and the "📣 New:"
prefix removed and the wiki links kept. An entry is skipped if the same audience and title
already exist.

| # | Title | an_posted |
|---|---|---|
| 1 | Tags and Review Comments | 2026-09-02 |
| 2 | Course Configuration is now a visual form | 2026-09-08 |
| 3 | Upcoming Courses dashboard improvements | 2026-09-08 (after #2) |
| 4 | Seating plan printing on 2 or 4 A4 pages | 2026-09-11 (approx.: the feature's deploy date) |
| 5 | Cells workbench and Dining seats | 2026-09-17 |
| 6 | Easier application photos | 2026-09-19 (approx.: the feature's deploy date) |

The AT audience starts empty.

**Retiring the variable:** the settings textarea is removed, but the `important_notice` value is
left untouched until the new block has been approved on prod. That is the rollback path:
reverting the code shows the old banner again. After approval, `variable_del('important_notice')`.

## Testing (local)

- The Manage page shows the six migrated boxes newest first. The archive pages and pager work,
  and the layout holds at phone width.
- Audience separation: an AT item shows on the AT Portal and not on the Manage page, and the reverse.
  Checked as a non-admin centre user and as an AT user, not only as an admin.
- Days setting: set to 5, older items leave the dashboard but remain in the archive. Editing keeps
  `an_posted`. Delete hides an item from both places.
- Non-admins cannot reach `announcements/manage*`.
- A `<script>` in the body is stripped. An unclosed `<div>` does not break the page.
- `php -l` passes and there are no browser console errors. Test data is removed afterwards.

## Prod deploy order (after the user's go-ahead)

1. Commit and push.
2. On prod, run the migration script *before* the pull, so the live page never queries a
   missing table. It creates the table, inserts the six banner rows, and copies the old nodes.
3. `sudo git pull --ff-only`.
4. Run the view switch script. It disables the old View, then flushes all caches, which
   rebuilds the menu for the new routes including `/announcements`.
5. Verify: a cache-busted page returns 200, the error log is clean, the archive route is live,
   and the six boxes show on the Manage page. The browser check is done by the user.
6. After approval, delete `important_notice`.
