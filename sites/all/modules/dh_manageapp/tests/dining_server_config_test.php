<?php
// sites/all/modules/dh_manageapp/tests/dining_server_config_test.php
// The Dining Configuration INI is now assembled client-side (js/dining-config.js);
// this test checks the ENGINE consumes the server sections that editor produces.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/allocate');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }

// An INI shaped exactly like dining-config.js writes it: student defaults +
// a Male server default + a Female server group-2 override.
$cfg = "[MALE]\nCells = 1-100\n\n[FEMALE]\nCells = 1-80\n\n[MALE_SERVER]\nCells = S1-S20\n\n[FEMALE_SERVER_2]\nCells = FS1-FS5\n";
$ini = parse_ini_string($cfg, true, INI_SCANNER_RAW);

ok(isset($ini['MALE']) && isset($ini['FEMALE']), 'student defaults present');
ok(dh_alloc_has_server_config($ini)===true, 'engine sees server config');
// server resolves to its own section (default + group override)
ok(dh_alloc_effective_key($ini,'M',0,'server','main')==='MALE_SERVER', 'male server -> MALE_SERVER');
ok(dh_alloc_effective_key($ini,'F',2,'server','group')==='FEMALE_SERVER_2', 'female group2 server -> FEMALE_SERVER_2');
// no female server default section -> server resolves to null (not seated)
ok(dh_alloc_effective_key($ini,'F',0,'server','main')===null, 'no FEMALE_SERVER -> null (not auto-seated)');
// pools expand as expected
$p = dh_alloc_pool($ini,'MALE_SERVER');
ok($p['cells']===array('S1','S2','S3','S4','S5','S6','S7','S8','S9','S10','S11','S12','S13','S14','S15','S16','S17','S18','S19','S20'), 'MALE_SERVER pool expands S1..S20');

// a config with no server section at all
$ini2 = parse_ini_string("[MALE]\nCells = 1-10\n", true, INI_SCANNER_RAW);
ok(dh_alloc_has_server_config($ini2)===false, 'no server config detected');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
