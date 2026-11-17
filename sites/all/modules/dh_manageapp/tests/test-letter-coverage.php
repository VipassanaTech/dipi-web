<?php
// Letter coverage (inc/letter-coverage.inc): the same letter as the send code, and what-if checks.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-coverage.php
require __DIR__ . '/lib.php';

// The send code's own query (dh_get_letter, letter not picked by hand): l_id, or NULL for "No letter Found!".
function tc_send_pick($c, $e, $t) {
  $r = db_query("select l_id, l_subject from dh_letter l where l_center='" . (int) $c . "' and l_event='" . addslashes($e) . "' and l_course_type in ('" . (int) $t . "', '0') and l_deleted=0 order by l_course_type desc, l_name asc limit 1")->fetchAssoc();
  return ($r && !($r['l_subject'] == '')) ? (int) $r['l_id'] : NULL;
}
function tc_letter($centre, $over = array()) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => $centre, 'l_event' => 'Confirmed', 'l_course_type' => 0,
    'l_name' => 'Zzq letter', 'l_subject' => 'Zzq subject', 'l_body' => '<p>x</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 0, 'l_updated' => $now, 'l_updated_by' => 0), $over))->execute();
}
// Made-up courses so centre 9900001 runs exactly these course types.
function tc_types($centre, $types) {
  foreach ($types as $t) t_make_course(array('c_center' => $centre, 'c_course_type' => $t));
  drupal_static_reset('dh_letter_course_types_to_cover_runs');
}

// C1 the statuses checked
$st = array_map('_dh_lcov_key', dh_letter_cover_statuses());
foreach (array('confirmed', 'cancelled', 'received', 'expected', 'review', 'duplicate', 'waitlist') as $s) t_ok(in_array($s, $st), "C1 $s is checked");
foreach (array('custom', 'r-atreview', 'a-atreview', 'r-attransfer') as $s) t_ok(!in_array($s, $st), "C1 $s is not checked");
t_eq(count($st), count(array_unique($st)), 'C1 no status twice');
t_ok(dh_letter_cover_is_review('Review ') && !dh_letter_cover_is_review('Confirmed'), 'C1 Review is known');

// C2 same letter as the send code, for every local centre, status and course type it runs
$bad = 0; $n = 0;
foreach (db_query("select distinct l_center from dh_letter where l_deleted = 0 and l_center > 0")->fetchCol() as $c) {
  $cov = dh_letter_coverage($c);
  foreach ($cov as $s => $bytype) foreach ($bytype as $t => $l) {
    $n++;
    if ((($l === NULL) ? NULL : $l['l_id']) !== tc_send_pick($c, $s, $t)) { $bad++; if ($bad <= 5) echo "    differs: centre $c, $s, type $t\n"; }
  }
}
t_ok($n > 1000, "C2 checked $n centre/status/type cases");
t_eq($bad, 0, 'C2 every case picks the same letter as the send code');

// C3 the what-if path with the letter unchanged gives the same order (types stay numbers, same name order)
$bad = 0;
foreach (db_query("select * from dh_letter where l_deleted = 0 and l_center > 0 order by l_id desc limit 300")->fetchAll(PDO::FETCH_ASSOC) as $row) {
  $a = array_column(_dh_lcov_candidates($row['l_center'], $row['l_event']), 'l_id');
  $b = array_column(_dh_lcov_candidates($row['l_center'], $row['l_event'], $row), 'l_id');
  if ($a !== $b) $bad++;
}
t_eq($bad, 0, 'C3 what-if with no change = same letters in the same order (300 letters)');

