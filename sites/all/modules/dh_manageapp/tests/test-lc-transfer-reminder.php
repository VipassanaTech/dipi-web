<?php
// The daily reminder to centres about long-course applications waiting in R-ATTransfer (dh_lc_transfer_reminders,
// inc/course.inc): who gets it (LC admin email, else the centre's reply-to address), which applications are listed
// (R-ATTransfer, course not started, not cancelled), and what each line says. Builds the messages only (nothing is
// sent); made-up data in a transaction that is rolled back.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-transfer-reminder.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $LC = 13;
  $mk_centre = function ($lc_admin, $reply_to) {
    $c = db_query("select * from dh_center where c_id > 0 order by c_id desc limit 1")->fetchAssoc();
    unset($c['c_id']);
    $id = (int) db_insert('dh_center')->fields(array_merge($c, array('c_name' => 'Zzqtest Centre ' . mt_rand(100000, 999999), 'c_email' => '', 'c_lc_admin_email' => $lc_admin)))->execute();
    $s = db_query("select * from dh_center_setting where cs_center > 0 order by cs_center desc limit 1")->fetchAssoc();
    if ($s) {
      foreach (array('cs_id') as $k) unset($s[$k]);
      db_insert('dh_center_setting')->fields(array_merge($s, array('cs_center' => $id, 'cs_email_reply_to' => $reply_to)))->execute();
    }
    return $id;
  };
  $c1 = $mk_centre('zzqtest-lcadmin@example.invalid', 'zzqtest-reply1@example.invalid');
  $c2 = $mk_centre('', 'zzqtest-reply2@example.invalid');
  $c3 = $mk_centre('', '');
  $course = function ($centre, $start, $over = array()) use ($LC) {
    return t_make_course(array_merge(array('c_center' => $centre, 'c_course_type' => $LC, 'c_start' => $start, 'c_end' => date('Y-m-d', strtotime($start . ' +45 days')), 'c_processed' => 0, 'c_finalize_mail' => 0), $over));
  };
  $future = date('Y-m-d', strtotime('+30 days'));
  $k1 = $course($c1, $future); $k1past = $course($c1, date('Y-m-d', strtotime('-5 days'))); $k1canc = $course($c1, $future, array('c_cancelled' => 1));
  $k2 = $course($c2, $future); $k3 = $course($c3, $future);
  $app = function ($course, $status, $first, $reco, $days_ago = NULL) {
    $id = t_make_applicant($course, array('a_attended' => 0, 'a_status' => $status, 'a_f_name' => $first, 'a_m_name' => '', 'a_l_name' => 'Person'));
    db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute();
    db_insert('dh_applicant_lc')->fields(array('al_applicant' => $id, 'al_recommending' => $reco, 'al_recommending_approved' => 'Transfer to Registrar', 'al_area_at' => '', 'al_area_at_approved' => 'Pending'))->execute();
    if ($days_ago !== NULL)
      db_insert('dh_log')->fields(array('l_center' => 0, 'l_module' => 'Applicant', 'l_identifier' => (string) $id, 'l_event' => 'Status Change',
        'l_msg' => 'Status changed from R-ATReview To R-ATTransfer', 'l_user' => 453, 'l_tstamp' => date('Y-m-d H:i:s', strtotime("-$days_ago days -1 hour"))))->execute();
    return $id;
  };
  $a1 = $app($k1, 'R-ATTransfer', 'Zzqone', 'Zzq Recoteacher', 3);
  $a2 = $app($k1, 'R-ATTransfer', 'Zzqtwo', '', 1);
  $app($k1, 'R-ATReview', 'Zzqreview', 'Zzq Recoteacher');      // not waiting
  $app($k1past, 'R-ATTransfer', 'Zzqpast', 'Zzq Recoteacher', 9); // course started
  $app($k1canc, 'R-ATTransfer', 'Zzqcanc', 'Zzq Recoteacher', 2); // course cancelled
  $b1 = $app($k2, 'R-ATTransfer', 'Zzqthree', 'Zzq Othereco', 10);
  $app($k3, 'R-ATTransfer', 'Zzqnoaddr', 'Zzq Recoteacher', 4);     // centre with no address at all

  $msgs = array();
  foreach (dh_lc_transfer_reminders() as $m)
    if (in_array($m['centre'], array($c1, $c2, $c3)))
      $msgs[$m['centre']] = $m;
  t_eq(array_keys($msgs), array($c1, $c2), 'R1 one message per centre with waiting applications; a centre with no address is skipped');
  t_eq(array($msgs[$c1]['to'], $msgs[$c2]['to']), array('zzqtest-lcadmin@example.invalid', 'zzqtest-reply2@example.invalid'), 'R2 to the LC admin email, else the centre reply-to address');
  t_eq(array($msgs[$c1]['count'], $msgs[$c2]['count']), array(2, 1), 'R3 only R-ATTransfer in courses not started and not cancelled');
  $t = $msgs[$c1]['text'];
  t_ok(strpos($t, "- Zzqone Person (application $a1), ") !== FALSE && strpos($t, ': returned from Zzq Recoteacher, waiting 3 days') !== FALSE, 'R4 a line: name, application number, course, the teacher it came back from, days waiting');
  t_ok(strpos($t, "- Zzqtwo Person (application $a2), ") !== FALSE && strpos($t, 'waiting 1 day') !== FALSE && strpos($t, 'Zzqtwo Person (application ' . $a2 . '), Zzqtest Centre') !== FALSE, 'R5 no teacher name when there is none; "1 day"');
  t_ok(strpos($t, 'These 2 long course applications were returned to the centre without approval from the Recommending AT') !== FALSE
    && strpos($msgs[$c2]['text'], 'This long course application was returned') !== FALSE, 'R6 the opening sentence (several / one)');
  t_eq($msgs[$c1]['subject'], 'DIPI: long course applications waiting for a new Recommending AT (2)', 'R7 subject with the number');
  t_ok(strpos($t, 'Zzqpast') === FALSE && strpos($t, 'Zzqcanc') === FALSE && strpos($t, 'Zzqreview') === FALSE, 'R8 started, cancelled and in-review applications are not listed');
});
t_done();
