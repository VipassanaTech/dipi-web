<?php
// sites/all/modules/dh_manageapp/tests/dining_reverse_test.php
// Reverse-range support: a "<Key>Rev = 1" flag reverses that range's expanded
// order in the engine, and the centre form's *_rev checkboxes serialise to it.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/allocate');
module_load_include('inc','dh_manageapp','inc/centre');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }

// --- engine: dh_alloc_pool reverses when the flag is set ---
$p = dh_alloc_pool(array('MALE'=>array('Cells'=>'1-5','CellsRev'=>'1')), 'MALE');
ok($p['cells'] === array('5','4','3','2','1'), 'CellsRev reverses combined pool (1-5 -> 5..1)');

$p2 = dh_alloc_pool(array('MALE'=>array('Cells'=>'1-5')), 'MALE');
ok($p2['cells'] === array('1','2','3','4','5'), 'no flag -> natural order');

$p3 = dh_alloc_pool(array('MALE'=>array('Old'=>'1-3','OldRev'=>'1','New'=>'10-12')), 'MALE');
ok($p3['old'] === array('3','2','1'), 'OldRev reverses old pool');
ok($p3['new'] === array('10','11','12'), 'New without flag stays natural');

// multi-part range reverses the whole expanded list
$p4 = dh_alloc_pool(array('MALE'=>array('Cells'=>'1-3, 20','CellsRev'=>'1')), 'MALE');
ok($p4['cells'] === array('20','3','2','1'), 'multi-part "1-3, 20" reversed -> 20,3,2,1');

// --- serializer: *_rev checkboxes write the flags ---
$ini = _dh_ma_diningcfg_ini(array(
  'diningcfg_male_cells' => '1-100', 'diningcfg_male_cells_rev' => '1',
  'diningcfg_female_cells' => '1-50',   // no rev
  'diningcfg_male_server_cells' => '200-210', 'diningcfg_male_server_cells_rev' => '1',
));
ok(strpos($ini, "[MALE]") !== false && strpos($ini, "CellsRev = 1") !== false, 'male CellsRev serialised');
$parsed = parse_ini_string($ini, TRUE, INI_SCANNER_RAW);
ok(isset($parsed['MALE']['CellsRev']) && $parsed['MALE']['CellsRev']=='1', 'MALE CellsRev present');
ok(!isset($parsed['FEMALE']['CellsRev']), 'FEMALE has no rev flag (unchecked)');
ok(isset($parsed['MALE_SERVER']['Cells']) && $parsed['MALE_SERVER']['Cells']==='200-210', 'server range serialised from card');
ok(isset($parsed['MALE_SERVER']['CellsRev']) && $parsed['MALE_SERVER']['CellsRev']=='1', 'server CellsRev serialised');

// old/new split inferred from data (no checkbox), with per-side rev
$ini2 = _dh_ma_diningcfg_ini(array(
  'diningcfg_male_new' => '1-10', 'diningcfg_male_old' => '11-20', 'diningcfg_male_old_rev' => '1',
));
$p5 = parse_ini_string($ini2, TRUE, INI_SCANNER_RAW);
ok(isset($p5['MALE']['Old']) && isset($p5['MALE']['New']) && !isset($p5['MALE']['Cells']), 'split inferred from New/Old (Cells omitted)');
ok(isset($p5['MALE']['OldRev']) && !isset($p5['MALE']['NewRev']), 'OldRev written, NewRev not');

echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
