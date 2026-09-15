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
