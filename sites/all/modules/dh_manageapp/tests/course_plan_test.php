<?php
// sites/all/modules/dh_manageapp/tests/course_plan_test.php
// Round-trip + defaults for the per-course plan flag (c_<res>_plan) and the
// aa_group_seat cache column that seat-in-reports relies on.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/allocate');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }

// A real course to poke (restore its flags afterwards).
$co = db_query("select c_id from dh_course order by c_id desc limit 1")->fetchField();
ok($co > 0, "found a course to test ($co)");

$saved = array();
foreach (array('cell','dining','seat') as $res) $saved[$res] = dh_course_plan($co, $res);

// unknown resource -> main (safe default)
ok(dh_course_plan($co, 'bogus') === 'main', "unknown resource defaults to main");

foreach (array('cell','dining','seat') as $res) {
  dh_course_plan_set($co, $res, 'group');
  ok(dh_course_plan($co, $res) === 'group', "$res set to group persists");
  dh_course_plan_set($co, $res, 'main');
  ok(dh_course_plan($co, $res) === 'main', "$res set back to main persists");
}

// schema the reports/seating rely on
ok((bool) db_query("show columns from dh_course like 'c_cell_plan'")->fetchField(), "dh_course.c_cell_plan exists");
ok((bool) db_query("show columns from dh_course like 'c_dining_plan'")->fetchField(), "dh_course.c_dining_plan exists");
ok((bool) db_query("show columns from dh_course like 'c_seat_plan'")->fetchField(), "dh_course.c_seat_plan exists");
ok((bool) db_query("show columns from dh_applicant_attended like 'aa_group_seat'")->fetchField(), "dh_applicant_attended.aa_group_seat exists");

// restore
foreach (array('cell','dining','seat') as $res) dh_course_plan_set($co, $res, $saved[$res]);
ok(dh_course_plan($co,'cell') === $saved['cell'], "original cell plan restored");

echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
