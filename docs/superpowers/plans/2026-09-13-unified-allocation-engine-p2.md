# Unified Allocation Engine — P2 Implementation Plan (Dining: server config + workbench)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Complete the dining feature: (a) an optional group-wise **server** config UI (activating the engine's dormant server allocation for dining), and (b) a single inline-editable **Review & Edit workbench** for dining, then make dining ready to ship.

**Architecture:** Build on P1's `inc/allocate.inc` engine (already merged on branch `dining-unified-engine`). The server config extends the existing "Dining Settings" fieldset in `centre.inc` with `[*_SERVER]` INI sections — the engine already allocates servers when such sections exist. The workbench is a new page (`inc/workbench.inc`) that renders every attendee in an editable grid, calls the engine for auto-fill, validates config via `dh_alloc_validate`, and persists all edits through one bulk-save AJAX endpoint. Dining-only for P2; the page is structured so a Cell toggle can be added in P4.

**Tech Stack:** Drupal 7.89 procedural PHP (php7.4), `db_query`/`db_update` bare table names, jQuery (already loaded), INI config in `dh_center_setting`. Tests: `php7.4` CLI scripts under `dh_manageapp/tests/`; interactive workbench behaviour verified in-browser by the controller (local site dipi.localhost), not by subagents.

**Spec:** `docs/superpowers/specs/2026-09-13-unified-allocation-engine-design.md` (§4.5 config UI, §4.6 workbench). **Predecessor:** `docs/superpowers/plans/2026-09-13-unified-allocation-engine-p1.md`.

## Global Constraints

- Legacy D7 procedural style; match surrounding code; no new framework. `git add` only named files (repo has untracked scratch).
- Engine is the single allocation authority: the workbench's auto-fill MUST call `dh_alloc_run($centre,$course,dh_alloc_descriptor('dining'),$mode)` — never re-implement allocation. Config validation MUST use `dh_alloc_validate($ini)`.
- INI grammar (servers): `[MALE_SERVER]`,`[FEMALE_SERVER]`,`[MALE_SERVER_1..9]`,`[FEMALE_SERVER_1..9]`; same keys as student sections (`Cells` OR `Old`/`New`, plus `Reserved`). Server sections are OPTIONAL — written only when non-empty, so a centre with no server dining is unchanged and servers stay unallocated.
- Eligibility unchanged from the engine: students always; servers only when server sections exist (`dh_alloc_has_server_config`).
- Dining columns already on local DB (`aa_dining`,`aa_group_dining`,`aa_dining_fixed`,`cs_has_dining`,`cs_dining_config`). No new columns in P2. `aa_group_cell` is P3.
- Routes added → `menu_rebuild()` / cache clear after. Access permission for workbench + save: `access zero day` (same as dining list pages).
- Local fixture: centre `5`, course `9900002` (dining config set, ~320 students seated). Prod deploy (ship) is a SEPARATE user-gated step: push + prod pull + `drush cc all` + the 5 dining `ALTER`s on prod DB (utf8mb4). Not a plan task.
- The workbench does NOT replace the existing Dining List pages or the Zero-Day per-person popup — they remain as shortcuts.

---

### Task 1: Optional server dining config UI (centre.inc)

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/centre.inc` (extend the "Dining Settings" fieldset ~L560–640 and `_dh_ma_diningcfg_ini`)
- Test: `sites/all/modules/dh_manageapp/tests/dining_server_config_test.php`

**Interfaces:**
- Consumes: existing `$mk_dining($sec_key,$prefix,$title,$collapsed)` closure and the `Drupal.behaviors.dhDiningGroups` add-on-demand JS already in this form.
- Produces: `_dh_ma_diningcfg_ini($input)` now also emits `[GENDER_SERVER]` (only when non-empty) and `[GENDER_SERVER_n]` (only when non-empty). New form fields under a "Server dining ranges (optional)" subsection using prefixes `diningcfg_male_server_`, `diningcfg_female_server_`, and per-group `diningcfg_<g>_server_g<n>_`.

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/dining_server_config_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/centre');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$in = array(
  'diningcfg_male_cells'=>'1-100','diningcfg_female_cells'=>'1-80',
  'diningcfg_male_server_cells'=>'S1-S20','diningcfg_male_server_reserved'=>'',
  'diningcfg_female_g2_server_cells'=>'FS1-FS5', // a server group override
);
$ini = parse_ini_string(_dh_ma_diningcfg_ini($in), true, INI_SCANNER_RAW);
ok(isset($ini['MALE']) && isset($ini['FEMALE']), 'student defaults present');
ok(isset($ini['MALE_SERVER']) && $ini['MALE_SERVER']['Cells']==='S1-S20', 'server default written');
ok(isset($ini['FEMALE_SERVER_2']) && $ini['FEMALE_SERVER_2']['Cells']==='FS1-FS5', 'server group override written');
ok(!isset($ini['FEMALE_SERVER']), 'empty server default NOT written');
ok(dh_alloc_has_server_config($ini)===true, 'engine sees server config');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/dining_server_config_test.php`
Expected: FAIL (server sections not emitted).

