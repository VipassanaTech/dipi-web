<?php
// sites/all/modules/dh_courseviewer/tests/dcv_halls_test.php
//
// Dhamma Halls in the bundle (dh_manageapp inc/halls.inc): at a centre with 2+ halls the bundle gets 'halls'
// (code + name, in order), 'group_halls' (the hall code of every group 1-9 for this course, from
// dh_halls_js_codes(): every group, also those with no GROUP<n>- seat config lines, and on main-plan courses too)
// and each section group 1-9 gets hall_code / hall_name; the "choose plans" labels of group-wise plans name the
// hall ("Group 2 · Dhamma Hall 3 · Male"). With 0 or 1 hall the bundle and the labels are exactly as before. schema_version stays 2 (added fields only; released apps ignore unknown keys).
// Synthetic fixture course 9900002 (centre 5), made group-wise and given halls inside a transaction that is rolled back.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function plain($b) { unset($b['generated_at']); return json_encode($b); }
function groups_of($b) { $out = array(); foreach ($b['sections'] as $s) { foreach ((isset($s['groups']) ? $s['groups'] : array()) as $g) { $out[] = $g; } } return $out; }
const COURSE = 9900002;   // centre 5
const CENTRE = 5;

ok(function_exists('dh_halls_course_map') && db_table_exists('dh_center_hall'), 'fixture: the halls helpers and tables are here');
$txn = db_transaction();
try {
  $now = date('Y-m-d H:i:s');
  $hall = function ($code, $name, $w) use ($now) {
    return (int) db_insert('dh_center_hall')->fields(array('ch_center' => CENTRE, 'ch_code' => $code, 'ch_name' => $name, 'ch_weight' => $w,
      'ch_deleted' => 0, 'ch_created' => $now, 'ch_created_by' => 0, 'ch_updated' => $now, 'ch_updated_by' => 0))->execute();
  };
  db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();   // group-wise: group entries with n
  db_update('dh_center_hall')->fields(array('ch_deleted' => 1))->condition('ch_center', CENTRE)->execute();
  db_delete('dh_center_hall_group')->condition('chg_center', CENTRE)->execute();
  db_delete('dh_course_hall_group')->condition('cg_course', COURSE)->execute();
  dh_halls_reset();
  $b0 = dcv_build_bundle(COURSE);
  ok(count(groups_of($b0)) > 0, 'fixture: the course has group entries');
  ok(!array_key_exists('halls', $b0) && !array_key_exists('group_halls', $b0), 'no halls: no halls list, no group_halls');
  $labels = function () { return array_map(function ($p) { return $p['label']; }, dcv_course_plans(COURSE)); };
  $l0 = $labels();
  ok(count($l0) > 0 && strpos($l0[0], 'Group ') === 0, 'fixture: group-wise plan labels');
  ok(count(array_filter(groups_of($b0), function ($g) { return array_key_exists('hall_code', $g); })) === 0, 'no halls: no group has a hall');

  $hall('HA', 'Hall Alpha', 0);
  dh_halls_reset();
  ok(plain(dcv_build_bundle(COURSE)) === plain($b0), 'one hall: the bundle is exactly as before');
  ok($labels() === $l0, 'one hall: the plan labels are exactly as before');

  $hb = $hall('HB', 'Hall Beta', 1);
  db_insert('dh_center_hall_group')->fields(array('chg_center' => CENTRE, 'chg_group' => 1, 'chg_hall' => $hb))->execute();
  dh_halls_reset();
  $b2 = dcv_build_bundle(COURSE);
  ok($b2['schema_version'] === 2, 'two halls: schema_version stays 2');
  ok(isset($b2['halls']) && $b2['halls'] === array(array('code' => 'HA', 'name' => 'Hall Alpha'), array('code' => 'HB', 'name' => 'Hall Beta')),
    'two halls: the halls list, in order');
  $keys = array_keys($b2);
  ok(array_slice($keys, -2) === array('halls', 'group_halls'), 'two halls: halls then group_halls are the last keys (the others keep their order)');
  $wantMap = array('1' => 'HB'); for ($g = 2; $g <= 9; $g++) { $wantMap[(string) $g] = 'HA'; }
  ok(isset($b2['group_halls']) && $b2['group_halls'] === $wantMap, 'two halls: group_halls = every group 1-9 (1 = HB, the rest HA)');
  ok(json_encode($b2['group_halls']) === '{"1":"HB","2":"HA","3":"HA","4":"HA","5":"HA","6":"HA","7":"HA","8":"HA","9":"HA"}',
    'two halls: group_halls goes out as a JSON object keyed by group');
  $okLabels = TRUE;
  foreach (dcv_course_plans(COURSE) as $p) {
    list($sec, $n) = explode('-', $p['id']);
    $side = ($sec === 'RIGHT') ? 'Male' : 'Female';
    $hallName = ((int) $n === 1) ? 'Hall Beta' : 'Hall Alpha';
    $expect = ((int) $n >= 1 && (int) $n <= 9) ? "Group $n · $hallName · $side" : "Group $n · $side";
    if ($p['label'] !== $expect) { $okLabels = FALSE; echo "  got '{$p['label']}', want '$expect'\n"; }
  }
  ok($okLabels, 'two halls: plan labels name the hall: "Group 1 · Hall Beta · Male"');
  db_update('dh_course')->fields(array('c_seat_plan' => 0))->condition('c_id', COURSE)->execute();
  dh_halls_reset(); drupal_static_reset('dh_course_plan');
  $bm = dcv_build_bundle(COURSE);
  ok(isset($bm['group_halls']) && $bm['group_halls'] === $wantMap, 'main plan: group_halls is sent too (the detail page shows the group)');
  $ml = array_map(function ($p) { return $p['label']; }, dcv_course_plans(COURSE));
  ok(count($ml) > 0 && !array_diff($ml, array('Male', 'Female')), 'main plan: labels stay "Male" / "Female"');
  db_update('dh_course')->fields(array('c_seat_plan' => 1))->condition('c_id', COURSE)->execute();
  dh_halls_reset(); drupal_static_reset('dh_course_plan');
  $bad = 0; $seen = 0;
  foreach (groups_of($b2) as $g) {
    $n = (int) $g['n'];
    if ($n >= 1 && $n <= 9) {
      $seen++;
      $want = ($n === 1) ? array('HB', 'Hall Beta') : array('HA', 'Hall Alpha');
      if (!isset($g['hall_code']) || array($g['hall_code'], $g['hall_name']) !== $want) { $bad++; }
    }
    elseif (array_key_exists('hall_code', $g)) { $bad++; }
  }
  ok($seen > 0 && $bad === 0, 'two halls: every group 1-9 has its hall (group 1 = HB, the others HA); group 0 has none');
  // A centre with no GROUP<n>- seat config lines (like Sindhu, centre 21, or 335): no group entries carry a hall,
  // but group_halls still names every group's hall and the labels still name it.
  $cfg = (string) db_query("select cs_seat_config from dh_center_setting where cs_center = :c", array(':c' => CENTRE))->fetchField();
  $noGroups = preg_replace('/^\s*GROUP\d+-.*$/mi', '', $cfg);
  ok($noGroups !== $cfg, 'fixture: centre 5 has GROUP lines to remove');
  db_update('dh_center_setting')->fields(array('cs_seat_config' => $noGroups))->condition('cs_center', CENTRE)->execute();
  dh_halls_reset();
  $bn = dcv_build_bundle(COURSE);
  ok(count(array_filter(groups_of($bn), function ($g) { return isset($g['hall_code']); })) === 0
    && isset($bn['group_halls']) && $bn['group_halls'] === $wantMap, 'no GROUP lines: no group entry has a hall, but group_halls has all 9');
  $ln = array_map(function ($p) { return $p['label']; }, dcv_course_plans(COURSE));
  ok(in_array('Group 1 · Hall Beta · Male', $ln, TRUE), 'no GROUP lines: the plan labels still name the hall');
  db_update('dh_center_setting')->fields(array('cs_seat_config' => $cfg))->condition('cs_center', CENTRE)->execute();

  // A shared seating plan follows the owner course's halls (the course the other one points to), as Dipi's pages do:
  // the course points to a partner that has its own change "group 2 -> HB".
  $row = db_query("select * from dh_course where c_id = :c", array(':c' => COURSE))->fetchAssoc();
  $row['c_id'] = 9900903; $row['c_combined_seat_course'] = 0;
  db_insert('dh_course')->fields($row)->execute();
  db_update('dh_course')->fields(array('c_combined_seat_course' => 9900903))->condition('c_id', COURSE)->execute();
  db_insert('dh_course_hall_group')->fields(array('cg_course' => 9900903, 'cg_group' => 2, 'cg_hall' => $hb))->execute();
  dh_halls_reset();
  $bs = dcv_build_bundle(COURSE);
  ok(isset($bs['group_halls']['2']) && $bs['group_halls']['2'] === 'HB' && $bs['group_halls']['3'] === 'HA',
    "shared plan: group_halls follows the owner course's change (group 2 -> HB)");
  ok(isset($bs['course']['combined_with']['id']) && $bs['course']['combined_with']['id'] === 9900903, 'shared plan: fixture linked');
  db_update('dh_course')->fields(array('c_combined_seat_course' => 0))->condition('c_id', COURSE)->execute();
  dh_halls_reset();

  $strip = $b2;
  unset($strip['halls'], $strip['group_halls']);
  foreach ($strip['sections'] as $i => $s) {
    foreach ((isset($s['groups']) ? $s['groups'] : array()) as $j => $g) {
      unset($strip['sections'][$i]['groups'][$j]['hall_code'], $strip['sections'][$i]['groups'][$j]['hall_name']);
    }
  }
  ok(plain($strip) === plain($b0), 'two halls: nothing else in the bundle changed');
}
finally {
  $txn->rollback();
}
dh_halls_reset();
ok(!db_query("select 1 from dh_course where c_id = 9900903")->fetchField()
  && !db_query("select 1 from dh_center_hall where ch_center = :c and ch_deleted = 0 and ch_code in ('HA', 'HB')", array(':c' => CENTRE))->fetchField()
  && (int) db_query("select c_seat_plan from dh_course where c_id = :c", array(':c' => COURSE))->fetchField() === 0, 'rolled back');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