// C4 edge cases on a made-up centre
t_in_txn(function () {
  $c = 9900001;
  tc_types($c, array(6, 10));
  $b = tc_letter($c, array('l_name' => 'Zzq B'));
  $a = tc_letter($c, array('l_name' => 'zzq a '));                 // lower case + space at the end: still first by name
  $x = tc_letter($c, array('l_event' => 'confirmed', 'l_course_type' => 6, 'l_name' => 'Zzq exact'));  // event in lower case
  $d = tc_letter($c, array('l_event' => 'Cancelled', 'l_course_type' => 6, 'l_subject' => ''));
  tc_letter($c, array('l_event' => 'Cancelled', 'l_course_type' => 0));
  tc_letter($c, array('l_event' => 'WaitList', 'l_deleted' => 1));
  $cov = dh_letter_coverage($c);
  t_eq($cov['Confirmed'][10]['l_id'], $a, 'C4 All letters: first by name as the database sorts it');
  t_eq($cov['Confirmed'][6]['l_id'], $x, 'C4 exact course type wins, event case does not matter');
  t_ok($cov['Cancelled'][6] === NULL, 'C4 the chosen letter has an empty subject = no letter');
  t_eq($cov['Cancelled'][10]['l_id'] > 0, TRUE, 'C4 other type uses the All letter');
  t_ok($cov['WaitList'][6] === NULL && $cov['WaitList'][10] === NULL, 'C4 inactive letters are not used');
  foreach (array(6, 10) as $t) foreach (array('Confirmed', 'Cancelled', 'WaitList') as $s)
    t_eq(($cov[$s][$t] === NULL) ? NULL : $cov[$s][$t]['l_id'], tc_send_pick($c, $s, $t), "C4 same as send code: $s / $t");
  // what-if: the exact-type letter moved to type 45 -> type 6 falls back to the All letter
  $row = db_query("select * from dh_letter where l_id = :l", array(':l' => $x))->fetchAssoc();
  $row['l_course_type'] = 45;
  t_eq(dh_letter_coverage($c, $row)['Confirmed'][6]['l_id'], $a, 'C4 what-if changes the pick without saving');
  t_eq(db_query("select l_course_type from dh_letter where l_id = :l", array(':l' => $x))->fetchField(), 6, 'C4 nothing was saved');
  // what-if: a new letter (l_id 0)
  $new = array('l_id' => 0, 'l_event' => 'WaitList', 'l_course_type' => 0, 'l_name' => 'Zzq new', 'l_subject' => 's', 'l_deleted' => 0);
  t_eq(dh_letter_coverage($c, $new)['WaitList'][6]['l_id'], 0, 'C4 a new letter can cover a status');
});
// C5-C10: warnings for a letter change (Thali replay and the other cases)
t_in_txn(function () {
  $c = 9900001;
  tc_types($c, array(6, 8, 10, 12, 13));
  $names = db_query("select td_id, td_val1 from dh_type_detail where td_type = 'COURSE-TYPE' and td_id in (8, 10, 12, 13)")->fetchAllKeyed();
  $lc = tc_letter($c, array('l_name' => 'Zzq LC n SPL'));                       // Confirmed, All types
  $ten = tc_letter($c, array('l_name' => 'Zzq 10 Day', 'l_course_type' => 6));   // Confirmed, 10 Day
  $form = array('l_event' => 'Confirmed', 'l_course_type' => 45, 'l_name' => 'Zzq LC n SPL', 'l_subject' => 'Zzq subject');

  $w = dh_letter_change_warnings($c, 'edit', $lc, $form);
  t_eq($w['gaps'], array('Confirmed' => $names), 'C5 Thali replay: All -> Other leaves 10 Day Special, 20, 30, 45 Day with no letter');
  t_ok($w['review'] === FALSE && dh_letter_has_warnings($w), 'C5 a warning, not a Review one');
  t_eq(dh_letter_change_warnings($c, 'edit', $lc, array('l_course_type' => 0) + $form)['gaps'], array(), 'C5 back to All: no gap');
  t_eq(dh_letter_change_warnings($c, 'edit', $ten, array('l_name' => 'Zzq 10 renamed', 'l_course_type' => 6) + $form)['gaps'], array(), 'C5 a rename leaves no gap');
  t_ok(!isset(dh_letter_change_warnings($c, 'edit', $lc, $form)['gaps']['Cancelled']), 'C5 an old gap (no Cancelled letter at all) is not reported');

  t_eq(dh_letter_change_warnings($c, 'inactive', $lc)['gaps'], array('Confirmed' => $names), 'C6 making the All letter inactive: same gaps');
  t_eq(dh_letter_change_warnings($c, 'inactive', $ten)['gaps'], array(), 'C6 making the 10 Day letter inactive: the All letter still covers 10 Day');
  $off = tc_letter($c, array('l_name' => 'Zzq off', 'l_event' => 'WaitList', 'l_deleted' => 1));
  t_ok(!dh_letter_has_warnings(dh_letter_change_warnings($c, 'active', $off)), 'C6 making a letter active: nothing to warn');

  $add = array('l_event' => 'Review', 'l_course_type' => 0, 'l_name' => 'Zzq review', 'l_subject' => 's');
  $w = dh_letter_change_warnings($c, 'add', '', $add);
  t_ok($w['review'] === TRUE && $w['gaps'] === array(), 'C7 a new Review letter: Review warning');
  $rv = tc_letter($c, array('l_name' => 'Zzq review', 'l_event' => 'Review'));
  t_ok(dh_letter_change_warnings($c, 'edit', $rv, $add)['review'], 'C7 editing a Review letter: Review warning every time');
  t_ok(!dh_letter_has_warnings(dh_letter_change_warnings($c, 'inactive', $rv)), 'C7 making a Review letter inactive: no warning');
  $rv2 = tc_letter($c, array('l_name' => 'Zzq review 2', 'l_event' => 'review', 'l_deleted' => 1));
  t_ok(dh_letter_change_warnings($c, 'active', $rv2)['review'], 'C7 making a Review letter active: Review warning (any case)');

  $t0 = tc_letter(0, array('l_name' => 'Zzq template'));
  t_ok(!dh_letter_has_warnings(dh_letter_change_warnings(0, 'inactive', $t0)), 'C8 template centre 0: never held');
  t_ok(!dh_letter_has_warnings(dh_letter_change_warnings(9900002, 'inactive', $lc)), "C8 another centre's letter: nothing");
  $lcr = tc_letter($c, array('l_name' => 'Recommending Review', 'l_event' => 'R-ATReview'));
  t_ok(!dh_letter_has_warnings(dh_letter_change_warnings($c, 'inactive', $lcr)), 'C8 a locked long-course letter: never held');

  t_ok(!dh_letter_has_warnings(dh_letter_change_warnings($c, 'copy', $lc, array('l_course_type' => 0) + $form)), 'C9 a copy adds a letter: no gap');

  $w = dh_letter_change_warnings($c, 'edit', $lc, $form);
  $h = dh_letter_warnings_html($w);
  t_ok(strpos($h, 'no letter') !== FALSE && strpos($h, 'Confirmed') !== FALSE && strpos($h, $names[10]) !== FALSE, 'C10 message names the status and course types');
  t_ok(strpos($h, 'Save anyway') !== FALSE, 'C10 save message says Save anyway');
  t_ok(strpos(dh_letter_warnings_html($w, 'inactive'), 'Make inactive anyway') !== FALSE, 'C10 inactive message says Make inactive anyway');
  t_ok(strpos(dh_letter_warnings_html(array('gaps' => array('<b>X</b>' => array(1 => '<i>T</i>')), 'review' => FALSE)), '<b>X</b>') === FALSE, 'C10 names are escaped');
  t_ok(strpos(dh_letter_warnings_html(array('gaps' => array(), 'review' => TRUE)), 'Review is an internal status') !== FALSE, 'C10 Review message');
  t_eq(dh_letter_warnings_text(array('gaps' => array('Confirmed' => array(10 => '20 Day Course')), 'review' => TRUE)), 'Confirmed: 20 Day Course; letter for Review', 'C10 plain text line');
  $before = (int) db_query("select count(*) from watchdog where type = 'letter_gap'")->fetchField();
  dh_letter_gap_record($c, $lc, 'edit', $w);
  t_eq((int) db_query("select count(*) from watchdog where type = 'letter_gap'")->fetchField(), $before + 1, 'C10 one letter_gap record');
});
t_done();
