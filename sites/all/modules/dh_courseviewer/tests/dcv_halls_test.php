<?php
// sites/all/modules/dh_courseviewer/tests/dcv_halls_test.php
//
// Dhamma Halls in the bundle (dh_manageapp inc/halls.inc): at a centre with 2+ halls the bundle gets 'halls'
// (code + name, in order) and each section group 1-9 gets hall_code / hall_name; with 0 or 1 hall the bundle is
// exactly as before. schema_version stays 2 (added fields only; released apps ignore unknown keys).
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
  ok(!array_key_exists('halls', $b0), 'no halls: no halls list');
  ok(count(array_filter(groups_of($b0), function ($g) { return array_key_exists('hall_code', $g); })) === 0, 'no halls: no group has a hall');

  $hall('HA', 'Hall Alpha', 0);
  dh_halls_reset();
  ok(plain(dcv_build_bundle(COURSE)) === plain($b0), 'one hall: the bundle is exactly as before');

  $hb = $hall('HB', 'Hall Beta', 1);
  db_insert('dh_center_hall_group')->fields(array('chg_center' => CENTRE, 'chg_group' => 1, 'chg_hall' => $hb))->execute();
  dh_halls_reset();
  $b2 = dcv_build_bundle(COURSE);
  ok($b2['schema_version'] === 2, 'two halls: schema_version stays 2');
  ok(isset($b2['halls']) && $b2['halls'] === array(array('code' => 'HA', 'name' => 'Hall Alpha'), array('code' => 'HB', 'name' => 'Hall Beta')),
    'two halls: the halls list, in order');
  $keys = array_keys($b2);
  ok(end($keys) === 'halls', 'two halls: halls is the last key (the other keys keep their order)');
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
  $strip = $b2;
  unset($strip['halls']);
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
ok(!db_query("select 1 from dh_center_hall where ch_center = :c and ch_deleted = 0 and ch_code in ('HA', 'HB')", array(':c' => CENTRE))->fetchField()
  && (int) db_query("select c_seat_plan from dh_course where c_id = :c", array(':c' => COURSE))->fetchField() === 0, 'rolled back');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
