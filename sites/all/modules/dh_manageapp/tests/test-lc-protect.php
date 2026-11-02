<?php
// LC letter rules (inc/letter-protect.inc): which letters are LC letters, what a save may do, by-name lookup.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-protect.php
require __DIR__ . '/lib.php';

$r = function ($name, $event) { return dh_letter_lc_role(array('l_name' => $name, 'l_event' => $event)); };
t_eq($r('Anything at all', 'R-ATReview'), 'r_review', 'P1 the R-ATReview status makes the recommending review letter, whatever its name');
t_eq($r('km Area Teacher Review', 'A-ATReview'), 'a_review', 'P1 the A-ATReview status makes the area review letter');
t_eq($r('Area AT Approved', 'Custom'), 'a_approved', 'P2 a reserved name, any status');
t_eq($r('AREA AT APPROVED  ', 'Received'), 'a_approved', 'P2 capitals and trailing spaces still match (as MySQL finds it)');
t_eq($r('LC Admin Notification', 'Custom'), 'lc_admin', 'P2 LC Admin Notification');
t_eq($r('km Area AT Approved', 'Custom'), NULL, 'P3 a prefixed name is an ordinary letter');
t_eq($r('Received Common', 'Received'), NULL, 'P3 an ordinary letter');
t_ok(dh_letter_lc_is_review('r_review') && dh_letter_lc_is_review('a_review') && !dh_letter_lc_is_review('lc_admin'), 'P4 review roles');
t_eq(count(dh_letter_lc_roles()), 7, 'P4 seven LC roles');

function tp_letter($over) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => 9900001, 'l_event' => 'Custom', 'l_course_type' => 0,
    'l_name' => 'Zzqtest Letter', 'l_subject' => 'Zzqtest subject', 'l_body' => '<p>Body</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 777100, 'l_updated' => $now, 'l_updated_by' => 777100), $over))->execute();
}

