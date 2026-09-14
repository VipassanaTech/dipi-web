# Unified Allocation Engine — P1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a shared, descriptor-driven resource-allocation engine (`inc/allocate.inc`) and re-base the existing (unshipped, local-only) dining allocator onto it, with no behaviour change for dining and the cell/server capabilities built in but dormant.

**Architecture:** One engine allocates a numbered resource (cell or dining seat) to course attendees. A *resource descriptor* (which columns, config variable, shareable flag) makes it generic. Allocation is driven by pure helpers (range-expand, pool-build, section-resolve) plus one DB function (`dh_alloc_run`). Dining (unique, one seat per person) and cells (shareable in batches) differ only by the descriptor's `shareable` flag. A `class` dimension (student/server) and group-wise sections are built into the engine now; the dining server-config UI and the workbench come in P2, cell rebasing in P3.

**Tech Stack:** Drupal 7.89 procedural PHP (php7.4 on local), MySQL via `db_query`/`db_select` with bare table names, INI config blobs in `dh_center_setting`. No build step, no PHPUnit — tests are standalone `php7.4` CLI scripts (pure functions required directly; DB tests bootstrap Drupal against the local DB).

**Spec:** `docs/superpowers/specs/2026-09-13-unified-allocation-engine-design.md`

## Global Constraints

- Legacy D7 procedural style: `db_query()` with **bare** table names (no `{curly}`), procedural functions, no new framework. Match surrounding code.
- Engine lives in a NEW file `sites/all/modules/dh_manageapp/inc/allocate.inc`, `include_once`'d from `dh_manageapp.module` alongside the other incs.
- Resource descriptors (verbatim):
  - CELL: `main_col=aa_cell`, `group_col=aa_group_cell`, `fixed_col=aa_cell_fixed`, `batch_col=aa_cell_group`, `config=cs_cells_config`, `has=cs_has_cells`, `shareable=true`.
  - DINING: `main_col=aa_dining`, `group_col=aa_group_dining`, `fixed_col=aa_dining_fixed`, `batch_col=null`, `config=cs_dining_config`, `has=cs_has_dining`, `shareable=false`.
- INI section grammar: students `[MALE]`,`[FEMALE]`,`[MALE_1..9]`,`[FEMALE_1..9]`; servers `[MALE_SERVER]`,`[FEMALE_SERVER]`,`[MALE_SERVER_1..9]`,`[FEMALE_SERVER_1..9]`. Section keys are the values above. Per section: `Cells=` (combined) OR `Old=`/`New=` (split), plus `Reserved=`.
- Allocation rules (verbatim): students always eligible; servers eligible only if the config contains any `*_SERVER` section. A row whose `fixed_col=1` **and** whose target column is non-empty keeps its value and reserves it out of the pool (out-of-range allowed). Groups that resolve to the same section share one pool. `shareable=true` cycles into batches (writes `batch_col`); `shareable=false` leaves the extras blank.
- `aa_group_cell` does NOT exist yet — it is added in P3, not here. In P1 the CELL descriptor is defined and unit-tested with synthetic INI only; **no cell DB writes**.
- Local test fixture: centre `5`, course `9900002` (has `cs_has_dining=1`, a dining config, and ~320 seated students). Login helper: bootstrap + `user_pass_reset_url(user_load(1))`, prepend `http://dipi.localhost`.
- After route/menu changes: none in P1. After editing `.module`: clear cache (`drupal_flush_all_caches()` via CLI) so the new `include_once` takes effect.

---

### Task 1: Scaffold `inc/allocate.inc` — descriptors + `dh_alloc_expand()`

**Files:**
- Create: `sites/all/modules/dh_manageapp/inc/allocate.inc`
- Modify: `sites/all/modules/dh_manageapp/dh_manageapp.module` (add `include_once 'inc/allocate.inc';` next to the other inc includes near the top)
- Test: `sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`

**Interfaces:**
- Produces: `dh_alloc_descriptor($resource)` → assoc array (keys: `resource,name,has,config,main_col,group_col,fixed_col,batch_col,shareable`). `dh_alloc_expand($str)` → array of string labels.

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/allocate_pure_test.php
// Pure-function tests — no Drupal bootstrap needed.
require_once __DIR__ . '/../inc/allocate.inc';
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c){$GLOBALS['fail']=1;} }

