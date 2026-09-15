<?php
// sites/all/modules/dh_manageapp/tests/workbench_save_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$aa = db_query("select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=9900002 limit 1")->fetchField();

// seed a known group value so we can prove saving MAIN doesn't clobber it
db_update('dh_applicant_attended')->fields(array('aa_group_dining'=>'KEEP-9'))->condition('aa_id',$aa)->execute();

// save the MAIN plan
$n = _dh_workbench_write_rows(5,9900002, array(array('aa'=>$aa,'val'=>'WB-7','df'=>1)), 'main');
ok($n===1, 'one row written (main)');
$r = db_query("select aa_dining,aa_group_dining,aa_dining_fixed from dh_applicant_attended where aa_id=$aa")->fetchObject();
ok($r->aa_dining==='WB-7' && $r->aa_dining_fixed==1, 'main value + fixed persisted');
ok($r->aa_group_dining==='KEEP-9', 'group column left untouched when saving main');

// save the GROUP plan
$n2 = _dh_workbench_write_rows(5,9900002, array(array('aa'=>$aa,'val'=>'GRP-4','df'=>0)), 'group');
ok($n2===1, 'one row written (group)');
$r2 = db_query("select aa_dining,aa_group_dining from dh_applicant_attended where aa_id=$aa")->fetchObject();
ok($r2->aa_group_dining==='GRP-4', 'group value persisted (group plan)');
ok($r2->aa_dining==='WB-7', 'main column left untouched when saving group');

// a foreign aa_id (different course) must be rejected
$foreign = db_query("select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course<>9900002 limit 1")->fetchField();
$n3 = _dh_workbench_write_rows(5,9900002, array(array('aa'=>$foreign,'val'=>'X','df'=>0)), 'main');
ok($n3===0, 'foreign course row rejected');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
