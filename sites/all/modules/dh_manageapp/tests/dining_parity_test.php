<?php
// sites/all/modules/dh_manageapp/tests/dining_parity_test.php
// Behavioral smoke test of the dining wrappers (generate_dining_list /
// generate_group_dining_list) on the local fixture (centre 5 / course
// 9900002). The old v1 allocator is gone, so a wrapper-vs-engine identity
// check is no longer meaningful (the wrappers ARE dh_alloc_run now); the
// real behavioral guard for the engine itself lives in allocate_run_test.php.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$centre=5; $course=9900002;

// known-good config so both plans have a real pool to draw from
$cfg="[MALE]\nCells = 1-200\nReserved = \n\n[FEMALE]\nCells = 1-150\nReserved = \n";
db_update('dh_center_setting')->fields(array('cs_has_dining'=>1,'cs_dining_config'=>$cfg))->condition('cs_center',$centre)->execute();
db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant set aa.aa_dining_fixed=0, aa.aa_dining=null, aa.aa_group_dining=null where a.a_course=$course");

// default plan: generate_dining_list() -> aa_dining
$n = generate_dining_list($centre, $course);
ok($n>0, "generate_dining_list wrote $n rows");
$male_null = db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and a.a_type='Student' and aa.aa_dining is null")->fetchField();
ok($male_null==0, 'male students have non-null aa_dining');
$dupe = db_query("select count(*) from (select aa_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and aa_dining is not null group by aa_dining having count(*)>1) t")->fetchField();
ok($dupe==0, 'no duplicate aa_dining among males');

// group plan: generate_group_dining_list() -> aa_group_dining
$ng = generate_group_dining_list($centre, $course);
ok($ng>0, "generate_group_dining_list wrote $ng rows");
$populated = db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and aa.aa_group_dining is not null")->fetchField();
ok($populated>0, 'aa_group_dining populated for at least one attendee');

echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
