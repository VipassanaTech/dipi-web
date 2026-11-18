<?php
// LC email review links: each step (recommending, area) has its own code, made when missing (letters.inc
// dh_get_letter); a step's code is wiped when that step's AT changes (trigger dh_lc_bu, installed by
// deploy/2026-11-18-lc-review-code-reset.php), so a former AT's link stops working; a transfer from the email
// page is logged under the recommending email user, not the applicant user (update_status_external).
// Made-up data (no email, no mobile, centre 6 has no transfer letter), always rolled back.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-review-codes.php
require __DIR__ . '/lib.php';

$GLOBALS['conf']['dh_lc_review_link_r'] = 'https://zzqtest.invalid/r/';
$GLOBALS['conf']['dh_lc_review_link_a'] = 'https://zzqtest.invalid/a/';
$uids = db_query("select td_key, td_val1 from dh_type_detail where td_type = 'COURSE-APPLICANT'")->fetchAllKeyed();

function t_codes($aid) {
  return db_query("select al_recommending_auth r, al_area_auth a from dh_applicant_lc where al_applicant = :a", array(':a' => $aid))->fetchAssoc();
}
function t_lc_set($aid, $f) { db_update('dh_applicant_lc')->fields($f)->condition('al_applicant', $aid)->execute(); }
function t_is_code($code, $aid) { return (bool) preg_match('/^' . $aid . '[0-9A-Za-z]{15}$/', (string) $code); }
// A made-up LC applicant on a future 45-day course at centre 6, waiting for the recommending AT. Letters are only
// built when the applicant has an email or mobile, so letter tests pass an address that can never be delivered.
function t_lc_app($email = '') {
  $course = t_make_course(array('c_center' => 6, 'c_course_type' => 13, 'c_start' => date('Y-m-d', strtotime('+30 days')),
    'c_end' => date('Y-m-d', strtotime('+76 days'))));
  $aid = t_make_applicant($course, array('a_status' => 'R-ATReview', 'a_attended' => 0, 'a_email' => $email));
  db_insert('dh_applicant_lc')->fields(array('al_applicant' => $aid, 'al_recommending' => 'Zzqtest Rat', 'al_area_at' => '',
    'al_recommending_approved' => 'Pending', 'al_area_at_approved' => 'Pending'))->execute();
  return $aid;
}

t_ok((bool) db_query("select 1 from information_schema.triggers where trigger_schema = database() and trigger_name = 'dh_lc_bu' and action_statement like '%al_recommending_auth%' and action_statement like '%al_area_auth%'")->fetchField(),
  'D0 trigger dh_lc_bu wipes review codes (the deploy script has run)');

t_in_txn(function () {
  $aid = t_lc_app('zzqtest@example.invalid');   // dh_get_letter only builds the letter; nothing is sent
  $now = date('Y-m-d H:i:s');
  foreach (array('R-ATReview' => 'Zzqtest RLetter', 'A-ATReview' => 'Zzqtest ALetter') as $ev => $nm)
    db_insert('dh_letter')->fields(array('l_center' => 6, 'l_event' => $ev, 'l_course_type' => 13, 'l_name' => $nm, 'l_subject' => 'Zzqtest review',
      'l_body' => '<p>Open [ReviewLink] now</p>', 'l_sms' => '', 'l_deleted' => 0, 'l_created' => $now, 'l_created_by' => 777130,
      'l_updated' => $now, 'l_updated_by' => 777130))->execute();

  // C1: the first letter makes both codes, in the usual form
  $l = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  $c1 = t_codes($aid);
  t_ok(t_is_code($c1['r'], $aid) && t_is_code($c1['a'], $aid), 'C1 the first letter makes both codes: application number + 15 letters/digits');
  t_ok($c1['r'] !== $c1['a'], 'C1 the two codes differ');
  t_ok(!empty($l['body']) && strpos($l['body'], 'https://zzqtest.invalid/r/' . $c1['r']) !== FALSE, 'C1 the recommending letter carries the recommending code');

  // W1: the same AT saved again keeps both codes
  t_lc_set($aid, array('al_recommending' => 'Zzqtest Rat', 'al_recommending_comments' => 'x'));
  t_eq(t_codes($aid), $c1, 'W1 saving the same recommending AT keeps both codes');

  // W2: a new recommending AT wipes only the recommending code
  t_lc_set($aid, array('al_recommending' => 'Zzqtest Rat Two'));
  $c2 = t_codes($aid);
  t_eq($c2['r'], NULL, 'W2 a new recommending AT wipes the recommending code (the old link stops)');
  t_eq($c2['a'], $c1['a'], 'W2 ... and keeps the area code');

  // C2: the next recommending letter makes a fresh recommending code only
  $l = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  $c3 = t_codes($aid);
  t_ok(t_is_code($c3['r'], $aid) && $c3['r'] !== $c1['r'], 'C2 the next recommending letter makes a fresh code');
  t_eq($c3['a'], $c1['a'], 'C2 ... and leaves the area code alone');
  t_ok(!empty($l['body']) && strpos($l['body'], $c3['r']) !== FALSE && strpos($l['body'], $c1['r']) === FALSE, "C2 the new AT's letter has the new code, not the old one");

  // W3: the recommending AT approves and names the area AT: the (never sent) area code is wiped, then made fresh
  t_lc_set($aid, array('al_area_at' => 'Zzqtest Aat', 'al_recommending_approved' => 'Approved'));
  t_eq(t_codes($aid)['a'], NULL, 'W3 naming the area AT wipes the unsent area code');
  db_update('dh_applicant')->fields(array('a_status' => 'A-ATReview'))->condition('a_id', $aid)->execute();
  $l = dh_get_letter('a-teacher', $aid, 'A-ATReview');
  $c4 = t_codes($aid);
  t_ok(t_is_code($c4['a'], $aid) && !empty($l['body']) && strpos($l['body'], 'https://zzqtest.invalid/a/' . $c4['a']) !== FALSE, 'W3 the area letter makes a fresh area code and carries it');
  t_eq($c4['r'], $c3['r'], 'W3 ... the recommending code is untouched');

  // W4: area transfer (Portal) wipes the area code
  t_lc_set($aid, array('al_area_at' => 'Zzqtest Aat Two'));
  t_eq(t_codes($aid)['a'], NULL, 'W4 an area transfer wipes the area code (the old area link stops)');
  t_eq(t_codes($aid)['r'], $c3['r'], 'W4 ... the recommending code is untouched');

  // W5: a save that writes a code itself keeps that code, even when the AT changes in the same save
  t_lc_set($aid, array('al_area_at' => 'Zzqtest Aat Three', 'al_area_auth' => $aid . 'ZZQexplicit0001'));
  t_eq(t_codes($aid)['a'], $aid . 'ZZQexplicit0001', 'W5 a code written in the same save is kept');

  // W6: a change only in letter case is the same AT: the code stays
  t_lc_set($aid, array('al_area_at' => 'ZZQTEST AAT THREE'));
  t_eq(t_codes($aid)['a'], $aid . 'ZZQexplicit0001', 'W6 the same name in other letter case keeps the code');
});

