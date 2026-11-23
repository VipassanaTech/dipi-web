<?php
// Renders ONE DIPI page for the Dhamma Halls tests, in its own PHP process (the teacher list, chit and slip pages
// print the page and then exit). The hall setup is made inside a transaction that is ALWAYS rolled back, then the
// page HTML is printed. Local only. Usage, from the Drupal root:
//   php7.4 sites/all/modules/dh_manageapp/tests/halls-page-runner.php '{"page":"checking-slip","course":65802,"halls":[["HA","Hall Alpha"],["HB","Hall Beta"]],"usual":{"2":"HB"}}'
// page: teacher-list | student-chit | checking-slip | group-seating. halls [] (or left out) = the centre has no halls.
if (php_sapi_name() !== 'cli') { exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), 'vri') !== FALSE) { echo "Refusing: local only.\n"; exit(1); }
error_reporting(E_ERROR | E_PARSE);   // notices carry line numbers, which change when code moves: keep them out of the page
$in = json_decode(isset($argv[1]) ? $argv[1] : '', TRUE);
$course = (int) (isset($in['course']) ? $in['course'] : 0);
$centre = (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $course))->fetchField();
if (!$centre) { echo "No such course.\n"; exit(1); }
$GLOBALS['user'] = user_load(1);
$GLOBALS['hr_txn'] = db_transaction();
// The page prints and exits: take the page out of the buffers, undo every change, then print the page.
ob_start();
register_shutdown_function(function () {
  $html = '';
  while (ob_get_level() > 0) { $html = ob_get_clean() . $html; }
  $GLOBALS['hr_txn']->rollback();
  echo $html;
});

// Halls for this run only (rolled back above).
db_update('dh_center_hall')->fields(array('ch_deleted' => 1))->condition('ch_center', $centre)->execute();
db_delete('dh_center_hall_group')->condition('chg_center', $centre)->execute();
db_delete('dh_course_hall_group')->condition('cg_course', dh_seat_plan_courses($course), 'IN')->execute();
$ids = array();
$now = date('Y-m-d H:i:s');
foreach ((isset($in['halls']) ? $in['halls'] : array()) as $w => $h)
  $ids[strtoupper($h[0])] = (int) db_insert('dh_center_hall')->fields(array('ch_center' => $centre, 'ch_code' => $h[0], 'ch_name' => $h[1],
    'ch_weight' => (int) $w, 'ch_deleted' => 0, 'ch_created' => $now, 'ch_created_by' => 0, 'ch_updated' => $now, 'ch_updated_by' => 0))->execute();
foreach ((isset($in['usual']) ? $in['usual'] : array()) as $g => $code)
  db_insert('dh_center_hall_group')->fields(array('chg_center' => $centre, 'chg_group' => (int) $g, 'chg_hall' => $ids[strtoupper($code)]))->execute();
dh_halls_reset();

$other = (int) db_query("select c_combined_seat_course from dh_course where c_id = :c", array(':c' => $course))->fetchField();
$both = $other > 0 ? array($course, $other) : $course;   // as the group-seating route passes it
$page = isset($in['page']) ? (string) $in['page'] : '';
$_GET['q'] = "$page/$centre/$course";
switch ($page) {
  case 'teacher-list': dh_generate_teacher_list($centre, $course, 0); break;   // prints and exits
  case 'student-chit': dh_student_chit($centre, $course, 0); break;            // prints and exits
  case 'checking-slip': dh_checking_slip($centre, $course, 0); break;          // prints and exits
  case 'group-seating': dh_auto_allocate_group_seats($centre, $both, FALSE); echo dh_generate_group_seating_plan($centre, $both, '', TRUE); break;
  default: echo "Unknown page.\n";
}
exit;
