<?php
// [ReviewLink] (inc/letter-protect.inc, letters.inc dh_get_letter, letter-check.inc, Manage Fields guard, admin settings).
// Made-up link starts only (no real address). Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-reviewlink.php
require __DIR__ . '/lib.php';
include_once 'sites/all/libraries/editor/php/DataTables.php';   // $db for the Manage Fields check

$GLOBALS['conf']['dh_lc_review_link_r'] = 'https://zzqtest.invalid/r/';
$GLOBALS['conf']['dh_lc_review_link_a'] = 'https://zzqtest.invalid/a/';
t_eq(dh_letter_review_link('r', 'CODE1'), 'https://zzqtest.invalid/r/CODE1', 'L1 recommending link = setting + code');
t_eq(dh_letter_review_link('a', 'CODE2'), 'https://zzqtest.invalid/a/CODE2', 'L1 area link = setting + code');

// The merge-field check
$p = dh_letter_check(5, array('l_event' => 'R-ATReview', 'l_body' => '<p>no link</p>'));
t_ok((bool) array_filter($p, function ($x) { return $x['level'] == 'error' && $x['raw'] == '[ReviewLink]'; }), 'C1 a review letter without [ReviewLink] is an error');
$p = dh_letter_check(5, array('l_event' => 'A-ATReview', 'l_body' => '<p>Click [ReviewLink]</p>'));
t_ok(!array_filter($p, function ($x) { return $x['level'] == 'error'; }), 'C2 with [ReviewLink] there is no error');
$p = dh_letter_check(5, array('l_event' => 'R-ATReview', 'l_body' => '<p>[ReviewLink] old [RAuth]</p>'));
t_ok((bool) array_filter($p, function ($x) { return $x['level'] == 'warning' && $x['raw'] == '[RAuth]'; }), 'C3 [RAuth] gets a "use [ReviewLink]" warning');
$p = dh_letter_check(5, array('l_event' => 'Received', 'l_body' => '<p>[ReviewLink]</p>'));
t_ok((bool) array_filter($p, function ($x) { return $x['level'] == 'error' && $x['raw'] == '[ReviewLink]'; }), 'C4 [ReviewLink] in a letter that does not go to an AT is an error');

// Admin settings: both required fields exist
$form = drupal_get_form('dh_manageapp_admin_form');
$found = array();
foreach (element_children($form) as $g)
  foreach (array('dh_lc_review_link_r', 'dh_lc_review_link_a') as $k)
    if (isset($form[$g][$k]))
      $found[$k] = !empty($form[$g][$k]['#required']);
t_eq($found, array('dh_lc_review_link_r' => TRUE, 'dh_lc_review_link_a' => TRUE), 'S1 the two settings are on the admin settings page and required');

// dh_get_letter fills [ReviewLink] for the right AT
t_in_txn(function () {
  $course = t_make_course();
  $aid = t_make_applicant($course, array('a_email' => 'zzqtest@example.invalid'));
  db_insert('dh_applicant_lc')->fields(array('al_applicant' => $aid, 'al_recommending' => 'Zzqtest Rat', 'al_area_at' => 'Zzqtest Aat',
    'al_recommending_auth' => 'RCODE9', 'al_area_auth' => 'ACODE9'))->execute();
  $c = db_query("select c_center, c_course_type from dh_course where c_id = :c", array(':c' => $course))->fetchAssoc();
  $now = date('Y-m-d H:i:s');
  foreach (array('R-ATReview' => 'Zzqtest RLetter', 'A-ATReview' => 'Zzqtest ALetter') as $ev => $nm)
    db_insert('dh_letter')->fields(array('l_center' => $c['c_center'], 'l_event' => $ev, 'l_course_type' => $c['c_course_type'], 'l_name' => $nm,
      'l_subject' => 'Zzqtest review', 'l_body' => '<p>Open [ReviewLink] now</p>', 'l_sms' => '', 'l_deleted' => 0,
      'l_created' => $now, 'l_created_by' => 777120, 'l_updated' => $now, 'l_updated_by' => 777120))->execute();
  $r = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  t_ok(!empty($r['result']) && strpos($r['body'], 'https://zzqtest.invalid/r/RCODE9') !== FALSE, 'G1 the recommending AT gets the recommending link with its code');
  $a = dh_get_letter('a-teacher', $aid, 'A-ATReview');
  t_ok(!empty($a['result']) && strpos($a['body'], 'https://zzqtest.invalid/a/ACODE9') !== FALSE, 'G2 the area AT gets the area link with its code');
  $GLOBALS['conf']['dh_lc_review_link_r'] = '';
  $e = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  t_ok(empty($e['result']), 'G3 an empty setting: the letter is not sent');
  t_ok((bool) db_query("select 1 from dh_log where l_module = 'Applicant' and l_identifier = :a and l_msg like 'Letter not sent: the review link%'", array(':a' => $aid))->fetchField(), 'G3 and the activity log says why');
  $GLOBALS['conf']['dh_lc_review_link_r'] = 'https://zzqtest.invalid/r/';
});

// Manage Fields refuses a centre field named like DIPI's review-only fields
$db->transaction();
try {
  $res = dh_letter_fields_editor($db, 9900001, 777121)->transaction(false)->process(array('action' => 'create',
    'data' => array(0 => array('dh_letter_fields' => array('lf_name' => 'ReviewLink', 'lf_value' => 'x', 'lf_deleted' => 0)))))->data();
  t_ok(!empty($res['fieldErrors']), 'M1 a centre field cannot be named ReviewLink');
} catch (Throwable $e) { t_ok(FALSE, 'unexpected error: ' . $e->getMessage()); }
$db->rollback();
t_done();
