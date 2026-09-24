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
| Dashboard lifetime | One setting: show for **N days** after posting (default 30), for both audiences. After that it appears only in the archive. No per-item override, no pin. |
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
- Setting: the Drupal variable `announcement_dashboard_days` (default 30, positive integer),
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

- Heading "📣 Announcements", then one green box per recent announcement (bold title,
  small grey date, body), newest first, with no count cap.
- The footer always has "All announcements →" linking to the archive, and "Manage" for admins.
  If nothing is recent, only this compact line shows.
- **Centre:** it replaces the `variable_get('important_notice')` line in `dh_manage_centre()`.
- **AT:** it is inserted under the heading in `dh_atportal_dashboard()`, guarded by `function_exists()`.
  `dh_atportal` already calls `dh_manageapp` functions, so the two modules are always enabled together.
- Archive pages use the same boxes, with 25 per page via `PagerDefault` and `theme('pager')`.
- Styles are in the new `css/announcements.css`: block width with a max width, and full width on phones.

**Safety:** the title goes through `check_plain()`. The body goes through
`_filter_htmlcorrector(filter_xss_admin($body))`, which strips script tags, `on*` handlers and
`style` attributes, then balances any unclosed tags, so a bad paste cannot break the page.

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
2. On prod, run the migration script (it creates the table and inserts the six rows) *before*
   the pull, so the live page never queries a missing table.
3. `sudo git pull --ff-only`.
4. `sudo drush cc all` and `menu_rebuild()` for the new routes.
5. Verify: a cache-busted page returns 200, the error log is clean, the archive route is live,
   and the six boxes show on the Manage page. The browser check is done by the user.
6. After approval, delete `important_notice`.
