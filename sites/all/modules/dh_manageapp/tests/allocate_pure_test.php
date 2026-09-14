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

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
