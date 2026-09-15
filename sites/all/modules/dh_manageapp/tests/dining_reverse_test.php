<?php
// sites/all/modules/dh_manageapp/tests/dining_reverse_test.php
// Reverse-range support: a "<Key>Rev = 1" flag reverses that range's expanded
// order in the engine, and the centre form's *_rev checkboxes serialise to it.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/allocate');
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

// --- the INI shape the editor writes is honoured end-to-end by dh_alloc_pool ---
// (dining-config.js assembles this exact INI; no PHP serialiser anymore.)
$cfg = "[MALE]\nCells = 1-100\nCellsRev = 1\n\n[FEMALE]\nCells = 1-50\n\n[MALE_SERVER]\nCells = 200-210\nCellsRev = 1\n";
$ini = parse_ini_string($cfg, TRUE, INI_SCANNER_RAW);
$pm = dh_alloc_pool($ini,'MALE');
ok($pm['cells'][0]==='100' && end($pm['cells'])==='1', 'MALE CellsRev -> pool starts at 100');
$pf = dh_alloc_pool($ini,'FEMALE');
ok($pf['cells'][0]==='1', 'FEMALE (no rev) -> pool starts at 1');
$ps = dh_alloc_pool($ini,'MALE_SERVER');
ok($ps['cells'][0]==='210' && end($ps['cells'])==='200', 'MALE_SERVER CellsRev -> pool 210..200');

echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