// descriptor
$d = dh_alloc_descriptor('dining');
ok($d['main_col']==='aa_dining' && $d['group_col']==='aa_group_dining' && $d['shareable']===false, 'dining descriptor');
$c = dh_alloc_descriptor('cell');
ok($c['main_col']==='aa_cell' && $c['batch_col']==='aa_cell_group' && $c['shareable']===true, 'cell descriptor');

// expand: single, comma, hyphen range, letter-prefixed range, blanks
ok(dh_alloc_expand('') === array(), 'expand empty');
ok(dh_alloc_expand('5') === array('5'), 'expand single');
ok(dh_alloc_expand('1-3, 5') === array('1','2','3','5'), 'expand mixed');
ok(dh_alloc_expand('A1-A3') === array('A1','A2','A3'), 'expand letter range');
ok(dh_alloc_expand('1, , 2') === array('1','2'), 'expand skips blanks');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: fatal / "Call to undefined function dh_alloc_descriptor".

- [ ] **Step 3: Write minimal implementation**

```php
<?php
// sites/all/modules/dh_manageapp/inc/allocate.inc
/**
 * Unified resource-allocation engine (cells + dining). See
 * docs/superpowers/specs/2026-09-13-unified-allocation-engine-design.md.
 */

/** Resource descriptor: 'cell' or 'dining'. */
function dh_alloc_descriptor($resource) {
  if ($resource === 'cell') {
    return array('resource'=>'cell','name'=>'Cell','has'=>'cs_has_cells','config'=>'cs_cells_config',
      'main_col'=>'aa_cell','group_col'=>'aa_group_cell','fixed_col'=>'aa_cell_fixed',
      'batch_col'=>'aa_cell_group','shareable'=>true);
  }
  return array('resource'=>'dining','name'=>'Dining','has'=>'cs_has_dining','config'=>'cs_dining_config',
    'main_col'=>'aa_dining','group_col'=>'aa_group_dining','fixed_col'=>'aa_dining_fixed',
    'batch_col'=>null,'shareable'=>false);
}

/** Expand "1-40, 55, A1-A5" into an array of labels (letter-prefix aware). */
function dh_alloc_expand($str) {
  $out = array();
  if ($str === '' || $str === null) return $out;
  foreach (explode(',', $str) as $v) {
    $v = trim($v);
    if ($v === '') continue;
    $p = explode('-', $v);
    if (count($p) > 1) {
      $prefix = preg_replace('/[0-9]/', '', $p[0]);
      $a = (int) preg_replace('/[A-Za-z-]/', '', $p[0]);
      $b = (int) preg_replace('/[A-Za-z-]/', '', $p[1]);
      for ($i = $a; $i <= $b; $i++) $out[] = $prefix . $i;
    } else {
      $out[] = $p[0];
    }
  }
  return $out;
}
```

Then add near the top of `dh_manageapp.module`, with the other `include_once`s:

```php
include_once 'sites/all/modules/dh_manageapp/inc/allocate.inc';
```
(Match the exact relative-path style already used for the neighbouring `inc/*.inc` includes in that file.)

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: `ALL PASS`. Also `php7.4 -l sites/all/modules/dh_manageapp/inc/allocate.inc` → no syntax errors.

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/allocate.inc sites/all/modules/dh_manageapp/dh_manageapp.module sites/all/modules/dh_manageapp/tests/allocate_pure_test.php
git commit -m "feat(alloc): engine scaffold - descriptors + range expand"
```

---

### Task 2: `dh_alloc_pool()` — build a section's pool

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/allocate.inc`
- Test: `sites/all/modules/dh_manageapp/tests/allocate_pure_test.php` (append)

**Interfaces:**
- Consumes: `dh_alloc_expand()`.
- Produces: `dh_alloc_pool($ini, $key)` → `array('sep'=>bool,'cells'=>[],'old'=>[],'new'=>[],'reserved'=>[])`. `$ini` is a parsed INI array; `$key` a section key. Missing section → all-empty pool.

