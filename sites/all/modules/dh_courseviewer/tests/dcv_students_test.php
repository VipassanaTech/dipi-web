<?php
// sites/all/modules/dh_courseviewer/tests/dcv_students_test.php
define('DRUPAL_ROOT', '/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/dcv-bundle.inc';
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

$students = dcv_course_students(9900002);
ok(is_array($students), 'students is array');
ok(count($students) === 320, 'fixture has 320 attended students');
$id = (string) array_key_first($students);
$s = $students[$id];
// grouped structure present
foreach (array('identity','ids','contact','languages','course','meditation','health','seating','notes','family','lc') as $g) {
    ok(is_array($s[$g]) || is_object($s[$g]) || array_key_exists($g,$s), "group present: $g");
}
// representative fields from each group (from dh_application_view's SELECT)
ok(array_key_exists('name', $s['identity']) && array_key_exists('gender', $s['identity']) && array_key_exists('old_student', $s['identity']), 'identity core');
ok(array_key_exists('aadhar', $s['ids']) && array_key_exists('passport', $s['ids']), 'ids present (full view, per decision B)');
ok(array_key_exists('email', $s['contact']) && array_key_exists('city', $s['contact']) && array_key_exists('emergency_name', $s['contact']), 'contact core');
ok(array_key_exists('lang_discourse', $s['languages']), 'languages.lang_discourse');
ok(array_key_exists('short_courses', $s['meditation']) && array_key_exists('first_course', $s['meditation']), 'meditation core');
ok(array_key_exists('physical', $s['health']) && array_key_exists('medication', $s['health']) && array_key_exists('pregnant', $s['health']), 'health core');
ok(array_key_exists('nationality', $s['identity']), 'nationality under identity');
ok(array_key_exists('id_issued', $s['ids']) && array_key_exists('id_issued_by', $s['ids']), 'id_issued under ids');
ok(array_key_exists('father', $s['family']) && array_key_exists('parent_course', $s['family']), 'family group has parent/teen fields');
ok(!array_key_exists('nationality', $s['health']) && !array_key_exists('id_issued', $s['health']) && !array_key_exists('father', $s['health']), 'moved fields no longer under health');
ok(array_key_exists('room', $s['seating']) && array_key_exists('dining', $s['seating']) && array_key_exists('section', $s['seating']), 'seating core (incl dining)');
ok(array_key_exists('committed', $s['lc']) && array_key_exists('special_req', $s['lc']), 'lc block present');
// wire contract: encodes as a JSON object keyed by numeric id
$je = json_encode($students);
ok(is_string($je) && $je[0] === '{', 'students encodes as a JSON object');
ok(ctype_digit($id), 'keyed by numeric student id');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