// S1: an email-page transfer (status-trigger.php: no login) is logged under the recommending email user
t_in_txn(function () use ($uids) {
  $aid = t_lc_app();
  $saved = $GLOBALS['user'];
  $GLOBALS['user'] = drupal_anonymous_user();
  update_status_external($aid, 'R-ATTransfer');
  $GLOBALS['user'] = $saved;
  $a = db_query("select a_status, a_updated_by from dh_applicant where a_id = :a", array(':a' => $aid))->fetchAssoc();
  t_eq($a['a_status'], 'R-ATTransfer', 'S1 the email-page transfer moves the status to R-ATTransfer');
  t_eq((int) $a['a_updated_by'], (int) $uids['COURSE-R-AT-UID'], 'S1 ... recorded under the recommending email user (not the applicant user)');
  $lu = db_query("select l_user from dh_log where l_module = 'Applicant' and l_identifier = :a and l_event = 'Status Change' and l_msg like '%To R-ATTransfer' order by l_id desc limit 1", array(':a' => $aid))->fetchField();
  t_eq((int) $lu, (int) $uids['COURSE-R-AT-UID'], 'S1 ... and the Activity Log shows that user');
});

// S2: a transfer made while logged in (Portal / app) keeps that person's own user
t_in_txn(function () {
  $aid = t_lc_app();
  $saved = $GLOBALS['user'];
  $GLOBALS['user'] = (object) array('uid' => 777131, 'name' => 'zzqtest', 'roles' => array(2 => 'authenticated user'));
  update_status_external($aid, 'R-ATTransfer');
  $GLOBALS['user'] = $saved;
  t_eq((int) db_query("select a_updated_by from dh_applicant where a_id = :a", array(':a' => $aid))->fetchField(), 777131, 'S2 a logged-in transfer keeps its own user');
});

// S3: the other no-login steps keep their users (intake = applicant user)
t_in_txn(function () use ($uids) {
  $aid = t_lc_app();
  db_update('dh_applicant')->fields(array('a_status' => 'Review'))->condition('a_id', $aid)->execute();
  $saved = $GLOBALS['user'];
  $GLOBALS['user'] = drupal_anonymous_user();
  update_status_external($aid, 'Received');   // LC row pending -> goes to R-ATReview (a new application)
  $GLOBALS['user'] = $saved;
  $a = db_query("select a_status, a_updated_by from dh_applicant where a_id = :a", array(':a' => $aid))->fetchAssoc();
  t_eq(array($a['a_status'], (int) $a['a_updated_by']), array('R-ATReview', (int) $uids['COURSE-APPLICANT-UID']), 'S3 a new application still goes to R-ATReview under the applicant user');
});

t_done();
