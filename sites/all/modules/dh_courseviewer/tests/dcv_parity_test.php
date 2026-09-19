<?php
// sites/all/modules/dh_courseviewer/tests/dcv_parity_test.php
// Confirms the DCV bundle's occupied seats == Dipi's own seating data.
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-bundle.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

$course = 9900002;
$plan = dh_course_plan($course, 'seat'); // 'main' here
$col_r = $plan === 'group' ? 'aa_group_seat_row' : 'aa_seat_row';
$col_c = $plan === 'group' ? 'aa_group_seat_col' : 'aa_seat_col';
$col_l = $plan === 'group' ? 'aa_group_seat'     : 'aa_seat';

// Live truth: Dipi's own occupied-seat rows for the course.
$truth = array();
$sql = "select a_id, a_gender, $col_r as r, $col_c as c, $col_l as l
        from dh_applicant
          left join dh_applicant_attended on a_id = aa_applicant
        where a_course = :c and a_attended = 1 and a_type = 'Student'
          and $col_r is not null and $col_c is not null";
foreach (db_query($sql, array(':c' => $course)) as $row) {
  $truth[(int) $row->a_id] = array(
    'section' => ($row->a_gender === 'M') ? 'RIGHT' : 'LEFT',
    'row' => (int) $row->r, 'col' => (int) $row->c, 'label' => (string) $row->l,
  );
}

ok(count($truth) > 0, 'fixture has occupied seats (truth non-empty)');

$seats = dcv_course_seats($course);
ok(count($seats) === count($truth), 'seat count matches live plan (' . count($truth) . ')');

$mismatch = 0;
foreach ($seats as $s) {
  $id = $s['student_id'];
  if (!isset($truth[$id])) { $mismatch++; continue; }
  $t = $truth[$id];
  if ($s['section'] !== $t['section'] || $s['row'] !== $t['row']
      || $s['col'] !== $t['col'] || $s['label'] !== $t['label']) {
    $mismatch++;
  }
}
ok($mismatch === 0, "all seat positions/labels match live plan (mismatches: $mismatch)");

// No two students share the same section+row+col.
$occ = array();
$dupes = 0;
foreach ($seats as $s) {
  $k = $s['section'] . ':' . $s['row'] . ':' . $s['col'];
  if (isset($occ[$k])) { $dupes++; } $occ[$k] = true;
}
ok($dupes === 0, "no duplicate seat coordinates (dupes: $dupes)");

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