- [ ] **Step 3: Implement**

In `_dh_ma_diningcfg_ini($input)`: after the existing student default + group loops, add server loops using the SAME `$block($sec,$prefix,$force)` helper already defined there, with `$force=false` for ALL server sections (servers optional):
```php
	// Optional server sections — written only when non-empty (servers are opt-in).
	foreach (array('male' => 'MALE', 'female' => 'FEMALE') as $gk => $sec)
		$out .= $block($sec.'_SERVER', 'diningcfg_'.$gk.'_server_', false);
	for ($n = 1; $n <= 9; $n++)
		foreach (array('male' => 'MALE', 'female' => 'FEMALE') as $gk => $sec)
			$out .= $block($sec.'_SERVER_'.$n, 'diningcfg_'.$gk.'_server_g'.$n.'_', false);
```
In the "Dining Settings" fieldset, add a collapsed subsection "Server dining ranges (optional)" (gated on `cs_has_dining`, `#collapsed=>true`) containing: Server Male + Server Female defaults via `$mk_dining('MALE_SERVER','diningcfg_male_server_','Male servers (default)',false)` and the female equivalent; plus a server-group add-on-demand block mirroring the existing `dining_groups` block but with prefixes `diningcfg_<g>_server_g<n>_` and INI keys `GENDER_SERVER_n`. Extend the `dhDiningGroups` JS to also wire the server add/remove control (generalise it to operate on any container marked with a data attribute, or duplicate the small behaviour for the server container). Prefill from `$diniraw` (already parsed) for the server sections in edit mode, exactly as the student sections do.

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/dining_server_config_test.php` → `ALL PASS`. Also `php7.4 -l sites/all/modules/dh_manageapp/inc/centre.inc`.

- [ ] **Step 5: Controller browser check (post-review):** load `centre/5/edit`, expand Dining Settings → Server dining ranges, add a server group, Save, reopen → server ranges persist. (Done by controller; note in report.)

- [ ] **Step 6: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/centre.inc sites/all/modules/dh_manageapp/tests/dining_server_config_test.php
git commit -m "feat(dining): optional group-wise server config sections in centre settings"
```

---

### Task 2: Workbench page — grouped editable grid (render only)

**Files:**
- Create: `sites/all/modules/dh_manageapp/inc/workbench.inc`
- Modify: `sites/all/modules/dh_manageapp/dh_manageapp.module` (`include_once` the new inc; add route `dining-workbench/%centre_id/%course_id` → `dh_dining_workbench`, access `access zero day`)
- Create: `sites/all/modules/dh_manageapp/css/workbench.css`
- Test: `sites/all/modules/dh_manageapp/tests/workbench_render_test.php`

