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

// --- seat config parser ---
require_once __DIR__ . '/../inc/dcv-bundle.inc';
$ini = "[RIGHT]\nSeatsPerRow = 5\nSeatsPerRowChowky = 1\nSeatDirection = right\nEmptySeats = 2-3\n"
     . "GROUP1-SeatsPerRow = 4\nGROUP1-SeatsPerRowChowky = 1\nGROUP1-SeatDirection = right\nGROUP1-EmptySeats = 2-2\n\n"
     . "[LEFT]\nSeatsPerRow = 4\nSeatDirection = left\nEmptySeats = 3-1\n";
$secs = dcv_parse_seat_config($ini);
ok(count($secs) === 2, 'two sections');
ok($secs[0]['key'] === 'RIGHT' && $secs[0]['gender'] === 'M' && $secs[0]['label'] === 'Male', 'RIGHT=Male');
ok($secs[0]['seats_per_row'] === 5, 'RIGHT SeatsPerRow=5 (int)');
ok($secs[0]['seat_direction'] === 'right', 'RIGHT direction');
ok($secs[0]['empty_seats'] === '2-3', 'RIGHT empty seats');
ok(count($secs[0]['groups']) === 1 && $secs[0]['groups'][0]['n'] === 1, 'one group override');
ok($secs[0]['groups'][0]['seats_per_row'] === 4, 'GROUP1 SeatsPerRow=4');
ok($secs[1]['key'] === 'LEFT' && $secs[1]['gender'] === 'F', 'LEFT=Female');
ok($secs[1]['seats_per_row'] === 4, 'LEFT SeatsPerRow=4');
// missing section is simply absent
$only = dcv_parse_seat_config("[RIGHT]\nSeatsPerRow = 3\n");
ok(count($only) === 1 && $only[0]['key'] === 'RIGHT', 'single section ok');

// --- centre access scoping (pure) ---
ok(dcv_centres_allow(array('*'), 5) === true,  'all-centres bypass allows any course');
ok(dcv_centres_allow(array(), 5) === false,    'no centres denies');
ok(dcv_centres_allow(array(5, 7), 5) === true, 'matching centre allowed');
ok(dcv_centres_allow(array(5, 7), 9) === false,'foreign centre denied');
ok(dcv_centres_allow(array(5, 7), false) === false, 'missing course (false centre) denied');
ok(dcv_centres_allow(array(5, 7), null) === false,  'null centre denied');
ok(dcv_centres_allow(array(5, 7), '5') === true, 'string centre id from DB coerces and matches');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