- [ ] **Step 1: Write the failing test** (append to `allocate_pure_test.php`, before the final echo)

```php
$ini = array(
  'MALE'   => array('Cells'=>'1-4','Reserved'=>'2'),
  'FEMALE' => array('Old'=>'1-2','New'=>'3-4','Reserved'=>''),
);
$pm = dh_alloc_pool($ini,'MALE');
ok($pm['sep']===false && $pm['cells']===array('1','2','3','4') && $pm['reserved']===array('2'), 'pool combined');
$pf = dh_alloc_pool($ini,'FEMALE');
ok($pf['sep']===true && $pf['old']===array('1','2') && $pf['new']===array('3','4'), 'pool split');
$pmiss = dh_alloc_pool($ini,'MALE_1');
ok($pmiss['cells']===array() && $pmiss['old']===array() && $pmiss['reserved']===array(), 'pool missing section');
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: FAIL "Call to undefined function dh_alloc_pool".

- [ ] **Step 3: Write minimal implementation** (append to `inc/allocate.inc`)

```php
/** Build a pool descriptor for one INI section. */
function dh_alloc_pool($ini, $key) {
  $sec = (isset($ini[$key]) && is_array($ini[$key])) ? $ini[$key] : array();
  $reserved = dh_alloc_expand(isset($sec['Reserved']) ? $sec['Reserved'] : '');
  $sep = (isset($sec['Old']) && $sec['Old'] !== '') || (isset($sec['New']) && $sec['New'] !== '');
  if ($sep) {
    return array('sep'=>true, 'cells'=>array(),
      'old'=>dh_alloc_expand(isset($sec['Old']) ? $sec['Old'] : ''),
      'new'=>dh_alloc_expand(isset($sec['New']) ? $sec['New'] : ''),
      'reserved'=>$reserved);
  }
  return array('sep'=>false, 'old'=>array(), 'new'=>array(),
    'cells'=>dh_alloc_expand(isset($sec['Cells']) ? $sec['Cells'] : ''),
    'reserved'=>$reserved);
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: `ALL PASS`.

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/allocate.inc sites/all/modules/dh_manageapp/tests/allocate_pure_test.php
git commit -m "feat(alloc): pool builder (combined / old-new split / reserved)"
```

---

### Task 3: `dh_alloc_effective_key()` + `dh_alloc_has_server_config()` — resolve section

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/allocate.inc`
- Test: `sites/all/modules/dh_manageapp/tests/allocate_pure_test.php` (append)

**Interfaces:**
- Produces:
  - `dh_alloc_effective_key($ini, $gender, $group, $class, $mode)` → section-key string, or `null` when a server has no server section. `$gender` `'M'|'F'`; `$group` int (0 = none); `$class` `'student'|'server'`; `$mode` `'default'|'group'`.
  - `dh_alloc_has_server_config($ini)` → bool (any section key contains `_SERVER`).

- [ ] **Step 1: Write the failing test** (append)

```php
$ini2 = array('MALE'=>array('Cells'=>'1'),'MALE_1'=>array('Cells'=>'D1'),'MALE_SERVER'=>array('Cells'=>'S1'));
// student, default -> MALE ; group with override -> MALE_1 ; group w/o override -> MALE
ok(dh_alloc_effective_key($ini2,'M',0,'student','default')==='MALE', 'student default');
ok(dh_alloc_effective_key($ini2,'M',1,'student','group')==='MALE_1', 'student group override');
ok(dh_alloc_effective_key($ini2,'M',2,'student','group')==='MALE', 'student group fallback');
// server -> MALE_SERVER ; female server absent -> null
ok(dh_alloc_effective_key($ini2,'M',0,'server','default')==='MALE_SERVER', 'server default');
ok(dh_alloc_effective_key($ini2,'F',0,'server','default')===null, 'server absent -> null');
ok(dh_alloc_has_server_config($ini2)===true, 'has server config');
ok(dh_alloc_has_server_config(array('MALE'=>array()))===false, 'no server config');
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: FAIL "undefined function dh_alloc_effective_key".

- [ ] **Step 3: Write minimal implementation** (append)

```php
/** True if the config defines any server section. */
function dh_alloc_has_server_config($ini) {
  if (!is_array($ini)) return false;
  foreach ($ini as $k => $v) {
    if (strpos($k, '_SERVER') !== false) return true;
  }
  return false;
}

/** Resolve the INI section key for one attendee, or null (server w/o section). */
function dh_alloc_effective_key($ini, $gender, $group, $class, $mode) {
  $base = (strtoupper($gender) === 'M') ? 'MALE' : 'FEMALE';
  if ($class === 'server') $base .= '_SERVER';
  if ($mode === 'group') {
    $g = (int) $group;
    if ($g > 0 && isset($ini[$base . '_' . $g])) return $base . '_' . $g;
  }
  if (isset($ini[$base])) return $base;
  // Students always resolve to their base (pool may be empty -> blank);
  // servers only when a server section exists.
  return ($class === 'server') ? null : $base;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: `ALL PASS`.

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/allocate.inc sites/all/modules/dh_manageapp/tests/allocate_pure_test.php
git commit -m "feat(alloc): section resolver (gender/group/class) + server-config probe"
```

---

### Task 4: `dh_alloc_run()` — unique allocation (dining path)

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/allocate.inc`
- Test: `sites/all/modules/dh_manageapp/tests/allocate_run_test.php`

**Interfaces:**
- Consumes: descriptor, `dh_alloc_pool`, `dh_alloc_effective_key`, `dh_alloc_has_server_config`.
- Produces: `dh_alloc_run($centre, $course, $desc, $mode)` → int (rows written). Writes `$desc['main_col']` when `$mode==='default'`, `$desc['group_col']` when `'group'`. Students always; servers when server config present. Skips rows where `fixed_col=1` and the target column is non-empty (reserving that value out). `shareable=false`: extras blank (NULL). (Shareable branch added in Task 5.)

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/allocate_run_test.php
// DB test against the local fixture (centre 5 / course 9900002).
define('DRUPAL_ROOT', '/dhamma/web/dipinew');
chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$centre=5; $course=9900002;
$desc = dh_alloc_descriptor('dining');

// set a clean known config (M reserve 5,6; group override for MALE_1)
$cfg="[MALE]\nCells = 1-200\nReserved = 5,6\n\n[FEMALE]\nCells = 1-150\nReserved = \n\n[MALE_1]\nCells = D1-D80\nReserved = \n";
db_update('dh_center_setting')->fields(array('cs_has_dining'=>1,'cs_dining_config'=>$cfg))->condition('cs_center',$centre)->execute();
// clear any fixed flags for a clean baseline
db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant set aa.aa_dining_fixed=0, aa.aa_dining=null, aa.aa_group_dining=null where a.a_course=$course");

$n = dh_alloc_run($centre,$course,$desc,'default');
ok($n>0, "default wrote $n rows");
$m5 = db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and aa.aa_dining in ('5','6')")->fetchField();
ok($m5==0, 'male reserved 5,6 excluded');
$dupe = db_query("select count(*) from (select aa_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and aa_dining is not null group by aa_dining having count(*)>1) t")->fetchField();
ok($dupe==0, 'male dining unique');

// fixed kept + reserved out (out of range)
$aa=db_query("select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and a.a_type='Student' limit 1")->fetchField();
db_update('dh_applicant_attended')->fields(array('aa_dining'=>'9999','aa_dining_fixed'=>1))->condition('aa_id',$aa)->execute();
dh_alloc_run($centre,$course,$desc,'default');
$kept=db_query("select aa_dining from dh_applicant_attended where aa_id=$aa")->fetchField();
ok($kept==='9999', 'fixed out-of-range value kept');
$only=db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and aa.aa_dining='9999'")->fetchField();
ok($only==1, 'fixed value reserved out for others');

// group override: MALE_1 has 80 D-seats, 160 in group -> 80 blank, values are D*
$ng=dh_alloc_run($centre,$course,$desc,'group');
ok($ng>0, "group wrote $ng rows");
$blank=db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and aa.aa_group=1 and (aa.aa_group_dining is null or aa.aa_group_dining='')")->fetchField();
ok($blank>0, 'group pool exhaustion leaves blanks');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_run_test.php`
Expected: FAIL "undefined function dh_alloc_run".

- [ ] **Step 3: Write minimal implementation** (append to `inc/allocate.inc`)

```php
/** Allocate a resource to a course's attendees. Returns rows written. */
function dh_alloc_run($centre, $course, $desc, $mode) {
  $col = ($mode === 'group') ? $desc['group_col'] : $desc['main_col'];
  $fixed_col = $desc['fixed_col'];
  $cfg = db_query("select ".$desc['config']." from dh_center_setting where cs_center=:c", array(':c'=>$centre))->fetchField();
  $ini = ($cfg !== '' && $cfg !== null) ? parse_ini_string($cfg, true, INI_SCANNER_RAW) : array();
  if (!is_array($ini)) $ini = array();
  $servers = dh_alloc_has_server_config($ini);

  // Attendees in seniority/course order (students; + servers when eligible).
  $type_filter = $servers ? "and a_type in ('Student','Sevak')" : "and a_type='Student'";
  $q = "select aa_id, a_gender, a_old, aa_group, a_type, $col as cur, $fixed_col as fixed
    from dh_applicant
    left join dh_applicant_course on a_id=ac_applicant
    left join dh_applicant_attended on a_id=aa_applicant
    left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender))
    where a_course='$course' and a_attended=1 $type_filter
    order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date('Y')."),
      IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc,
      IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc,
      IFNULL(ac_teen,0) desc, TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) desc, a_id";
  $rows = db_query($q)->fetchAll();

  $classof = function($r){ return (strtolower($r->a_type) === 'sevak') ? 'server' : 'student'; };
  $keyof = function($r) use ($ini, $mode, $classof) {
    return dh_alloc_effective_key($ini, $r->a_gender, $r->aa_group, $classof($r), $mode);
  };

  // Pass 1: reserve fixed (non-empty) values out of their pool.
  $fixedvals = array();
  foreach ($rows as $r) {
    if ($r->fixed && trim((string)$r->cur) !== '') {
      $k = $keyof($r);
      if ($k !== null) $fixedvals[$k][] = trim((string)$r->cur);
    }
  }
  // Build queues per distinct key (config-reserved + fixed removed).
  $queues = array();
  foreach ($rows as $r) {
    $k = $keyof($r);
    if ($k === null || isset($queues[$k])) continue;
    $p = dh_alloc_pool($ini, $k);
    $remove = array_merge($p['reserved'], isset($fixedvals[$k]) ? $fixedvals[$k] : array());
    $clean = function($arr) use ($remove) {
      return array_values(array_filter($arr, function($x) use ($remove) { return $x !== '' && !in_array($x, $remove); }));
    };
    $queues[$k] = array('sep'=>$p['sep'], 'old'=>$clean($p['old']), 'new'=>$clean($p['new']), 'cells'=>$clean($p['cells']));
  }
  // Bucket rows by key -> old/new preserving order.
  $buckets = array();
  foreach ($rows as $r) {
    $k = $keyof($r);
    if ($k === null) continue;
    $buckets[$k][$r->a_old ? 'old' : 'new'][] = $r;
  }

  $updates = array();
  foreach ($buckets as $k => $on) {
    foreach (array('old','new') as $type) {
      if (empty($on[$type])) continue;
      foreach ($on[$type] as $r) {
        if ($r->fixed && trim((string)$r->cur) !== '') continue; // keep fixed
        $slot = $queues[$k]['sep'] ? $type : 'cells';
        $seat = !empty($queues[$k][$slot]) ? array_shift($queues[$k][$slot]) : '';
        $updates[$r->aa_id] = ($seat === '') ? null : $seat;
      }
    }
  }
  foreach ($updates as $aa_id => $val) {
    db_update('dh_applicant_attended')->fields(array($col => $val))->condition('aa_id', $aa_id)->execute();
  }
  return count($updates);
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_run_test.php`
Expected: `ALL PASS`.

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/allocate.inc sites/all/modules/dh_manageapp/tests/allocate_run_test.php
git commit -m "feat(alloc): dh_alloc_run - unique allocation with fixed/reserved/group/server"
```

