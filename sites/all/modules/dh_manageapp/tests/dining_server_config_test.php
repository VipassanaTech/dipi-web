<?php
// sites/all/modules/dh_manageapp/tests/dining_server_config_test.php
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/centre');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
$in = array(
  'diningcfg_male_cells'=>'1-100','diningcfg_female_cells'=>'1-80',
  'diningcfg_male_server_cells'=>'S1-S20','diningcfg_male_server_reserved'=>'',
  'diningcfg_female_server_g2_cells'=>'FS1-FS5', // a server group override
);
$ini = parse_ini_string(_dh_ma_diningcfg_ini($in), true, INI_SCANNER_RAW);
ok(isset($ini['MALE']) && isset($ini['FEMALE']), 'student defaults present');
ok(isset($ini['MALE_SERVER']) && $ini['MALE_SERVER']['Cells']==='S1-S20', 'server default written');
ok(isset($ini['FEMALE_SERVER_2']) && $ini['FEMALE_SERVER_2']['Cells']==='FS1-FS5', 'server group override written');
ok(!isset($ini['FEMALE_SERVER']), 'empty server default NOT written');
ok(dh_alloc_has_server_config($ini)===true, 'engine sees server config');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
