<?php
// Registrar's "R-AT Review" warning (search page): dh_lc_transfer_warning_at() names the recommending AT who
// transferred an LC application to the registrar while that AT is still the recommending AT, and is '' otherwise.
// Made-up data, always rolled back. Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-transfer-warning.php
require __DIR__ . '/lib.php';

function t_tw_app() {
  $course = t_make_course(array('c_center' => 6, 'c_course_type' => 13, 'c_start' => date('Y-m-d', strtotime('+30 days')),
    'c_end' => date('Y-m-d', strtotime('+76 days'))));
  $aid = t_make_applicant($course, array('a_status' => 'R-ATReview', 'a_attended' => 0));
  db_insert('dh_applicant_lc')->fields(array('al_applicant' => $aid, 'al_recommending' => 'Zzqtest Rat', 'al_area_at' => '',
    'al_recommending_approved' => 'Pending', 'al_area_at_approved' => 'Pending'))->execute();
  return $aid;
}
function t_tw_row($aid) {
  return db_query("select a.a_id, a.a_status, al.al_recommending, al.al_recommending_approved from dh_applicant a
    join dh_applicant_lc al on al.al_applicant = a.a_id where a.a_id = :a", array(':a' => $aid))->fetchAssoc();
}
function t_tw_lc($aid, $f) { db_update('dh_applicant_lc')->fields($f)->condition('al_applicant', $aid)->execute(); }

t_ok(function_exists('dh_lc_transfer_warning_at'), 'F0 dh_lc_transfer_warning_at() exists');
if (!function_exists('dh_lc_transfer_warning_at')) t_done();

// The AT transfers it themself (Portal / email link / app all save the decision "Transfer to Registrar")
t_in_txn(function () {
  $aid = t_tw_app();
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), '', 'P0 not transferred: no warning');
  t_tw_lc($aid, array('al_recommending_approved' => 'Transfer to Registrar'));
  _update_status($aid, 'R-ATTransfer', 777140);
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), 'Zzqtest Rat', 'P1 the AT transferred it and is still the recommending AT: warn with their name');
  t_tw_lc($aid, array('al_recommending' => 'Zzqtest Rat Two'));
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), '', 'P2 the registrar chose another AT: no warning');
  t_tw_lc($aid, array('al_recommending' => 'Zzqtest Rat'));
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), 'Zzqtest Rat', 'P3 set back to the AT who transferred it: warn again');
  t_tw_lc($aid, array('al_recommending' => 'ZZQTEST RAT'));
  t_ok(dh_lc_transfer_warning_at(t_tw_row($aid)) !== '', 'P4 the same name in other letter case is the same AT: warn');
  // The registrar sends it back: decision becomes Pending, status R-ATReview -> no warning
  t_tw_lc($aid, array('al_recommending_approved' => 'Pending'));
  _update_status($aid, 'R-ATReview', 777141);
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), '', 'P5 sent back to review: no warning');
});

// DIPI moved it to the registrar because the AT had no email address: the decision stays Pending -> no warning
t_in_txn(function () {
  $aid = t_tw_app();
  _update_status($aid, 'R-ATTransfer', 15);
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), '', 'P6 DIPI moved it (AT had no email): no warning');
});

// The app's transfer empties the recommending AT: nothing to warn about (DIPI asks for an AT anyway)
t_in_txn(function () {
  $aid = t_tw_app();
  t_tw_lc($aid, array('al_recommending_approved' => 'Transfer to Registrar', 'al_recommending' => ''));
  _update_status($aid, 'R-ATTransfer', 397);
  t_eq(dh_lc_transfer_warning_at(t_tw_row($aid)), '', 'P7 no recommending AT left: no warning');
});

t_done();
