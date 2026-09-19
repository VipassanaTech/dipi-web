<?php
// sites/all/modules/dh_courseviewer/tests/dcv_bundle_test.php
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-bundle.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

ok(dcv_build_bundle(999999999) === null, 'missing course => null');

$b = dcv_build_bundle(9900002);
ok(is_array($b), 'bundle is array');
ok($b['schema_version'] === 1, 'schema_version');
ok($b['course']['id'] === 9900002, 'course id');
ok($b['course']['centre'] === 5, 'course centre');
ok($b['course']['start_date'] === '2026-09-30', 'start date');
ok($b['course']['end_date'] === '2026-10-03', 'end date');
ok($b['course']['seat_plan'] === 'main', 'seat plan main');
ok($b['expires_after'] === '2026-10-04', 'expiry = end + 1 day');
ok(is_array($b['sections']) && count($b['sections']) >= 1, 'has sections');
ok(count($b['seats']) === count(dcv_course_seats(9900002)), 'seats match');
ok(count($b['students']) === 320, 'students match');
ok(isset($b['generated_at']) && strlen($b['generated_at']) > 0, 'generated_at set');
// JSON-encodable
ok(json_encode($b) !== false, 'bundle json-encodes');
$je = json_encode($b['students']); ok(is_string($je) && $je[0] === '{', 'students encodes as JSON object');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
