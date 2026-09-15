<?php
// sites/all/modules/dh_manageapp/tests/workbench_render_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/workbench');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
// dh_dining_workbench() normally print()s then exit()s like the other page
// callbacks (dh_dining_list et al). Pass $return=TRUE (matching the
// dh_generate_seating_plan/$return_html pattern in zero-day.inc) so it
// returns the HTML string instead of exiting the test process; ob_start()
// is kept as a safety net in case anything stray gets echoed along the way.
ob_start(); $exit=false; $ret = '';
try { $ret = dh_dining_workbench(5,9900002,TRUE); } catch (Exception $e) {}
$buffered = ob_get_clean();
$html = ($ret !== null && $ret !== '') ? $ret : $buffered;
ok(strpos($html,'wb-main')!==false, 'main dining inputs rendered');
ok(strpos($html,'wb-fixed')!==false, 'fixed checkboxes rendered');
ok(substr_count($html,'data-aa=')>50, 'many attendee rows rendered');
ok(strpos($html,'Auto-fill')!==false, 'toolbar present');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
