<?php
// sites/all/modules/dh_manageapp/tests/allocate_batch_test.php
require_once '/dhamma/web/dipinew/sites/all/modules/dh_manageapp/inc/allocate.inc';
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }
// 5 people, pool of 2, shareable -> seats cycle, batch increments every 2.
$pool = array('1','2');
$people = array('a','b','c','d','e');
$res = dh_alloc_assign_batches($people, $pool);
// expected: a->(1,b0) b->(2,b0) c->(1,b1) d->(2,b1) e->(1,b2)
ok($res['a']===array('1',0), 'a seat1 batch0');
ok($res['c']===array('1',1), 'c seat1 batch1');
ok($res['e']===array('1',2), 'e seat1 batch2');
// empty pool -> everyone blank batch0
$res2 = dh_alloc_assign_batches(array('x','y'), array());
ok($res2['x']===array('',0) && $res2['y']===array('',0), 'empty pool -> blanks');
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
