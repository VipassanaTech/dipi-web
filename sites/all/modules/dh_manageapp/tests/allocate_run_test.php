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

// regression: old+new must share the combined pool (no duplicate male seats)
$old_ids = db_query("select a_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and a.a_type='Student' limit 5")->fetchCol();
if (!empty($old_ids)) {
  db_query("update dh_applicant set a_old=1 where a_id in (".implode(',', $old_ids).")");
}
db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant set aa.aa_dining_fixed=0, aa.aa_dining=null where a.a_course=$course");
dh_alloc_run($centre,$course,$desc,'default');
$dup = db_query("select count(*) from (select aa_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and aa_dining is not null group by aa_dining having count(*)>1) t")->fetchField();
ok($dup==0, 'no duplicate male seats with mixed old/new');
if (!empty($old_ids)) {
  db_query("update dh_applicant set a_old=0 where a_id in (".implode(',', $old_ids).")");
}

echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
