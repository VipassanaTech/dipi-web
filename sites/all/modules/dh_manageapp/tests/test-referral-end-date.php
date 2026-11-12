<?php
// A referral is valid through its whole End Date (end date today or later), the same rule as the
// centre referral Active tab (r_end >= curdate()). Checked on the search page flag, the AT Portal
// review form and the Dipi App bundle, which used to compare with the time of day and so hid the
// referral on its last day. "Today" is the database date, whatever the user's own time zone.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-end-date.php
require __DIR__ . '/lib.php';

$today = db_query("select curdate()")->fetchField();
$yesterday = db_query("select curdate() - interval 1 day")->fetchField();
$tomorrow = db_query("select curdate() + interval 1 day")->fetchField();

// H: the shared rule
t_ok(function_exists('dh_referral_end_ok'), 'H1 dh_referral_end_ok exists');
if (function_exists('dh_referral_end_ok')) {
  t_eq(array(dh_referral_end_ok($today), dh_referral_end_ok($tomorrow), dh_referral_end_ok($yesterday), dh_referral_end_ok('')),
    array(TRUE, TRUE, FALSE, TRUE), 'H2 end today / tomorrow valid, yesterday not, no end date valid');
  $tz = date_default_timezone_get();
  foreach (array('Pacific/Kiritimati', 'Pacific/Pago_Pago') as $far) {   // UTC+14 and UTC-11
    date_default_timezone_set($far); drupal_static_reset('dh_referral_end_ok');
    t_eq(array(dh_referral_end_ok($today), dh_referral_end_ok($yesterday)), array(TRUE, FALSE), "H3 same answer for a user in $far");
  }
  date_default_timezone_set($tz); drupal_static_reset('dh_referral_end_ok');
}

t_in_txn(function () use ($today, $yesterday, $tomorrow) {
  $course = t_make_course(array('c_start' => date('Y-m-d', strtotime('+10 days')), 'c_end' => date('Y-m-d', strtotime('+20 days'))));
  $centre = (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $course))->fetchField();
  $type_key = db_query("select td.td_key from dh_course c join dh_type_detail td on td.td_id = c.c_course_type where c.c_id = :c", array(':c' => $course))->fetchField();
  $mk = function ($end, $centre_of_ref) {
    static $n = 0; $n++;
    $s = t_make_student(array('s_l_name' => "Refend$n"));
    return (int) db_insert('dh_referral')->fields(array('r_student' => $s, 'r_center' => $centre_of_ref, 'r_reason' => "zzq-end-reason-$n",
      'r_listed_by' => 'zzq', 'r_start' => date('Y-m-d', strtotime('-60 days')), 'r_end' => $end, 'r_listed_10d' => 1, 'r_listed_stp' => 1,
      'r_listed_spl' => 1, 'r_listed_20d' => 1, 'r_listed_30d' => 1, 'r_listed_45d' => 1, 'r_listed_60d' => 1, 'r_listed_tsc' => 1,
      'r_listed_10d_seva' => 1, 'r_listed_lc_seva' => 1, 'r_deleted' => 0, 'r_created_by' => 0, 'r_updated_by' => 0))->execute();
  };

  // S: search page flag + referral details
  foreach (array('today' => $today, 'tomorrow' => $tomorrow, 'yesterday' => $yesterday) as $label => $end) {
    $ref = $mk($end, $centre);
    $app = t_make_applicant($course, array('a_referral' => $ref));
    drupal_static_reset('drupal_add_js');
    $out = dh_manageapp_search_results('applicant', "a_id = $app", $centre);
    $inline = ''; foreach (drupal_add_js() as $item) if (isset($item['type']) && $item['type'] == 'inline') $inline .= $item['data'];
    $want = $label != 'yesterday';
    t_eq(strpos($inline, "ref=$ref") !== FALSE, $want, "S1 search: Referral-List flag, end date $label");
    t_eq(strpos($inline, "zzq-end-reason") !== FALSE, $want, "S2 search: referral details, end date $label");
  }

  // D: Dipi App bundle
  foreach (array('today' => $today, 'tomorrow' => $tomorrow, 'yesterday' => $yesterday) as $label => $end) {
    $ref = $mk($end, $centre);
    t_eq(dcv_student_referral($ref, $type_key, 'Student') !== NULL, $label != 'yesterday', "D1 Dipi App bundle: referral, end date $label");
  }

  // A: AT Portal review form (global referrals only)
  $GLOBALS['user'] = user_load(1);
  foreach (array('today' => $today, 'yesterday' => $yesterday) as $label => $end) {
    $ref = $mk($end, 0);
    $app = t_make_applicant($course, array('a_referral' => $ref));
    $form_state = array('build_info' => array('args' => array($app)));
    $form = drupal_retrieve_form('dh_atportal_review_form', $form_state);
    t_eq(strpos(serialize($form), 'zzq-end-reason') !== FALSE, $label != 'yesterday', "A1 AT review form: referral details, end date $label");
  }
});

t_done();
