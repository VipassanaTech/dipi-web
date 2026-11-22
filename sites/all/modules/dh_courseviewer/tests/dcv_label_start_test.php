<?php
// sites/all/modules/dh_courseviewer/tests/dcv_label_start_test.php
//
// Seat label start in the bundle: each grid's saved start (Dipi's "Seat labels…" box, read with
// dh_seat_label_starts()) goes out as label_start {main, chowky}: on each section for a main
// plan, on each group for a group-wise plan (a group with a saved start but no GROUP<n>- config
// lines gets an entry of its own; Dipi's 'gUnassigned' = the app's group 0). 0 = normal start.
// Runs against THIS worktree's code. Rows for the synthetic fixture course 9900002 (centre 5)
// are added inside a transaction that is rolled back at the end.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function sec($b, $key) { foreach ($b['sections'] as $s) { if ($s['key'] === $key) { return $s; } } return NULL; }
function grp($s, $n) { foreach ($s['groups'] as $g) { if ((int) $g['n'] === $n) { return $g; } } return NULL; }
const COURSE = 9900002;   // centre 5
$Z = array('main' => 0, 'chowky' => 0);

ok(function_exists('dh_seat_label_starts') && db_table_exists('dh_seat_label_start'), 'fixture: the label start helper and table are here');
$conv = (int) db_query("select cs_seat_naming_conv from dh_center_setting where cs_center = 5")->fetchField();
ok(!db_query("select 1 from dh_seat_label_start where sls_course = :c", array(':c' => COURSE))->fetchField(), 'fixture: no saved starts yet');

$b0 = dcv_build_bundle(COURSE);
ok(sec($b0, 'RIGHT')['label_start'] === $Z && sec($b0, 'LEFT')['label_start'] === $Z, 'nothing saved: every section starts normally');
$plainGroups = sec($b0, 'RIGHT')['groups'];

$txn = db_transaction();
try {
  $save = function ($plan, $section, $grid, $start, $c = NULL) use ($conv) {
    db_insert('dh_seat_label_start')->fields(array('sls_course' => COURSE, 'sls_plan' => $plan, 'sls_section' => $section,
      'sls_grid' => $grid, 'sls_conv' => ($c === NULL ? $conv : $c), 'sls_start' => $start, 'sls_updated_by' => 1,
      'sls_updated' => date('Y-m-d H:i:s')))->execute();
  };
  // --- main plan
  $save('main', 'M', 'main', 8); $save('main', 'M', 'chowky', 2); $save('main', 'F', 'chowky', 5);
  $save('main', 'F', 'main', 9, ($conv + 1) % 3);           // saved under another style: ignored
  $save('group', 'g1_M', 'main', 3);                        // a group plan row: not for the main plan
  $b = dcv_build_bundle(COURSE);
  ok(sec($b, 'RIGHT')['label_start'] === array('main' => 8, 'chowky' => 2), 'main plan: Male main grid 8, chowky 2');
  ok(sec($b, 'LEFT')['label_start'] === array('main' => 0, 'chowky' => 5), 'main plan: Female chowky 5; a row of another style is ignored');
  ok(count(sec($b, 'RIGHT')['groups']) === count($plainGroups) && count(sec($b, 'LEFT')['groups']) === count(sec($b0, 'LEFT')['groups']),
    'main plan: no group entries added');
  $l = dcv_build_bundle(COURSE, array('LEFT'));
  ok(count($l['sections']) === 1 && sec($l, 'LEFT')['label_start'] === array('main' => 0, 'chowky' => 5), 'a one-plan bundle keeps its start');

  // --- group-wise plan
  db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();
  $save('group', 'g2_F', 'chowky', 4);                      // no GROUP2- lines for Female in the config
  $save('group', 'gUnassigned_M', 'main', 6);               // students with no group: the app's group 0
  $g = dcv_build_bundle(COURSE);
  ok($g['course']['seat_plan'] === 'group', 'fixture: the course is group-wise now');
  ok(sec($g, 'RIGHT')['label_start'] === $Z && sec($g, 'LEFT')['label_start'] === $Z, 'group plan: the main plan starts are not sent on the sections');
  $g1 = grp(sec($g, 'RIGHT'), 1);
  ok($g1 !== NULL && $g1['label_start'] === array('main' => 3, 'chowky' => 0) && $g1['seat_direction'] === 'right', 'group 1 Male: its start, and its own settings kept');
  $g2 = grp(sec($g, 'LEFT'), 2);
  ok($g2 !== NULL && $g2['label_start'] === array('main' => 0, 'chowky' => 4), 'group 2 Female: an entry of its own with its start');
  ok($g2 !== NULL && $g2['seat_direction'] === sec($g, 'LEFT')['seat_direction'] && $g2['chowky_position'] === sec($g, 'LEFT')['chowky_position'],
    "the added group draws as its section (resolved direction and chowky side)");
  $g0 = grp(sec($g, 'RIGHT'), 0);
  ok($g0 !== NULL && $g0['label_start'] === array('main' => 6, 'chowky' => 0), "Dipi's Unassigned group = group 0");
  ok(grp(sec($g, 'LEFT'), 0) === NULL, 'no entry for a group with nothing saved and no config lines');
  $ns = array_map(function ($x) { return (int) $x['n']; }, sec($g, 'RIGHT')['groups']);
  $sorted = $ns; sort($sorted);
  ok($ns === $sorted, 'groups stay in order of n');
  $one = dcv_build_bundle(COURSE, array('LEFT-2'));
  ok(count($one['sections']) === 1 && array_map(function ($x) { return (int) $x['n']; }, sec($one, 'LEFT')['groups']) === array(2)
    && grp(sec($one, 'LEFT'), 2)['label_start'] === array('main' => 0, 'chowky' => 4), 'plan LEFT-2 only: just that group, with its start');
}
finally {
  $txn->rollback();
}
ok(!db_query("select 1 from dh_seat_label_start where sls_course = :c", array(':c' => COURSE))->fetchField()
  && (int) db_query("select c_seat_plan from dh_course where c_id = :c", array(':c' => COURSE))->fetchField() === 0, 'rolled back');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