---

### Task 5: `dh_alloc_run()` — shareable/batch branch (cells-ready)

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/allocate.inc` (extend `dh_alloc_run`)
- Test: `sites/all/modules/dh_manageapp/tests/allocate_batch_test.php`

**Interfaces:**
- Produces: when `$desc['shareable']===true`, `dh_alloc_run` cycles a pool that is smaller than its bucket into rounds — the same seat is reused; each attendee's `$desc['batch_col']` records the 0-based round. Unique (`shareable=false`) behaviour from Task 4 is unchanged.

Because P1 must not write cell columns to the DB (`aa_group_cell` does not exist yet), this task tests the batch **logic** by factoring the seat-assignment loop into a pure helper and testing that directly.

- [ ] **Step 1: Write the failing test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/allocate_batch_test.php
require_once '/dhamma/web/dipinew/sites/all/modules/dh_manageapp/inc/allocate.inc';
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
// 5 people, pool of 2, shareable -> seats cycle, batch increments every 2.
$pool = array('1','2');
$people = array('a','b','c','d','e');
$res = dh_alloc_assign_batches($people, $pool);
// expected: a->(1,b0) b->(2,b0) c->(1,b1) d->(2,b1) e->(1,b2)
ok($res['a']===array('1',0), 'a seat1 batch0');
ok($res['c']===array('1',1), 'c seat1 batch1');
ok($res['e']===array('1',2), 'e seat1 batch2');
// empty pool -> everyone blank batch0
$res2 = dh_alloc_assign_batches(array('x','y'), array());
ok($res2['x']===array('',0) && $res2['y']===array('',0), 'empty pool -> blanks');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_batch_test.php`
Expected: FAIL "undefined function dh_alloc_assign_batches".