t_in_txn(function () {
  list(, $e) = dh_letter_protect(9900001, 'add', '', array('l_name' => 'Recommending AT Approved', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_body' => 'x'));
  t_ok($e !== '', 'S1 a new letter cannot take a reserved name');
  list(, $e) = dh_letter_protect(9900001, 'copy', '', array('l_name' => 'Zzqtest Copy', 'l_event' => 'R-ATReview', 'l_course_type' => 0, 'l_body' => '[ReviewLink]'));
  t_ok($e !== '', 'S1 a new letter cannot take a review status');
  list($f, $e) = dh_letter_protect(9900001, 'add', '', array('l_name' => 'Zzqtest Ordinary', 'l_event' => 'Received', 'l_course_type' => 0, 'l_body' => 'x'));
  t_ok($e === '' && $f['l_name'] === 'Zzqtest Ordinary', 'S2 an ordinary new letter is untouched');

  $lc = tp_letter(array('l_name' => 'Recommending AT Approved', 'l_event' => 'Custom', 'l_course_type' => 0));
  list($f, $e) = dh_letter_protect(9900001, 'edit', $lc, array('l_name' => 'Zzqtest Hacked', 'l_event' => 'Received', 'l_course_type' => 10, 'l_subject' => 'New subject', 'l_body' => 'x'));
  t_eq(array($e, $f['l_name'], $f['l_event'], $f['l_course_type'], $f['l_subject']), array('', 'Recommending AT Approved', 'Custom', 0, 'New subject'), 'S3 an LC letter keeps its name, status and course type; the subject may change');

  $rv = tp_letter(array('l_name' => 'Zzqtest Review', 'l_event' => 'R-ATReview', 'l_body' => '<p>Click [ReviewLink]</p>'));
  list(, $e) = dh_letter_protect(9900001, 'edit', $rv, array('l_name' => 'Zzqtest Review', 'l_event' => 'R-ATReview', 'l_course_type' => 0, 'l_body' => '<p>No link now</p>'));
  t_ok($e !== '' && strpos($e, '[ReviewLink]') !== FALSE, 'S4 a review letter cannot be saved without [ReviewLink]');
  list(, $e) = dh_letter_protect(9900001, 'edit', $rv, array('l_name' => 'Zzqtest Review', 'l_event' => 'R-ATReview', 'l_course_type' => 0, 'l_body' => '<p>New text [ReviewLink]</p>'));
  t_eq($e, '', 'S4 with [ReviewLink] it saves');
  $old = tp_letter(array('l_name' => 'Zzqtest Old Review', 'l_event' => 'A-ATReview', 'l_body' => '<p>no link</p>'));
  list(, $e) = dh_letter_protect(9900001, 'edit', $old, array('l_subject' => 'Only the subject'));
  t_ok($e !== '', 'S4 a review letter without [ReviewLink] in its stored text cannot be saved either');

  $ord = tp_letter(array('l_name' => 'Zzqtest Ordinary Two', 'l_event' => 'Received'));
  list(, $e) = dh_letter_protect(9900001, 'edit', $ord, array('l_name' => 'LC Admin Notification', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_body' => 'x'));
  t_ok($e !== '', 'S5 an ordinary letter cannot be renamed to a reserved name');
  list(, $e) = dh_letter_protect(9900002, 'edit', $lc, array('l_name' => 'Zzqtest X', 'l_event' => 'Received', 'l_course_type' => 0, 'l_body' => 'x'));
  t_eq($e, '', 'S6 a letter of another centre is not looked up (the caller already refuses it)');

  list(, $e) = dh_letter_protect(9900001, 'add', '', array('l_name' => 'Área AT Approved', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_body' => 'x'));
  t_ok($e !== '', 'S7 a new letter cannot take a reserved name that differs only by an accent (the database finds it)');
  $ord3 = tp_letter(array('l_name' => 'Zzqtest Ordinary Three', 'l_event' => 'Received'));
  list(, $e) = dh_letter_protect(9900001, 'edit', $ord3, array('l_event' => 'R-ATReview ', 'l_course_type' => 0, 'l_body' => '[ReviewLink]'));
  t_ok($e !== '', 'S7 an ordinary letter cannot take the review status with a trailing space (the database finds it)');
  $ord4 = tp_letter(array('l_name' => 'Zzqtest Ordinary Four', 'l_event' => 'Received', 'l_subject' => 'Old subject', 'l_body' => '<p>Old</p>'));
  list($f, $e) = dh_letter_protect(9900001, 'edit', $ord4, array('l_name' => 'Zzqtest Ordinary Four Renamed', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_subject' => 'New subject', 'l_body' => 'x'));
  t_eq(array($e, $f['l_name'], $f['l_event'], $f['l_subject']), array('', 'Zzqtest Ordinary Four Renamed', 'Custom', 'New subject'), 'S8 an ordinary letter edited with a new ordinary name, status and subject saves as posted');
  $rv2 = tp_letter(array('l_name' => 'Zzqtest Review Nine', 'l_event' => 'R-ATReview', 'l_course_type' => 0, 'l_body' => '<p>[ReviewLink]</p>'));
  list($f, $e) = dh_letter_protect(9900001, 'edit', $rv2, array('l_name' => 'Zzqtest Renamed', 'l_event' => 'R-ATReview', 'l_course_type' => 10, 'l_body' => '<p>[ReviewLink]</p>'));
  t_eq(array($e, $f['l_name'], $f['l_course_type']), array('', 'Zzqtest Review Nine', 0), 'S9 a stored review letter keeps its name and course type, whatever is posted');

  $a = tp_letter(array('l_name' => 'Zzqtest Same', 'l_deleted' => 1));
  $b = tp_letter(array('l_name' => 'Zzqtest Same'));
  $c = tp_letter(array('l_name' => 'Zzqtest Same'));
  t_eq(dh_letter_by_name(9900001, 'Zzqtest Same'), $b, 'N1 by name: the lowest-id ACTIVE letter');
  t_eq(dh_letter_by_name(9900001, 'Zzqtest None'), 0, 'N1 no letter gives 0');
  t_eq(dh_letter_by_name(9900002, 'Zzqtest Same'), 0, 'N1 only that centre');

  // S10: the reserved-name check also works when the connection is not utf8mb4 (a utf8mb3 connection)
  db_query("SET NAMES utf8");
  try {
    t_ok(in_array(db_query("select @@character_set_connection")->fetchField(), array('utf8', 'utf8mb3')), 'S10 the test connection really is utf8mb3');
    list(, $e) = dh_letter_protect(9900001, 'add', '', array('l_name' => 'Área AT Approved', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_body' => 'x'));
    t_ok($e !== '', 'S10 on a utf8mb3 connection a reserved name that differs only by an accent is still refused');
    list(, $e) = dh_letter_protect(9900001, 'add', '', array('l_name' => 'Zzqtest Fine Name', 'l_event' => 'R-ATReview ', 'l_course_type' => 0, 'l_body' => '[ReviewLink]'));
    t_ok($e !== '', 'S10 and the review status with a trailing space is still refused');
    list(, $e) = dh_letter_protect(9900001, 'add', '', array('l_name' => 'Zzqtest Fine Name', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_body' => 'x'));
    t_eq($e, '', 'S10 and an ordinary letter still saves');
  } finally {
    db_query("SET NAMES utf8mb4");
  }
});
t_done();
