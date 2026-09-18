<?php
// sites/all/modules/dh_courseviewer/tests/dcv_pure_test.php
// Pure-function tests — no Drupal bootstrap. Run: php7.4 <thisfile>
require_once __DIR__ . '/../inc/dcv-api.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

$e = dcv_envelope('OK', array('pong' => true), 'hi');
ok($e['status'] === 'OK', 'envelope status');
ok($e['msg'] === 'hi', 'envelope msg');
ok($e['data']['pong'] === true, 'envelope data');
$d = dcv_envelope('FAIL');
ok($d['status'] === 'FAIL' && $d['data'] === array() && $d['msg'] === '', 'envelope defaults');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