- [ ] **Step 3: Write minimal implementation** (append helper to `inc/allocate.inc`; then use it in the shareable path of `dh_alloc_run`)

```php
/**
 * Assign a shareable pool to an ordered list of items, cycling into batches.
 * Returns map item => array(seat_or_'', batch_index). Empty pool -> all ('',0).
 */
function dh_alloc_assign_batches($items, $pool) {
  $out = array();
  $n = count($pool);
  $i = 0;
  foreach ($items as $it) {
    if ($n === 0) { $out[$it] = array('', 0); continue; }
    $out[$it] = array($pool[$i % $n], intdiv($i, $n));
    $i++;
  }
  return $out;
}
```

In `dh_alloc_run`, replace the per-bucket assignment loop so that when `$desc['shareable']` is true it uses `dh_alloc_assign_batches()` per (key, old/new) bucket and also records the batch into `$desc['batch_col']`:

```php
  $batch_col = $desc['batch_col'];
  $updates = array();      // aa_id => array(col=>val, [batch_col=>b])
  foreach ($buckets as $k => $on) {
    foreach (array('old','new') as $type) {
      if (empty($on[$type])) continue;
      $slot = $queues[$k]['sep'] ? $type : 'cells';
      $pool = $queues[$k][$slot];
      // keep only non-fixed rows for assignment; fixed keep their value
      $assignable = array();
      foreach ($on[$type] as $r) { if (!($r->fixed && trim((string)$r->cur) !== '')) $assignable[] = $r; }
      if ($desc['shareable']) {
        $ids = array(); foreach ($assignable as $r) $ids[] = $r->aa_id;
        $map = dh_alloc_assign_batches($ids, $pool);
        foreach ($assignable as $r) {
          list($seat,$b) = $map[$r->aa_id];
          $f = array($col => ($seat === '' ? null : $seat));
          if ($batch_col) $f[$batch_col] = $b;
          $updates[$r->aa_id] = $f;
        }
      } else {
        $q = $pool;
        foreach ($assignable as $r) {
          $seat = !empty($q) ? array_shift($q) : '';
          $updates[$r->aa_id] = array($col => ($seat === '' ? null : $seat));
        }
      }
    }
  }
  foreach ($updates as $aa_id => $fields) {
    db_update('dh_applicant_attended')->fields($fields)->condition('aa_id', $aa_id)->execute();
  }
  return count($updates);
```

