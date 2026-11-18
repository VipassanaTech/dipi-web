<?php
// Step 1 move of referral person details (inc/referral-move.inc): details, links (two passes), dry runs, re-runs, rollback.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-move.php
require __DIR__ . '/lib.php';
require_once DRUPAL_ROOT . '/sites/all/modules/dh_manageapp/inc/referral-move.inc';
dh_refmove_prepare();                      // DDL commits: outside the test transaction

t_in_txn(function () {
  $course = t_make_course();
  $centre = db_query('select c_center from dh_course where c_id = :c', array(':c' => $course))->fetchField();
  $ref = function ($sid, $over = array()) {
    return (int) db_insert('dh_referral')->fields($over + array('r_student' => $sid, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
      'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_listed_10d' => 1, 'r_created_by' => 0, 'r_updated_by' => 0))->execute();
  };
  $sci = function ($sid, $n10) { db_insert('dh_student_course_input')->fields(array('sci_student' => $sid, 'sci_10d' => $n10))->execute(); };
  $p1 = array('s_f_name' => 'Zzqtest', 's_l_name' => 'Movetwin', 's_email' => 'zzqmove1@example.invalid');
  $real = t_make_student($p1);                                                   // the person's real record
  $c1 = t_make_student($p1 + array('s_phone_mobile' => '9811111111', 's_dob' => '1981-01-01'));
  $sci($c1, 4); $sci($c1, 7);
  $r1 = $ref($c1);                                                               // group twin
  $p2 = array('s_f_name' => 'Zzqtest', 's_l_name' => 'Moveown', 's_email' => 'zzqmove2@example.invalid');
  $c2 = t_make_student($p2);
  db_insert('dh_student_course')->fields(array('sc_student' => $c2, 'sc_course' => $course, 'sc_center' => $centre))->execute();
  $r2 = $ref($c2);                                                               // group own
  $c4 = t_make_student($p2);
  $r4 = $ref($c4);                                                               // same person referred twice: pass 2 -> $c2
  $c3 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Movenone', 's_email' => 'zzqmove3@example.invalid'));
  $r3 = $ref($c3);                                                               // group none
  $c5 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Movedel', 's_email' => 'zzqmove5@example.invalid'));
  $r5 = $ref($c5, array('r_deleted' => 1));                                      // deleted: moved too
  $ids = array($r1, $r2, $r3, $r4, $r5);
  $get = function ($r) { return db_query('select * from dh_referral where r_id = :r', array(':r' => $r))->fetchAssoc(); };
  $moved = function () use ($ids) { return (int) db_query("select count(*) from dh_referral_log where rl_event = 'Moved' and rl_referral in (:r)", array(':r' => $ids))->fetchField(); };

  $d = dh_refmove_details(TRUE, $ids);
  t_eq(array($d['to_move'], $d['no_copy']), array(5, 0), 'D1 the dry run counts 5 to move');
  t_eq($get($r1)['r_f_name'], NULL, 'D2 the dry run wrote nothing');
  dh_refmove_details(FALSE, $ids);
  $g1 = $get($r1);
  t_eq(array($g1['r_f_name'], $g1['r_l_name'], $g1['r_email'], $g1['r_phone_mobile'], $g1['r_dob'], $g1['r_cnt_10d']),
    array('Zzqtest', 'Movetwin', 'zzqmove1@example.invalid', '9811111111', '1981-01-01', 4), 'D3 details and the lowest count row are copied');
  t_ok($g1['r_moved'] !== NULL, 'D4 r_moved is set');
  t_eq(db_query('select old_student from dh_referral_move_bak where r_id = :r', array(':r' => $r1))->fetchField(), $c1, 'D5 the old r_student is backed up');
  t_eq(dh_refmove_details(FALSE, $ids)['to_move'], 0, 'D6 a second run moves nothing');

  // F3: edits made to a copy by the OLD code after the details run are copied in again by details --refresh.
  db_update('dh_student')->fields(array('s_phone_mobile' => '9833333333'))->condition('s_id', $c1)->execute();
  db_update('dh_referral')->fields(array('r_updated' => date('Y-m-d H:i:s', time() + 30)))->condition('r_id', $r1)->execute();
  db_update('dh_student')->fields(array('s_phone_mobile' => '9844444444'))->condition('s_id', $c5)->execute();
  db_update('dh_referral')->fields(array('r_student' => 0))->condition('r_id', $r5)->execute();      // links already ran for this one
  $moved5 = $get($r5)['r_moved'];
  t_eq(dh_refmove_details(TRUE, $ids, TRUE)['to_refresh'], 4, 'H1 the refresh dry run counts the 4 still on their copy');
  t_eq($get($r1)['r_phone_mobile'], '9811111111', 'H2 the dry run wrote nothing');
  dh_refmove_details(FALSE, $ids, TRUE);
  t_eq(array($get($r1)['r_phone_mobile'], $get($r5)['r_phone_mobile'], $get($r5)['r_moved']), array('9833333333', NULL, $moved5),
    'H3 refresh copies the edited copy in; a referral no longer on its copy is not refreshed');
  db_update('dh_referral')->fields(array('r_student' => $c5))->condition('r_id', $r5)->execute();
  t_eq(dh_refmove_details(FALSE, $ids)['to_move'], 0, 'H4 without --refresh nothing is moved again');

  // F4: a zero month or day in the copy's date of birth gives no date, not a failed update.
  $c8 = t_make_student(array('s_f_name' => 'Zzqtest', 's_l_name' => 'Movezero', 's_email' => 'zzqmove8@example.invalid'));
  $mode = db_query('select @@session.sql_mode')->fetchField();
  db_query("set session sql_mode = ''");
  $zero = TRUE;
  try { db_query("update dh_student set s_dob = '1980-05-00' where s_id = :s", array(':s' => $c8)); } catch (Exception $e) { $zero = FALSE; }
  db_query('set session sql_mode = :m', array(':m' => $mode));
  if ($zero && db_query('select dayofmonth(s_dob) from dh_student where s_id = :s', array(':s' => $c8))->fetchField() === '0') {
    $r8 = $ref($c8);
    db_query("set session sql_mode = 'STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE'");       // as Drupal's session: a zero day makes an UPDATE fail
    try { dh_refmove_details(FALSE, array($r8)); } finally { db_query('set session sql_mode = :m', array(':m' => $mode)); }
    t_eq(array($get($r8)['r_dob'], $get($r8)['r_moved'] !== NULL), array(NULL, TRUE), 'Z1 a copy date of birth with day 00 moves as no date');
  } else {
    echo "  skip Z1 (this database refuses a zero day in a date even with an empty sql_mode)\n";
  }

  // F2: a referral NOT in the backup, linked to an old copy in the pull -> links window, is linked afresh by links.
  $r7 = $ref($c3, array('r_moved' => date('Y-m-d H:i:s')));
  $lids = array_merge($ids, array($r7));
  $dr = dh_refmove_links(TRUE, $lids);
  t_eq($dr['recheck_candidates'], 1, 'W1 the links dry run reports the referral to re-check');
  dh_refmove_links(TRUE, $ids);
  t_eq(array((int) $get($r1)['r_student'], (int) db_query('select count(*) from dh_referral_copy where rc_student in (:s)', array(':s' => array($c1, $c3, $c4, $c5)))->fetchField()),
    array($c1, 0), 'K1 the links dry run wrote nothing');
  $ln = dh_refmove_links(FALSE, $lids);
  t_eq(array($ln['rechecked'], (int) $get($r7)['r_student']), array(1, 0), 'W2 links re-checks it: no longer pointing at the old copy');
  t_eq(array((int) $get($r1)['r_student'], (int) $get($r2)['r_student'], (int) $get($r3)['r_student'], (int) $get($r4)['r_student'], (int) $get($r5)['r_student']),
    array($real, $c2, 0, $c2, 0), 'K2 twin -> real record, own -> its copy, none -> 0, referred twice -> the own copy, deleted none -> 0');
  $rc = db_query('select rc_student, rc_group, rc_status from dh_referral_copy where rc_student in (:s)', array(':s' => array($c1, $c2, $c3, $c4, $c5)))->fetchAllAssoc('rc_student', PDO::FETCH_ASSOC);
  t_eq(array_map(function ($x) { return $x['rc_group'] . '/' . $x['rc_status']; }, $rc),
    array($c1 => 'twin/open', $c3 => 'none/open', $c4 => 'twin/open', $c5 => 'none/open'), 'K3 copies recorded as open; the own copy is not');
  t_eq($moved(), 5, 'K4 one Moved line per referral');
  $again = dh_refmove_links(FALSE, $lids);
  t_eq(array($again['links_changed'], $again['copies_recorded'], $moved()), array(0, 0, 5), 'K5 a second run changes nothing');
  t_eq(dh_referral_link_find(dh_referral_who_row($get($r3))), 0, 'K6 an open copy is no longer found');

  $r6 = $ref(0, array('r_f_name' => 'Zzqtest', 'r_l_name' => 'Movenew', 'r_cnt_10d' => 2, 'r_photo' => 'private://zzq/move6.jpg', 'r_moved' => date('Y-m-d H:i:s')));
  db_update('dh_referral')->fields(array('r_phone_mobile' => '9822222222', 'r_updated' => date('Y-m-d H:i:s', time() + 60)))->condition('r_id', $r3)->execute();
  $ids[] = $r6;
  dh_refmove_rollback(FALSE, $ids);
  t_eq(array((int) $get($r1)['r_student'], (int) $get($r3)['r_student']), array($c1, $c3), 'B1 rollback points referrals at their copies again');
  $s6 = (int) $get($r6)['r_student'];
  t_ok($s6 > 0 && db_query('select s_l_name from dh_student where s_id = :s', array(':s' => $s6))->fetchField() === 'Movenew', 'B2 a referral added after the move gets a copy');
  t_eq(db_query('select sci_10d from dh_student_course_input where sci_student = :s', array(':s' => $s6))->fetchField(), 2, 'B3 with its course counts');
  t_eq(db_query('select s_phone_mobile from dh_student where s_id = :s', array(':s' => $c3))->fetchField(), '9822222222', 'B4 an edit after the move goes back into the copy');
  t_eq(db_query("select count(*) from dh_referral_copy where rc_student in (:s) and rc_status = 'open'", array(':s' => array($c1, $c3, $c4, $c5)))->fetchField(), 0, 'B5 the open copy rows are removed');
  t_eq(db_query('select s_photo from dh_student where s_id = :s', array(':s' => $s6))->fetchField(), 'private://zzq/move6.jpg', 'B6 the new copy keeps the photo');
  dh_refmove_rollback(FALSE, $ids);
  t_eq(array((int) db_query("select count(*) from dh_student where s_l_name = 'Movenew'")->fetchField(), (int) $get($r6)['r_student']), array(1, $s6), 'B7 a second rollback adds no second copy');
});
t_done();
