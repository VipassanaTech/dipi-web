<?php
// sites/all/modules/dh_manageapp/tests/dining_parity_test.php
// Proves the engine reproduces the old allocator's output for dining.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$centre=5; $course=9900002;
// snapshot after new wrappers run
generate_dining_list($centre,$course);
generate_group_dining_list($centre,$course);
$after = db_query("select aa_id, aa_dining, aa_group_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course order by aa_id")->fetchAllKeyed(0,1);
ok(count($after)>0, 'wrappers ran and wrote rows');
// engine-direct produces identical result
$d = dh_alloc_descriptor('dining');
dh_alloc_run($centre,$course,$d,'default'); dh_alloc_run($centre,$course,$d,'group');
$direct = db_query("select aa_id, aa_dining from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course order by aa_id")->fetchAllKeyed(0,1);
ok($after==$direct, 'wrapper output == engine output');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