(Remove the Task-4 `$updates`/persist block being replaced. The unique path is preserved exactly by the `else` branch.)

- [ ] **Step 4: Run tests to verify they pass**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_batch_test.php` → `ALL PASS`.
Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_run_test.php` → `ALL PASS` (dining unique path unchanged).

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/allocate.inc sites/all/modules/dh_manageapp/tests/allocate_batch_test.php
git commit -m "feat(alloc): shareable batch cycling (cells-ready) via dh_alloc_assign_batches"
```

---

### Task 6: `dh_alloc_validate()` — config sanity warnings

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/allocate.inc`
- Test: `sites/all/modules/dh_manageapp/tests/allocate_pure_test.php` (append)

**Interfaces:**
- Produces: `dh_alloc_validate($ini)` → array of human-readable warning strings: (a) a `Reserved` value not present in that section's available range (inert reserved), (b) a duplicate literal within a section's available list. Empty array = clean.

- [ ] **Step 1: Write the failing test** (append)

```php
$bad = array('MALE'=>array('Cells'=>'1-3','Reserved'=>'9'), 'FEMALE'=>array('Cells'=>'1,1,2','Reserved'=>''));
$w = dh_alloc_validate($bad);
ok(count(array_filter($w, function($s){return strpos($s,'MALE')!==false && stripos($s,'reserved')!==false;}))>0, 'flags out-of-range reserved');
ok(count(array_filter($w, function($s){return strpos($s,'FEMALE')!==false && stripos($s,'duplicate')!==false;}))>0, 'flags duplicate');
ok(dh_alloc_validate(array('MALE'=>array('Cells'=>'1-3','Reserved'=>'2')))===array(), 'clean config no warnings');
```

