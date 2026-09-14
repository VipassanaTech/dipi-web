<?php
// sites/all/modules/dh_manageapp/tests/allocate_pure_test.php
// Pure-function tests — no Drupal bootstrap needed.
require_once __DIR__ . '/../inc/allocate.inc';
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c){$GLOBALS['fail']=1;} }

// descriptor
$d = dh_alloc_descriptor('dining');
ok($d['main_col']==='aa_dining' && $d['group_col']==='aa_group_dining' && $d['shareable']===false, 'dining descriptor');
$c = dh_alloc_descriptor('cell');
ok($c['main_col']==='aa_cell' && $c['batch_col']==='aa_cell_group' && $c['shareable']===true, 'cell descriptor');

// expand: single, comma, hyphen range, letter-prefixed range, blanks
ok(dh_alloc_expand('') === array(), 'expand empty');
ok(dh_alloc_expand('5') === array('5'), 'expand single');
ok(dh_alloc_expand('1-3, 5') === array('1','2','3','5'), 'expand mixed');
ok(dh_alloc_expand('A1-A3') === array('A1','A2','A3'), 'expand letter range');
ok(dh_alloc_expand('1, , 2') === array('1','2'), 'expand skips blanks');

// Task 2 tests - pool
$ini = array(
  'MALE'   => array('Cells'=>'1-4','Reserved'=>'2'),
  'FEMALE' => array('Old'=>'1-2','New'=>'3-4','Reserved'=>''),
);
$pm = dh_alloc_pool($ini,'MALE');
ok($pm['sep']===false && $pm['cells']===array('1','2','3','4') && $pm['reserved']===array('2'), 'pool combined');
$pf = dh_alloc_pool($ini,'FEMALE');
ok($pf['sep']===true && $pf['old']===array('1','2') && $pf['new']===array('3','4'), 'pool split');
$pmiss = dh_alloc_pool($ini,'MALE_1');
ok($pmiss['cells']===array() && $pmiss['old']===array() && $pmiss['reserved']===array(), 'pool missing section');

// Task 3 tests - effective key and server config
$ini2 = array('MALE'=>array('Cells'=>'1'),'MALE_1'=>array('Cells'=>'D1'),'MALE_SERVER'=>array('Cells'=>'S1'));
// student, default -> MALE ; group with override -> MALE_1 ; group w/o override -> MALE
ok(dh_alloc_effective_key($ini2,'M',0,'student','default')==='MALE', 'student default');
ok(dh_alloc_effective_key($ini2,'M',1,'student','group')==='MALE_1', 'student group override');
ok(dh_alloc_effective_key($ini2,'M',2,'student','group')==='MALE', 'student group fallback');
// server -> MALE_SERVER ; female server absent -> null
ok(dh_alloc_effective_key($ini2,'M',0,'server','default')==='MALE_SERVER', 'server default');
ok(dh_alloc_effective_key($ini2,'F',0,'server','default')===null, 'server absent -> null');
ok(dh_alloc_has_server_config($ini2)===true, 'has server config');
ok(dh_alloc_has_server_config(array('MALE'=>array()))===false, 'no server config');

// Task 6 tests - validation
$bad = array('MALE'=>array('Cells'=>'1-3','Reserved'=>'9'), 'FEMALE'=>array('Cells'=>'1,1,2','Reserved'=>''));
$w = dh_alloc_validate($bad);
ok(count(array_filter($w, function($s){return strpos($s,'MALE')!==false && stripos($s,'reserved')!==false;}))>0, 'flags out-of-range reserved');
ok(count(array_filter($w, function($s){return strpos($s,'FEMALE')!==false && stripos($s,'duplicate')!==false;}))>0, 'flags duplicate');
ok(dh_alloc_validate(array('MALE'=>array('Cells'=>'1-3','Reserved'=>'2')))===array(), 'clean config no warnings');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