**Interfaces:**
- Produces: `dh_dining_workbench($centre, $course)` — prints an HTML page: a toolbar (Auto-fill Main / Auto-fill Group-wise / Clear non-fixed / Save all / Print — buttons are inert until Tasks 3-4), a validation panel (server-side `dh_alloc_validate` warnings from the centre's `cs_dining_config`), and an editable grid. Rows grouped by class (Students, then Servers if server config exists) → gender → old/new → AT-group. Each data row: index, name, room, `<input class="wb-main" data-aa="AAID" value="…">`, `<input class="wb-group" …>`, `<input type="checkbox" class="wb-fixed" …>`. Rows carry `data-aa` = `aa_id`.

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/workbench_render_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
ob_start(); $exit=false;
try { dh_dining_workbench(5,9900002); } catch (Exception $e) {}
$html = ob_get_clean();
ok(strpos($html,'wb-main')!==false, 'main dining inputs rendered');
ok(strpos($html,'wb-fixed')!==false, 'fixed checkboxes rendered');
ok(substr_count($html,'data-aa=')>50, 'many attendee rows rendered');
ok(strpos($html,'Auto-fill')!==false, 'toolbar present');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```
(Note: `dh_dining_workbench` normally `print`s then `exit`s like the other page fns. For testability, have it build into a `$out` string and — matching `dh_dining_list`'s `$return_html`/print pattern — accept an optional `$return=FALSE`; the test calls it so it returns/echoes without `exit`. Use `ob_get_clean()`; if the function `exit`s, refactor to echo-without-exit under a test flag as `dh_dining_list` does.)

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/workbench_render_test.php` → FAIL (function/inc missing).

- [ ] **Step 3: Implement**

Create `inc/workbench.inc` with `dh_dining_workbench($centre,$course,$return=FALSE)`:
- Load config INI (`cs_dining_config`), compute `$warnings = dh_alloc_validate($ini)` and `$has_server = dh_alloc_has_server_config($ini)`.
- Query attendees (students always; + Sevak when `$has_server`) with `aa_id, a_gender, a_old, aa_group, a_type, aa_dining, aa_group_dining, aa_dining_fixed, aa_section, aa_acco, name` in the same seniority order the engine uses (copy the ORDER BY from `dh_alloc_run`).
- Build the page: `<title>` = `dh_course_file_basename($centre,$course,'dining-workbench')`; import `workbench.css`; toolbar `<div class="wb-bar no-print">` with buttons `id="wb-fill-main" wb-fill-group wb-clear wb-save"` + a Print link + Back link; validation panel `<div class="wb-warn">` listing `$warnings` (empty → hidden); then the grid.
- Grid: iterate class (`Student`, then `Sevak` if `$has_server`) → gender (M,F) → old(1),new(0) → group. Section header row per bucket. Data row per attendee with the inputs described in Interfaces (values from DB, `check_plain`'d; the fixed checkbox `checked` when `aa_dining_fixed`). Give each `<tr>` `data-aa="{aa_id}"`.
- End: if `$return` return `$out` else `print $out; exit;` (mirror `dh_dining_list`).
Create `css/workbench.css` (compact table, sticky toolbar on screen, `@media print{.no-print{display:none}}`, inputs sized ~5em). Add to `dh_manageapp.module`: `include_once(dirname(__FILE__)."/inc/workbench.inc");` and the route (page callback `dh_dining_workbench`, page arguments the centre/course, access `access zero day`, MENU_CALLBACK).

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/workbench_render_test.php` → `ALL PASS`. `php7.4 -l` on the new inc + module.

- [ ] **Step 5: Controller browser check (post-review):** clear cache; open `dining-workbench/5/9900002` — grid renders grouped, inputs show current values, validation panel shows any config warnings.

- [ ] **Step 6: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/workbench.inc sites/all/modules/dh_manageapp/css/workbench.css sites/all/modules/dh_manageapp/dh_manageapp.module sites/all/modules/dh_manageapp/tests/workbench_render_test.php
git commit -m "feat(workbench): dining review-and-edit grid (render)"
```

---

### Task 3: Bulk save endpoint + Save-all JS

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/workbench.inc` (add `dh_dining_workbench_save()` handler + the page's save JS)
- Modify: `sites/all/modules/dh_manageapp/dh_manageapp.module` (route `dining-workbench-save/%centre_id/%course_id`, access `access zero day`)
- Test: `sites/all/modules/dh_manageapp/tests/workbench_save_test.php`

**Interfaces:**
- Consumes: the grid inputs from Task 2 (`data-aa`, `.wb-main`, `.wb-group`, `.wb-fixed`).
- Produces: `dh_dining_workbench_save()` — reads `$_POST['rows']` (array of `{aa: aa_id, dn: main, gdn: group, df: 0|1}`), validates the course/centre, guards with `dh_manageapp_lock_acquire`, and `db_update('dh_applicant_attended')` per row setting `aa_dining`, `aa_group_dining`, `aa_dining_fixed` (empty string → NULL). Returns `drupal_json_output(array('status'=>1,'saved'=>N))`. Only touches attended rows belonging to this course (guard each aa_id by joining to the course).

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/workbench_save_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$aa = db_query("select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=9900002 limit 1")->fetchField();
$_POST['rows'] = array(array('aa'=>$aa,'dn'=>'WB-7','gdn'=>'WB-G3','df'=>1));
// call the row-writer directly (factor the write into a testable helper _dh_workbench_write_rows($centre,$course,$rows))
$n = _dh_workbench_write_rows(5,9900002,$_POST['rows']);
ok($n===1, 'one row written');
$r = db_query("select aa_dining,aa_group_dining,aa_dining_fixed from dh_applicant_attended where aa_id=$aa")->fetchObject();
ok($r->aa_dining==='WB-7' && $r->aa_group_dining==='WB-G3' && $r->aa_dining_fixed==1, 'values persisted');
// a foreign aa_id (different course) must be rejected
$foreign = db_query("select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course<>9900002 limit 1")->fetchField();
$n2 = _dh_workbench_write_rows(5,9900002,array(array('aa'=>$foreign,'dn'=>'X','gdn'=>'','df'=>0)));
ok($n2===0, 'foreign course row rejected');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/workbench_save_test.php` → FAIL (`_dh_workbench_write_rows` undefined).

- [ ] **Step 3: Implement**

Add `_dh_workbench_write_rows($centre,$course,$rows)` to `inc/workbench.inc`: build the set of valid aa_ids for this course once (`select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=:c`), then for each posted row whose `aa` is in that set, `db_update('dh_applicant_attended')->fields(array('aa_dining'=>($dn===''?null:$dn),'aa_group_dining'=>($gdn===''?null:$gdn),'aa_dining_fixed'=>$df?1:0))->condition('aa_id',$aa)->execute();` counting writes; skip rows not in the set (return count of accepted). Add `dh_dining_workbench_save()` page callback: read `$_POST['rows']`, acquire the lock, call `_dh_workbench_write_rows`, `drupal_json_output(array('status'=>1,'saved'=>$n))`. Add the route. Add "Save all" JS to the workbench page: collect every `tr[data-aa]` into a `rows` array (aa, dn=.wb-main val, gdn=.wb-group val, df=.wb-fixed checked?1:0), `$.post('/dining-workbench-save/'+centre+'/'+course', {rows: rows}, ...)`, show a saved/failed toast.

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/workbench_save_test.php` → `ALL PASS`. `php7.4 -l` on inc + module.

- [ ] **Step 5: Controller browser check (post-review):** edit a couple of cells + toggle a Fixed, click Save all, reload → values persisted; confirm a foreign aa_id can't be written.

- [ ] **Step 6: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/workbench.inc sites/all/modules/dh_manageapp/dh_manageapp.module sites/all/modules/dh_manageapp/tests/workbench_save_test.php
git commit -m "feat(workbench): bulk save endpoint + Save-all (course-scoped, locked)"
```

---

### Task 4: Auto-fill (engine), clear, and live validation

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/workbench.inc` (auto-fill handling in `dh_dining_workbench` + JS for clear + client-side duplicate validation)
- Test: `sites/all/modules/dh_manageapp/tests/workbench_fill_test.php`

**Interfaces:**
- Consumes: `dh_alloc_run` (engine), `generate_dining_list`/`generate_group_dining_list` wrappers.
- Produces: `dh_dining_workbench($centre,$course)` handles `?fill=main` and `?fill=group` — runs the corresponding engine allocation (which respects Fixed rows), then re-renders the grid with fresh values. "Clear non-fixed" is client-side (empties `.wb-main`/`.wb-group` on rows whose `.wb-fixed` is unchecked; user then Saves). Live validation JS highlights duplicate non-empty values within each plan column and shows a count.

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/workbench_fill_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
// clear, then simulate ?fill=main and confirm the engine populated aa_dining
db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant set aa.aa_dining=null, aa.aa_dining_fixed=0 where a.a_course=9900002");
$_GET['fill']='main'; $_REQUEST['fill']='main';
ob_start(); dh_dining_workbench(5,9900002,TRUE); ob_end_clean();
$filled = db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=9900002 and a.a_gender='M' and aa.aa_dining is not null")->fetchField();
ok($filled>0, 'fill=main ran the engine and populated aa_dining');
unset($_GET['fill'],$_REQUEST['fill']);
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/workbench_fill_test.php` → FAIL (fill not handled).

- [ ] **Step 3: Implement**

At the top of `dh_dining_workbench`, before rendering: `if (isset($_REQUEST['fill'])) { if ($_REQUEST['fill']==='group') generate_group_dining_list($centre,$course); else generate_dining_list($centre,$course); }` — then the grid re-queries and shows fresh values. Wire the toolbar buttons: "Auto-fill Main" → `confirm('Re-fill all non-fixed dining seats from the centre ranges? Unsaved edits will be lost.')` then `location = '/dining-workbench/'+c+'/'+course+'?fill=main'`; same for group. "Clear non-fixed" → JS empties non-fixed inputs. Live validation JS: on input, for each plan column, collect non-empty values, mark duplicates with a `.wb-dup` class and update a counter in `.wb-warn`.

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/workbench_fill_test.php` → `ALL PASS`.

- [ ] **Step 5: Controller browser check (post-review):** Auto-fill Main/Group populate the grid (fixed rows preserved); Clear non-fixed empties the right cells; entering a duplicate seat highlights both.

- [ ] **Step 6: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/workbench.inc sites/all/modules/dh_manageapp/tests/workbench_fill_test.php
git commit -m "feat(workbench): engine auto-fill (main/group), clear non-fixed, live duplicate validation"
```

---

### Task 5: Course-page link + final wiring

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/course.inc` (add "Dining Workbench" to `$modules` + `$new_tab`)
- Test: manual/controller (course page renders the link; workbench reachable)

**Interfaces:**
- Consumes: the `dining-workbench` route (Task 2).

- [ ] **Step 1: Add the link**

In `course.inc` `$modules` (near the Dining List entries): `'Dining Workbench' => 'dining-workbench/'.arg(1).'/'.arg(2),` and add `'Dining Workbench'` to `$new_tab`. Do NOT add it to the regen-confirm branch (the workbench has its own controls).

- [ ] **Step 2: Verify**

`php7.4 -l sites/all/modules/dh_manageapp/inc/course.inc`. Controller: clear cache + `menu_rebuild()`; open `course/5/9900002` → "Dining Workbench" link present, opens the workbench in a new tab.

- [ ] **Step 3: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/course.inc
git commit -m "feat(workbench): course-page link to the dining workbench"
```

---

## Self-Review notes

- **Spec coverage (P2):** server config UI (§4.5) → Task 1; workbench page/grid (§4.6) → Task 2; save-all → Task 3; auto-fill + clear + validation (§4.6) → Task 4; discoverability → Task 5. "Ship dining" = user-gated deploy, out of plan scope by the Global Constraints.
- **Engine is the authority:** auto-fill calls `dh_alloc_run`; validation uses `dh_alloc_validate`. No allocation logic re-implemented in the workbench.
- **Type consistency:** `dh_dining_workbench($centre,$course,$return=FALSE)`, `dh_dining_workbench_save()`, `_dh_workbench_write_rows($centre,$course,$rows)`; grid classes `wb-main`/`wb-group`/`wb-fixed` and `data-aa` used consistently across Tasks 2–4.
- **Scope guard:** dining-only; no cell writes; no `aa_group_cell`. The page is structured for a future Cell toggle (P4) but only dining is wired.
- **Testing reality:** php7.4 CLI tests for PHP logic (render string checks, save writes, fill-runs-engine); interactive JS behaviour (save button, auto-fill navigation, clear, duplicate highlight) verified in-browser by the controller and noted in each task's report — subagents do not drive the browser.
- **Security:** save endpoint is course-scoped (rejects aa_ids outside the course) and lock-guarded; `$course`/`$centre` arrive via menu int-loaders; engine already int-casts `$course`.
