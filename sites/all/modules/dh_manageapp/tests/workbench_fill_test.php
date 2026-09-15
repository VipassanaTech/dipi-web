<?php
// sites/all/modules/dh_manageapp/tests/workbench_fill_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
// clear, then simulate ?fill=main and confirm the engine populated aa_dining
db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant set aa.aa_dining=null, aa.aa_dining_fixed=0 where a.a_course=9900002");
$_GET['fill']='main'; $_REQUEST['fill']='main';
ob_start(); dh_dining_workbench(5,9900002,TRUE); ob_end_clean();
$filled = db_query("select count(*) from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=9900002 and a.a_gender='M' and aa.aa_dining is not null")->fetchField();
ok($filled>0, 'fill=main ran the engine and populated aa_dining');
unset($_GET['fill'],$_REQUEST['fill']);
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
