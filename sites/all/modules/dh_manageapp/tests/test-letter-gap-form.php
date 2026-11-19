<?php
// Letter editor (inc/letters.inc): a change that leaves a status with no letter, or maps it to Review, is held until "Save anyway" is ticked.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-gap-form.php
// Never calls the submit handler or the delete/restore links: their drupal_goto() would exit and commit the test transaction.
require __DIR__ . '/lib.php';

function tf_letter($centre, $over = array()) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => $centre, 'l_event' => 'Confirmed', 'l_course_type' => 0,
    'l_name' => 'Zzq letter', 'l_subject' => 'Zzq subject', 'l_body' => '<p>Dear [FirstName]</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 0, 'l_updated' => $now, 'l_updated_by' => 0), $over))->execute();
}
function tf_row($id) { return db_query("select * from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc(); }
// The letter form as the browser would get it for $path with $input posted (empty input = first GET).
function tf_build($path, $input, $row = array()) {
  $_GET['q'] = $path;
  $form_state = form_state_defaults();
  $form_state['input'] = $input;
  $form = dh_letters_form(array(), $form_state, $row);
  return array($form, $form_state);
}
// Builder + validation for a POST; returns array(errors, form, form_state).
function tf_post($path, $input, $row = array()) {
  list($form, $form_state) = tf_build($path, $input, $row);
  form_clear_error();
  drupal_get_messages();
  dh_letters_form_validate($form, $form_state);
  $errors = form_get_errors();
  drupal_get_messages();
  return array($errors ? $errors : array(), $form, $form_state);
}
function tf_input($row, $over = array()) {
  return array_merge(array('l_name' => $row['l_name'], 'l_event' => $row['l_event'], 'l_course_type' => $row['l_course_type'],
    'l_subject' => $row['l_subject'], 'l_body' => $row['l_body'], 'l_sms' => $row['l_sms'], 'op' => 'Update Letter'), $over);
}

t_in_txn(function () {
  $c = 9900001;
  foreach (array(6, 8, 10, 12, 13) as $t) t_make_course(array('c_center' => $c, 'c_course_type' => $t));
  drupal_static_reset('dh_letter_course_types_to_cover_runs');
  $lc = tf_letter($c, array('l_name' => 'Zzq LC n SPL'));
  $ten = tf_letter($c, array('l_name' => 'Zzq 10 Day', 'l_course_type' => 6));
  $row = tf_row($lc);

  list($form) = tf_build("letters/$c/edit/$lc", array(), $row);
  t_ok(!isset($form['l_gap_ok']), 'F1 opening the form: no tick');

  list($err, $form) = tf_post("letters/$c/edit/$lc", tf_input($row, array('l_course_type' => 45)), $row);
  t_ok(isset($form['l_gap_ok']), 'F2 a change with a gap: the tick is on the form');
  t_ok(isset($err['l_gap_ok']) && strpos($err['l_gap_ok'], 'no letter') !== FALSE && strpos($err['l_gap_ok'], '20 Day') !== FALSE, 'F2 held, with the gaps named');
  t_eq(tf_row($lc)['l_course_type'], 0, 'F2 nothing saved');

  list($err, $form, $fs) = tf_post("letters/$c/edit/$lc", tf_input($row, array('l_course_type' => 45, 'l_gap_ok' => '1')), $row);
  t_ok(!isset($err['l_gap_ok']), 'F3 ticked: not held');
  t_ok(!empty($fs['dh_letter_anyway']['gaps']['Confirmed']), 'F3 the warnings are kept for the record');

  $r10 = tf_row($ten);
  list($err, $form, $fs) = tf_post("letters/$c/edit/$ten", tf_input($r10, array('l_name' => 'Zzq 10 Day renamed')), $r10);
  t_ok(!isset($form['l_gap_ok']) && !isset($err['l_gap_ok']) && empty($fs['dh_letter_anyway']), 'F4 a change with no gap: no tick, not held');

  $new = array('l_name' => 'Zzq review', 'l_event' => 'Review', 'l_course_type' => 0, 'l_subject' => 's', 'l_body' => '<p>x</p>', 'l_sms' => '', 'op' => 'Add Letter');
  list($err) = tf_post("letters/$c/add", $new);
  t_ok(isset($err['l_gap_ok']) && strpos($err['l_gap_ok'], 'Review is an internal status') !== FALSE, 'F5 a new Review letter is held');
  list($err) = tf_post("letters/$c/add", $new + array('l_gap_ok' => '1'));
  t_ok(!isset($err['l_gap_ok']), 'F5 ticked: not held');

  $lcr = tf_letter($c, array('l_name' => 'Recommending Review', 'l_event' => 'R-ATReview', 'l_body' => '<p>[ReviewLink]</p>'));
  $rr = tf_row($lcr);
  list($err, $form) = tf_post("letters/$c/edit/$lcr", tf_input($rr), $rr);
  t_ok(!isset($form['l_gap_ok']) && !isset($err['l_gap_ok']), 'F6 a locked long-course letter is never held by this check');

  $bad = tf_input($r10, array('l_body' => '<p>Dear [firstname]</p>'));
  list($err) = tf_post("letters/$c/edit/$ten", $bad, $r10);
  t_ok(!empty($err) && !isset($err['l_gap_ok']), 'F7 the merge-field check still holds a bad field, as before');
});
// G1-G5: Make inactive / Make active through the checked function (the router only adds messages and drupal_goto).
t_in_txn(function () {
  $c = 9900001;
  foreach (array(6, 10) as $t) t_make_course(array('c_center' => $c, 'c_course_type' => $t));
  drupal_static_reset('dh_letter_course_types_to_cover_runs');
  $all = tf_letter($c, array('l_name' => 'Zzq all'));
  $ten = tf_letter($c, array('l_name' => 'Zzq ten', 'l_course_type' => 6));
  $wd = function () { return (int) db_query("select count(*) from watchdog where type = 'letter_gap'")->fetchField(); };

  $r = dh_letter_set_inactive_checked($c, $all, TRUE, 777041, FALSE);
  t_ok(!$r['done'] && !empty($r['held']['gaps']['Confirmed']), 'G1 making the All letter inactive is held (20 Day would get no letter)');
  t_eq(tf_row($all)['l_deleted'], 0, 'G1 not changed');

  $n = $wd();
  $r = dh_letter_set_inactive_checked($c, $all, TRUE, 777042, TRUE);
  t_ok($r['done'] && $r['held'] === NULL, 'G2 anyway: done');
  t_eq(array(tf_row($all)['l_deleted'], tf_row($all)['l_updated_by']), array(1, 777042), 'G2 inactive, by who did it');
  t_eq($wd(), $n + 1, 'G2 one letter_gap record');

  $r = dh_letter_set_inactive_checked($c, $ten, TRUE, 777043, FALSE);
  t_ok(!$r['done'] && !empty($r['held']['gaps']), 'G3 now the 10 Day letter is the last one for 10 Day: held');
  $r = dh_letter_set_inactive_checked($c, $all, FALSE, 777044, FALSE);
  t_ok($r['done'] && $r['held'] === NULL, 'G3 making the All letter active again: no warning, done at once');
  $r = dh_letter_set_inactive_checked($c, $ten, TRUE, 777045, FALSE);
  t_ok($r['done'], 'G3 with the All letter back, the 10 Day letter can be made inactive at once');

  $rv = tf_letter($c, array('l_name' => 'Zzq review', 'l_event' => 'Review', 'l_deleted' => 1));
  $r = dh_letter_set_inactive_checked($c, $rv, FALSE, 777046, FALSE);
  t_ok(!$r['done'] && !empty($r['held']['review']), 'G4 making a Review letter active is held');
  t_ok(dh_letter_set_inactive_checked($c, $rv, FALSE, 777047, TRUE)['done'], 'G4 anyway: done');

  $r = dh_letter_set_inactive_checked(9900002, $all, TRUE, 777048, TRUE);
  t_ok(!$r['done'] && $r['held'] === NULL, "G5 another centre's letter: refused as before");
  $lcr = tf_letter($c, array('l_name' => 'Recommending Review', 'l_event' => 'R-ATReview'));
  $r = dh_letter_set_inactive_checked($c, $lcr, TRUE, 777049, FALSE);
  t_ok(!$r['done'] && $r['held'] === NULL, 'G5 a long-course letter: refused as before');
});
// H1-H4: the Letter check box on the Letters page
t_eq(dh_letter_check_box(0), '', 'H1 no box for the template centre 0');
t_in_txn(function () {
  $c = 9900001;
  foreach (array(6, 10) as $t) t_make_course(array('c_center' => $c, 'c_course_type' => $t));
  drupal_static_reset('dh_letter_course_types_to_cover_runs');
  drupal_static_reset('dh_letter_cover_statuses');
  $all = tf_letter($c, array('l_name' => 'Zzq <b>all</b>'));
  tf_letter($c, array('l_name' => 'Zzq cancel', 'l_event' => 'Cancelled', 'l_course_type' => 6));
  tf_letter($c, array('l_name' => 'Zzq review', 'l_event' => 'Review'));
  $h = dh_letter_check_box($c);
  t_ok(strpos($h, '<details class="lcov">') === 0, 'H2 a folded box');
  t_ok(strpos($h, 'statuses have course types with no letter') !== FALSE, 'H2 summary counts the statuses with a gap');
  t_ok(strpos($h, 'letters/' . $c . '/view/' . $all) !== FALSE, 'H2 a letter name links to its page');
  t_ok(strpos($h, 'Zzq <b>all</b>') === FALSE && strpos($h, 'Zzq &lt;b&gt;all&lt;/b&gt;') !== FALSE, 'H2 names are escaped');
  t_ok(strpos($h, 'lcov-none') !== FALSE, 'H2 No letter cells are marked');
  t_ok(strrpos($h, '>Review<') > strrpos($h, '>Duplicate<'), 'H3 the Review row is last');
  $review_row = substr($h, strrpos($h, '>Review<'));
  t_ok(strpos(substr($review_row, 0, strpos($review_row, '</tr>')), 'lcov-none') === FALSE, 'H3 the Review row has no red');
  $_GET['q'] = "letters/$c";
  t_ok(strpos(dh_manage_letters(), '<details class="lcov">') !== FALSE, 'H4 the Letters page shows the box');
});

// H5 optional statuses (Left, Clarification-Response): grey "No letter (optional)", not counted
t_in_txn(function () {
  $c = 9900001;
  t_make_course(array('c_center' => $c, 'c_course_type' => 6));
  drupal_static_reset('dh_letter_course_types_to_cover_runs');
  drupal_static_reset('dh_letter_cover_statuses');
  foreach (dh_letter_cover_statuses() as $s)
    if (!dh_letter_cover_is_review($s) && !dh_letter_cover_is_optional($s))
      tf_letter($c, array('l_name' => 'Zzq ' . $s, 'l_event' => $s));
  $h = dh_letter_check_box($c);
  t_ok(strpos($h, 'Letter check: every status has a letter') !== FALSE, 'H5 only the optional statuses have no letter: the summary is green');
  t_ok(strpos($h, 'lcov-none') === FALSE, 'H5 no red cell');
  t_eq(substr_count($h, 'lcov-optional'), 2, 'H5 two grey cells (Left and Clarification-Response, one course type)');
  t_ok(strpos($h, 'No letter (optional)') !== FALSE, 'H5 they say "No letter (optional)"');
  t_ok(strpos($h, 'Left and Clarification-Response are optional') !== FALSE, 'H5 the help line explains it');
});
t_done();