- [ ] **Step 2: Run test to verify it fails**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: FAIL "undefined function dh_alloc_validate".

- [ ] **Step 3: Write minimal implementation** (append)

```php
/** Return human-readable warnings about a parsed config (empty = clean). */
function dh_alloc_validate($ini) {
  $warn = array();
  if (!is_array($ini)) return $warn;
  foreach ($ini as $key => $sec) {
    if (!is_array($sec)) continue;
    $avail = array_merge(
      dh_alloc_expand(isset($sec['Cells']) ? $sec['Cells'] : ''),
      dh_alloc_expand(isset($sec['Old']) ? $sec['Old'] : ''),
      dh_alloc_expand(isset($sec['New']) ? $sec['New'] : '')
    );
    $counts = array_count_values($avail);
    foreach ($counts as $v => $c) {
      if ($c > 1) $warn[] = "[$key] duplicate value '$v' in available list.";
    }
    foreach (dh_alloc_expand(isset($sec['Reserved']) ? $sec['Reserved'] : '') as $r) {
      if ($r !== '' && !in_array($r, $avail)) $warn[] = "[$key] reserved value '$r' is outside the available range (has no effect).";
    }
  }
  return $warn;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/allocate_pure_test.php`
Expected: `ALL PASS`.

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/allocate.inc sites/all/modules/dh_manageapp/tests/allocate_pure_test.php
git commit -m "feat(alloc): config validation warnings (inert reserved, duplicates)"
```

---

### Task 7: Re-base dining onto the engine + parity check

**Files:**
- Modify: `sites/all/modules/dh_manageapp/inc/zero-day.inc` (rewrite `generate_dining_list` / `generate_group_dining_list` to call the engine; delete `_dh_dining_allocate`, `_dh_dining_pool`, `_dh_dining_expand`)
- Test: `sites/all/modules/dh_manageapp/tests/dining_parity_test.php`

**Interfaces:**
- Consumes: `dh_alloc_run()`, `dh_alloc_descriptor()`.
- Produces: `generate_dining_list($centre,$course)` and `generate_group_dining_list($centre,$course)` unchanged signatures; now thin wrappers over the engine. `dh_dining_list()` display is untouched (reads the same columns).

- [ ] **Step 1: Write the failing/parity test**

```php
<?php
// sites/all/modules/dh_manageapp/tests/dining_parity_test.php
// Proves the engine reproduces the old allocator's output for dining.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$centre=5; $course=9900002;
// snapshot after new wrappers run
generate_dining_list($centre,$course);
generate_group_dining_list($centre,$course);
$after = db_query("select aa_id, aa_dining, aa_group_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course order by aa_id")->fetchAllKeyed(0,1);
ok(count($after)>0, 'wrappers ran and wrote rows');
// engine-direct produces identical result
$d = dh_alloc_descriptor('dining');
dh_alloc_run($centre,$course,$d,'default'); dh_alloc_run($centre,$course,$d,'group');
$direct = db_query("select aa_id, aa_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course order by aa_id")->fetchAllKeyed(0,1);
ok($after==$direct, 'wrapper output == engine output');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
```

- [ ] **Step 2: Run test to verify current state**

Run: `php7.4 sites/all/modules/dh_manageapp/tests/dining_parity_test.php`
Expected: runs (old wrappers exist) — record the result; it should PASS after Step 3 too. (The parity assertion is the real gate in Step 4.)

- [ ] **Step 3: Rewrite the dining wrappers**

Replace `generate_dining_list` / `generate_group_dining_list` (currently at `inc/zero-day.inc:4017-4018`) and delete the three `_dh_dining_*` helpers (`_dh_dining_expand` ~L3899, `_dh_dining_pool` ~L3922, `_dh_dining_allocate` ~L3938):

```php
function generate_dining_list($centre, $course) {
  return dh_alloc_run($centre, $course, dh_alloc_descriptor('dining'), 'default');
}
function generate_group_dining_list($centre, $course) {
  return dh_alloc_run($centre, $course, dh_alloc_descriptor('dining'), 'group');
}
```

Leave `dh_dining_list()` (the display page) exactly as-is.

- [ ] **Step 4: Verify parity + lint + pages**

Run: `php7.4 -l sites/all/modules/dh_manageapp/inc/zero-day.inc` → no syntax errors.
Run: `php7.4 sites/all/modules/dh_manageapp/tests/dining_parity_test.php` → `ALL PASS`.
Manual (local, logged in): open `dining-list/5/9900002` and `group-dining-list/5/9900002?r=1` — pages render, Regenerate works, values populate; open `teacher-list/5/9900002` — Dining columns still present. Confirm 0 new `php` watchdog errors.

- [ ] **Step 5: Commit**

```bash
git add sites/all/modules/dh_manageapp/inc/zero-day.inc sites/all/modules/dh_manageapp/tests/dining_parity_test.php
git commit -m "refactor(dining): re-base allocators onto the unified engine; drop _dh_dining_*"
```

---

## Self-Review notes

- **Spec coverage (P1 slice):** engine core (§4.2) → Tasks 1–3,6; allocation semantics incl. fixed/reserved/group/server/shareable (§4.3) → Tasks 4–5; dining re-based (§5 P1) → Task 7. Out of P1 by design: dining server-config UI + workbench (P2), cell rebase + `aa_group_cell` + parity diff (P3), centre-settings shared form builder (P2). No P1 requirement left unimplemented.
- **No cell DB writes in P1:** CELL descriptor is defined and its shareable logic unit-tested (Task 5) with synthetic data only; `aa_group_cell` is not created here.
- **Type consistency:** `dh_alloc_run($centre,$course,$desc,$mode)`, `dh_alloc_descriptor($resource)`, `dh_alloc_pool($ini,$key)`, `dh_alloc_effective_key($ini,$gender,$group,$class,$mode)`, `dh_alloc_has_server_config($ini)`, `dh_alloc_expand($str)`, `dh_alloc_validate($ini)`, `dh_alloc_assign_batches($items,$pool)` — used consistently across tasks.
- **Testing reality:** no PHPUnit in this project; tests are `php7.4` CLI scripts under `dh_manageapp/tests/` (pure ones `require_once` the inc directly; DB ones bootstrap Drupal against the local fixture centre 5 / course 9900002). These are dev-time checks, not CI.
