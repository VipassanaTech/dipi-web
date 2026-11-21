<?php
// A long-course application can be marked attended only when both reviews (Recommending AT and Area AT) are Approved:
// students, servers and teachers alike (a teacher's own application is approved at both steps by the AT Portal).
// Checked in the Zero Day popup (dh_app_update_attended) and the application Edit / Add form (Attending Course? = Yes).
// Someone already attended is not checked again; other courses are not affected.
// Zero Day saves run in their own PHP process (the page answers with JSON and exits), on made-up courses and
// applicants that are deleted at the end.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-attend-block.php
require __DIR__ . '/lib.php';

$LC = 13;      // 45-DayOSC (td_val3 = 'LC')
$TEN = 6;      // 10-Day
$runner = tempnam(sys_get_temp_dir(), 'zzq-lc-attend-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$in = json_decode($argv[1], TRUE); $GLOBALS["user"] = user_load(1); $_GET["q"] = "app-update-attended/" . (int) $in["app"];
$_REQUEST = $_POST = $in["req"]; dh_app_update_attended();
');
$made = array('course' => array(), 'app' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_log' => $max('dh_log', 'l_id'));
try {
  // Running courses (started yesterday), so Zero Day needs no room.
  $run = array('c_start' => date('Y-m-d', strtotime('-1 day')), 'c_end' => date('Y-m-d', strtotime('+40 days')), 'c_processed' => 0, 'c_finalize_mail' => 0);
  $lc = $made['course'][] = t_make_course(array_merge($run, array('c_course_type' => $LC)));
  $ten = $made['course'][] = t_make_course(array_merge($run, array('c_course_type' => $TEN)));
  // A not-yet-attended applicant (no attended row), with its review row when $reco is given.
  $new = function ($course, $reco = NULL, $area = NULL, $over = array()) use (&$made) {
    $id = t_make_applicant($course, array_merge(array('a_attended' => 0, 'a_email' => 'zzqtest-lca@example.invalid'), $over));
    db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute();
    if ($reco !== NULL)
      db_insert('dh_applicant_lc')->fields(array('al_applicant' => $id, 'al_recommending' => 'Zzq Reco', 'al_recommending_approved' => $reco,
        'al_area_at' => 'Zzq Area', 'al_area_at_approved' => $area))->execute();
    $made['app'][] = $id;
    return $id;
  };
  $save = function ($id, $attend = 'true') use ($runner) {
    $req = array('a' => $attend, 's' => '', 'r' => '', 'g' => '1', 'l' => '', 'v' => '', 'c' => '', 'cf' => '0', 'gc' => '', 'gcf' => '0',
      'dn' => '', 'df' => '0', 'gdn' => '', 'gdf' => '0', 'chow' => '0', 'chai' => '0', 'back' => '0', 'comment' => '');
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array('app' => $id, 'req' => $req))) . ' 2>/dev/null');
    $j = json_decode(substr($out, strpos($out, '{')), TRUE);
    return is_array($j) ? (!empty($j['status']) ? 'ok' : (string) $j['msg']) : 'no reply';
  };
  $state = function ($id) {
    return db_query("select a_status, a_attended, (select count(*) from dh_applicant_attended where aa_applicant = a_id) aa from dh_applicant where a_id = :a",
      array(':a' => $id))->fetchAssoc();
  };
  $msg = 'Not marked attended: this long-course application needs both reviews approved first. Recommending AT (Zzq Reco): %r. Area AT (Zzq Area): %a. '
    . 'The teachers approve it by the review email or the AT Portal, or the registrar can choose Approved in Change Status on the search page.';
  $want = function ($r, $a) use ($msg) { return str_replace(array('%r', '%a'), array($r, $a), $msg); };

  // --- the check itself
  $pp = $new($lc, 'Pending', 'Pending', array('a_status' => 'R-ATReview'));
  t_eq(dh_lc_attend_block($pp), $want('Pending', 'Pending'), 'B1 both reviews Pending: refused, says who and what to do');
  $ap = $new($lc, 'Approved', 'Pending', array('a_status' => 'A-ATReview'));
  t_eq(dh_lc_attend_block($ap), $want('Approved', 'Pending'), 'B2 only the Recommending AT approved: refused');
  $rj = $new($lc, 'Rejected', 'Pending', array('a_status' => 'Rejected'));
  t_eq(dh_lc_attend_block($rj), $want('Rejected', 'Pending'), 'B3 rejected: refused');
  $no = $new($lc);
  t_eq(dh_lc_attend_block($no), 'Not marked attended: this long-course application needs both reviews approved first. Recommending AT (not chosen): Pending. '
    . 'Area AT (not chosen): Pending. The teachers approve it by the review email or the AT Portal, or the registrar can choose Approved in Change Status on the search page.',
    'B4 no review details at all: refused');
  $ok = $new($lc, 'Approved', 'Approved', array('a_status' => 'Confirmed'));
  t_eq(dh_lc_attend_block($ok), '', 'B5 both approved: allowed');
  $srv = $new($lc, 'Pending', 'Pending', array('a_status' => 'Confirmed', 'a_type' => 'Sevak'));
  t_eq(dh_lc_attend_block($srv), $want('Pending', 'Pending'), 'B6 a server needs both approvals too (even when already Confirmed)');
  $self = $new($lc, 'Approved', 'Approved', array('a_status' => 'Confirmed'));
  db_update('dh_applicant_lc')->fields(array('al_area_at' => 'Not Applicable', 'al_area_at_comments' => 'Self approved by AT.'))->condition('al_applicant', $self)->execute();
  t_eq(dh_lc_attend_block($self), '', 'B7 a teacher\'s self-approved application (both Approved, Area AT "Not Applicable"): allowed');
  $old = t_make_applicant($lc, array('a_email' => 'zzqtest-lca@example.invalid'));   // attended, with an attended row
  $made['app'][] = $old;
  db_insert('dh_applicant_lc')->fields(array('al_applicant' => $old, 'al_recommending_approved' => 'Pending', 'al_area_at_approved' => 'Pending'))->execute();
  t_eq(dh_lc_attend_block($old), '', 'B8 already attended: not checked again');
  $tenapp = $new($ten, NULL, NULL, array('a_status' => 'Confirmed'));
  t_eq(dh_lc_attend_block($tenapp), '', 'B9 a 10-day course is not affected');
  t_eq(dh_lc_attend_block(0, $lc), 'Not marked attended: a new long-course application needs both reviews approved first. '
    . 'Save it with Attending Course? = No; it then goes to the teachers for review.', 'B10 a new application for a long course: refused');
  t_eq(dh_lc_attend_block(0, $ten), '', 'B11 a new application for a 10-day course: allowed');
  $esc = $new($lc, 'Pending', 'Pending');
  db_update('dh_applicant_lc')->fields(array('al_recommending' => 'A <b>&'))->condition('al_applicant', $esc)->execute();
  t_ok(strpos(dh_lc_attend_block($esc), 'Recommending AT (A &lt;b&gt;&amp;)') !== FALSE, 'B12 names are escaped (the message is shown as HTML)');

  // --- Zero Day popup
  t_eq($save($pp), $want('Pending', 'Pending'), 'Z1 Zero Day: both Pending is refused with the message');
  t_eq($state($pp), array('a_status' => 'R-ATReview', 'a_attended' => '0', 'aa' => '0'), 'Z2 ... and nothing changed (still R-ATReview, not attended, no Confirmed)');
  t_eq($save($srv), $want('Pending', 'Pending'), 'Z3 Zero Day: a Confirmed server with reviews Pending is refused');
  t_eq($save($ok), 'ok', 'Z4 Zero Day: both approved is marked attended');
  t_eq($state($ok), array('a_status' => 'Confirmed', 'a_attended' => '1', 'aa' => '1'), 'Z5 ... attended, Confirmed');
  t_eq($save($old), 'ok', 'Z6 Zero Day: someone already attended (reviews Pending) can still be changed');
  t_eq($save($tenapp), 'ok', 'Z7 Zero Day: a 10-day course applicant is marked attended as before');
  t_eq($save($pp, 'false'), 'ok', 'Z8 Zero Day: un-attending is never blocked');

  // --- application Edit / Add form (validate)
  $validate = function ($path, $attending) {
    drupal_static_reset('form_set_error');
    $_GET['q'] = $path;
    $form = array(); $fs = array('input' => array('a_gender' => 'M', 'ac_teacher_code' => '', 'a_email' => 'zzqtest-lca@example.invalid', 'attending' => $attending));
    dh_ma_applicant_form_validate($form, $fs);
    $e = form_get_errors();
    drupal_static_reset('form_set_error');
    return $e ? $e : array();
  };
  t_eq($validate("app/$pp/edit", '1'), array('attending' => $want('Pending', 'Pending')), 'E1 Edit, Attending = Yes, reviews Pending: refused at the Attending field');
  t_eq($validate("app/$pp/edit/nojs", '1'), array('attending' => $want('Pending', 'Pending')), 'E2 same from the popup edit (app/{id}/edit/nojs)');
  t_eq($validate("app/$pp/edit", '0'), array(), 'E3 Edit, Attending = No: saves as before');
  t_eq($validate("app/$ok/edit", '1'), array(), 'E4 Edit, both approved: allowed');
  t_eq($validate("app/$old/edit", '1'), array(), 'E5 Edit, already attended: allowed (no new check)');
  t_eq($validate("app/$tenapp/edit", '1'), array(), 'E6 Edit, 10-day course: allowed');
  t_eq($validate("app/add/0/$lc", '1'), array('attending' => 'Not marked attended: a new long-course application needs both reviews approved first. '
    . 'Save it with Attending Course? = No; it then goes to the teachers for review.'), 'E7 Add for a long course with Attending = Yes: refused');
  t_eq($validate("app/add/0/$lc", '0'), array(), 'E8 Add for a long course with Attending = No: allowed');
  t_eq($validate("app/add/0/$ten", '1'), array(), 'E9 Add for a 10-day course with Attending = Yes: allowed');
}
finally {
  if ($made['app']) {
    foreach (array('dh_applicant_attended' => 'aa_applicant', 'dh_applicant_lc' => 'al_applicant', 'dh_application_tag' => 'at_application',
        'dh_application_tag_log' => 'atl_application', 'dh_applicant' => 'a_id') as $t => $col)
      db_delete($t)->condition($col, $made['app'], 'IN')->execute();
    db_query("delete from dh_log where l_id > :l and l_identifier in (:a)", array(':l' => $start['dh_log'], ':a' => array_map('strval', $made['app'])));
  }
  $newtags = db_query("select t_id from dh_tag where t_id > :t", array(':t' => $start['dh_tag']))->fetchCol();
  if ($newtags) {
    db_delete('dh_tag_log')->condition('tl_tag', $newtags, 'IN')->execute();
    db_delete('dh_tag')->condition('t_id', $newtags, 'IN')->execute();
  }
  foreach ($made['course'] as $c) { db_delete('dh_course')->condition('c_id', $c)->execute(); db_delete('dh_course_log')->condition('crl_course', $c)->execute(); }
  @unlink($runner);
  echo '  cleanup: ' . count($made['app']) . ' made-up applicants, ' . count($made['course']) . ' courses, ' . count($newtags) . " new system tags removed\n";
}
t_ok((int) db_query("select count(*) from dh_applicant where a_email = 'zzqtest-lca@example.invalid'")->fetchField() === 0, 'C1 nothing left behind');
t_done();
