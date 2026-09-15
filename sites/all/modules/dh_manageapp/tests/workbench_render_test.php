<?php
// sites/all/modules/dh_manageapp/tests/workbench_render_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
// dh_dining_workbench() normally print()s then exit()s; $return=TRUE returns the
// HTML string instead (ob_start kept as a safety net for stray echoes).
ob_start(); $ret = '';
try { $ret = dh_dining_workbench(5,9900002,TRUE); } catch (Exception $e) {}
$buffered = ob_get_clean();
$html = ($ret !== null && $ret !== '') ? $ret : $buffered;
ok(strpos($html,'wb-val')!==false, 'single dining input column rendered');
ok(strpos($html,'wb-fixed')!==false, 'fixed checkboxes rendered');
ok(strpos($html,'class="wb-main"')===false && strpos($html,'class="wb-group"')===false, 'no separate main/group input columns (single-plan view)');
ok(substr_count($html,'data-aa=')>50, 'many attendee rows rendered');
ok(strpos($html,'Auto-fill')!==false, 'toolbar present');
ok(strpos($html,'Plan:')!==false && strpos($html,'Group-wise')!==false, 'plan switch present');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
