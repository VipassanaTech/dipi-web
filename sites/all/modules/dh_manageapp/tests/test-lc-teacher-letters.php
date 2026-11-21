<?php
// The student's teacher-change letters (21 Nov 2026): "Recommending AT Changed", "Area AT Changed" and
// "Recommending AT Returned" (with a one-time [ChooseTeacherLink]). Which letter goes when, and what it says.
// Every action runs in its own PHP process like the page, with letters RECORDED instead of sent
// ($GLOBALS['dh_letter_capture'], in dh_send_letter), on a made-up centre that gets a copy of the template's (centre 0)
// letters and letter fields, made-up teachers and applicants; all deleted at the end.
// Needs the three letters at centre 0 (deploy/2026-11-21-lc-teacher-change-letters.php --apply on local).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-teacher-letters.php
require __DIR__ . '/lib.php';

$LC = 13;      // 45-DayOSC
$LINK = 'https://zzqtest.invalid/choose?c=';
$runner = tempnam(sys_get_temp_dir(), 'zzq-teacher-letters-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$GLOBALS["conf"]["mailgun_key"] = "zzq-no-mail"; $GLOBALS["conf"]["dh_lc_choose_link"] = ' . var_export($LINK, TRUE) . ';
$GLOBALS["dh_letter_capture"] = array();
register_shutdown_function(function () { echo "\nCAPTURE:" . json_encode($GLOBALS["dh_letter_capture"]) . "\n"; });
$in = json_decode($argv[1], TRUE); $GLOBALS["user"] = user_load(1); $GLOBALS["user"]->name = $in["name"]; $id = (int) $in["app"];
switch ($in["do"]) {
  case "portal": $_GET["q"] = "lc-review/app/" . $id; $form = array(); $fs = array("input" => $in["req"]);
    dh_atportal_review_form_validate($form, $fs); if (!form_get_errors()) dh_atportal_review_form_submit($form, $fs); break;
  case "status": update_status_external($id, $in["status"]); break;
  case "review_mail": dh_send_letter("r-teacher", $id, "R-ATReview"); break;
  case "edit_letters": dh_lc_edit_teacher_letters($id, $in["prev_reco"], $in["prev_area"]); break;
}
');
$made = array('centre' => 0, 'course' => array(), 'app' => array(), 'teacher' => array(), 'letter' => array(), 'field' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_log' => $max('dh_log', 'l_id'), 'dh_letter_log' => $max('dh_letter_log', 'll_id'));
try {
  foreach (array('Recommending AT Changed', 'Area AT Changed', 'Recommending AT Returned') as $n)
    if (!dh_letter_by_name(0, $n)) throw new Exception("centre 0 has no '$n' letter: run deploy/2026-11-21-lc-teacher-change-letters.php --apply first");
  $teacher = function ($code, $g, $last, $email = TRUE) use (&$made) {
    $made['teacher'][] = db_insert('dh_teacher')->fields(array('t_code' => $code, 't_gender' => $g, 't_f_name' => 'Zzq', 't_l_name' => $last, 't_status' => 'Active',
      't_email' => $email ? 'zzqtest-' . strtolower($last) . '@example.invalid' : '', 't_created_by' => 0, 't_updated_by' => 0))->execute();
  };
  $teacher('ZZQ9', 'M', 'Recoteacher'); $teacher('ZZQ7', 'M', 'Othereco'); $teacher('ZZQ6', 'M', 'Noemail', FALSE);
  $teacher('ZZQ8', 'F', 'Areateacher'); $teacher('ZZQ5', 'F', 'Otherarea');
  // made-up centre (no emails) with a copy of the template's letters and letter fields
  $c = db_query("select * from dh_center where c_id > 0 order by c_id desc limit 1")->fetchAssoc();
  unset($c['c_id']);
  $made['centre'] = $cen = (int) db_insert('dh_center')->fields(array_merge($c, array('c_name' => 'Zzqtest Centre ' . mt_rand(100000, 999999), 'c_email' => '', 'c_lc_admin_email' => '')))->execute();
  foreach (db_query("select * from dh_letter where l_center = 0 and l_deleted = 0")->fetchAll(PDO::FETCH_ASSOC) as $l) {
    unset($l['l_id']); $l['l_center'] = $cen; $l['l_created'] = $l['l_updated'] = date('Y-m-d H:i:s');
    $made['letter'][] = db_insert('dh_letter')->fields($l)->execute();
  }
  foreach (db_query("select * from dh_letter_fields where lf_center = 0 and lf_deleted = 0")->fetchAll(PDO::FETCH_ASSOC) as $f) {
    unset($f['lf_id']); $f['lf_center'] = $cen;
    $made['field'][] = db_insert('dh_letter_fields')->fields($f)->execute();
  }
  $lid = function ($name) use ($cen) { return dh_letter_by_name($cen, $name); };
  $lc = $made['course'][] = t_make_course(array('c_center' => $cen, 'c_course_type' => $LC, 'c_start' => date('Y-m-d', strtotime('+20 days')), 'c_end' => date('Y-m-d', strtotime('+65 days')), 'c_processed' => 0, 'c_finalize_mail' => 0));
  $new = function ($status, $reco_at, $reco, $area_at, $area) use ($lc, &$made) {
    $id = t_make_applicant($lc, array('a_attended' => 0, 'a_status' => $status, 'a_email' => 'zzqtest-tl@example.invalid', 'a_updated_by' => 0));
    db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute();
    db_insert('dh_applicant_lc')->fields(array('al_applicant' => $id, 'al_recommending' => $reco_at, 'al_recommending_approved' => $reco, 'al_area_at' => $area_at, 'al_area_at_approved' => $area))->execute();
    $made['app'][] = $id;
    return $id;
  };
  $run = function ($do, $id, $extra = array()) use ($runner) {
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array_merge(array('do' => $do, 'app' => $id, 'name' => 'zzq9.m'), $extra))) . ' 2>/dev/null');
    return preg_match('/CAPTURE:(.*)/', $out, $m) ? json_decode($m[1], TRUE) : 'no capture: ' . substr($out, 0, 200);
  };
  $sent = function ($cap) { return is_array($cap) ? array_map(function ($c) { return $c['type'] . ':' . ($c['letter_id'] ? $c['letter_id'] : $c['event']); }, $cap) : $cap; };
  $applicant_letter = function ($cap) { foreach ((array) $cap as $c) if ($c['type'] == 'applicant') return $c; return array('body' => '', 'sms' => '', 'letter_id' => 0); };
  $status = function ($id) { return db_query("select a_status from dh_applicant where a_id = :a", array(':a' => $id))->fetchField(); };

  // --- AT Portal: the Recommending AT passes it to another one
  $a1 = $new('R-ATReview', 'Zzq Recoteacher', 'Pending', '', 'Pending');
  $cap = $run('portal', $a1, array('req' => array('action' => 'TransferReco', 'transfer_reco_teacher' => 'Zzq Othereco', 'comments' => 'x', 'area_teacher' => '')));
  t_eq($sent($cap), array('r-teacher:' . $lid('Recommending Review'), 'applicant:' . $lid('Recommending AT Changed')), 'L1 Recommending AT transfer: review request to the new AT, then "Recommending AT Changed" to the student');
  $l = $applicant_letter($cap);
  t_ok(strpos($l['body'], 'reassigned from the Recommending Assistant Teacher Zzq Recoteacher to the Recommending Assistant Teacher Zzq Othereco for review') !== FALSE
    && strpos($l['sms'], 'from the Recommending Assistant Teacher Zzq Recoteacher to Zzq Othereco') !== FALSE, 'L2 ... it names the former and the new teacher (email and SMS)');

  // --- AT Portal: the Area AT passes it to another one
  $a2 = $new('A-ATReview', 'Zzq Recoteacher', 'Approved', 'Zzq Areateacher', 'Pending');
  $cap = $run('portal', $a2, array('name' => 'zzq8.f', 'req' => array('action' => 'Transfer', 'transfer_area_teacher' => 'Zzq Otherarea', 'comments' => 'y')));
  t_eq($sent($cap), array('a-teacher:' . $lid('Area Teacher Review'), 'applicant:' . $lid('Area AT Changed')), 'L3 Area AT transfer: review request to the new Area AT, then "Area AT Changed" (no more "Recommending AT Approved")');
  t_ok(strpos($applicant_letter($cap)['body'], 'reassigned from the Area Teacher Zzq Areateacher to the Area Teacher Zzq Otherarea for review') !== FALSE, 'L4 ... it names both Area teachers');

  // --- returned to the centre (the review email link's "Transfer"), then the link and the code
  $a3 = $new('R-ATReview', 'Zzq Recoteacher', 'Pending', '', 'Pending');
  db_update('dh_applicant_lc')->fields(array('al_recommending_approved' => 'Transfer to Registrar'))->condition('al_applicant', $a3)->execute();
  $cap = $run('status', $a3, array('status' => 'R-ATTransfer'));
  t_eq(array($status($a3), $sent($cap)), array('R-ATTransfer', array('applicant:' . $lid('Recommending AT Returned'))), 'L5 returned by the teacher: R-ATTransfer and "Recommending AT Returned" to the student');
  $l = $applicant_letter($cap);
  $code = db_query("select al_choose_auth from dh_applicant_lc where al_applicant = :a", array(':a' => $a3))->fetchField();
  t_ok(strpos($l['body'], 'returned to the centre without approval from your Recommending Assistant Teacher Zzq Recoteacher') !== FALSE, 'L6 ... "without approval from" the teacher, no reason');
  t_ok(strlen($code) > 15 && strpos($code, (string) $a3) === 0 && strpos($l['body'], $LINK . $code) !== FALSE && strpos($l['sms'], $LINK . $code) !== FALSE, 'L7 ... a one-time code is saved and the link carries it (email and SMS)');

  // --- returned automatically: the chosen Recommending AT has no email in DIPI
  $a4 = $new('R-ATReview', 'Zzq Noemail', 'Pending', '', 'Pending');
  $cap = $run('review_mail', $a4);
  t_eq(array($status($a4), $sent($cap)), array('R-ATTransfer', array('applicant:' . $lid('Recommending AT Returned'))), 'L8 automatic return (teacher has no email): R-ATTransfer and the same letter to the student');
  t_ok(strpos($applicant_letter($cap)['body'], 'without approval from your Recommending Assistant Teacher Zzq Noemail') !== FALSE, 'L9 ... true for this case too ("without approval from")');

  // --- back from R-ATTransfer to review: with another teacher -> "Changed"; with the same one -> no letter
  db_update('dh_applicant_lc')->fields(array('al_recommending' => 'Zzq Othereco', 'al_recommending_approved' => 'Pending'))->condition('al_applicant', $a3)->execute();
  $cap = $run('status', $a3, array('status' => 'R-ATReview'));
  t_eq(array($status($a3), $sent($cap)), array('R-ATReview', array('r-teacher:' . $lid('Recommending Review'), 'applicant:' . $lid('Recommending AT Changed'))),
    'L10 sent back to review with another Recommending AT (registrar, or the student via the link): review request + "Recommending AT Changed"');
  t_ok(strpos($applicant_letter($cap)['body'], 'from the Recommending Assistant Teacher Zzq Recoteacher to the Recommending Assistant Teacher Zzq Othereco') !== FALSE, 'L11 ... former = the teacher it was returned from');
  $a5 = $new('R-ATReview', 'Zzq Recoteacher', 'Pending', '', 'Pending');
  db_update('dh_applicant_lc')->fields(array('al_recommending_approved' => 'Transfer to Registrar'))->condition('al_applicant', $a5)->execute();
  $run('status', $a5, array('status' => 'R-ATTransfer'));
  db_update('dh_applicant_lc')->fields(array('al_recommending_approved' => 'Pending'))->condition('al_applicant', $a5)->execute();
  $cap = $run('status', $a5, array('status' => 'R-ATReview'));
  t_eq($sent($cap), array('r-teacher:' . $lid('Recommending Review')), 'L12 sent back to the SAME teacher: review request only, no "Changed" letter');

  // --- DIPI Edit changed a teacher during the review
  $a6 = $new('R-ATReview', 'Zzq Othereco', 'Pending', '', 'Pending');
  t_eq($sent($run('edit_letters', $a6, array('prev_reco' => 'Zzq Recoteacher', 'prev_area' => ''))), array('applicant:' . $lid('Recommending AT Changed')), 'L13 Edit changed the Recommending AT in R-ATReview: "Changed" letter');
  t_eq($sent($run('edit_letters', $a6, array('prev_reco' => '', 'prev_area' => ''))), array(), 'L14 Edit set a first teacher (no former one): no letter');
  $a7 = $new('A-ATReview', 'Zzq Recoteacher', 'Approved', 'Zzq Otherarea', 'Pending');
  t_eq($sent($run('edit_letters', $a7, array('prev_reco' => '', 'prev_area' => 'Zzq Areateacher'))), array('applicant:' . $lid('Area AT Changed')), 'L15 Edit changed the Area AT in A-ATReview: "Area AT Changed"');
  t_eq($sent($run('edit_letters', $a4, array('prev_reco' => 'Zzq Recoteacher', 'prev_area' => ''))), array(), 'L16 not while the application is back at the centre (R-ATTransfer)');

  // --- the "Returned" letter cannot lose its link (letter editor save rule)
  $ret_id = $lid('Recommending AT Returned');
  $r1 = dh_letter_protect($cen, 'edit', $ret_id, array('l_body' => '<p>No link here</p>'));
  $r2 = dh_letter_protect($cen, 'edit', $ret_id, array('l_body' => '<p>Choose: [ChooseTeacherLink]</p>'));
  t_ok($r1[1] !== '' && strpos($r1[1], '[ChooseTeacherLink]') !== FALSE && $r2[1] === '', 'L19 the "Returned" letter cannot be saved without [ChooseTeacherLink]; with it, it can');

  // --- a centre without the new letters
  db_update('dh_letter')->fields(array('l_deleted' => 1))->condition('l_center', $cen)->condition('l_name', array('Recommending AT Changed', 'Recommending AT Returned'), 'IN')->execute();
  $a8 = $new('R-ATReview', 'Zzq Othereco', 'Pending', '', 'Pending');
  $run('edit_letters', $a8, array('prev_reco' => 'Zzq Recoteacher', 'prev_area' => ''));
  t_eq((int) db_query("select count(*) from dh_log where l_id > :l and l_identifier = :a and l_msg = 'No Recommending AT Changed letter found!'", array(':l' => $start['dh_log'], ':a' => (string) $a8))->fetchField(), 1, 'L17 no such letter at the centre: logged, nothing sent');
  db_update('dh_applicant_lc')->fields(array('al_recommending_approved' => 'Transfer to Registrar'))->condition('al_applicant', $a8)->execute();
  $cap = $run('status', $a8, array('status' => 'R-ATTransfer'));
  t_ok(is_array($cap) && count($cap) == 0 && (int) db_query("select count(*) from dh_log where l_id > :l and l_identifier = :a and l_msg = 'No letter Found!'", array(':l' => $start['dh_log'], ':a' => (string) $a8))->fetchField() == 1,
    'L18 no "Returned" letter: the centre\'s own R-ATTransfer status letter is tried instead, as before (none here)');
}
catch (Exception $e) { t_ok(FALSE, 'setup: ' . $e->getMessage()); }
finally {
  if ($made['app']) {
    foreach (array('dh_applicant_attended' => 'aa_applicant', 'dh_applicant_lc' => 'al_applicant', 'dh_applicant_course' => 'ac_applicant', 'dh_application_tag' => 'at_application',
        'dh_application_tag_log' => 'atl_application', 'dh_applicant' => 'a_id') as $t => $col)
      db_delete($t)->condition($col, $made['app'], 'IN')->execute();
    db_query("delete from dh_log where l_id > :l and l_identifier in (:a)", array(':l' => $start['dh_log'], ':a' => array_map('strval', $made['app'])));
  }
  $newtags = db_query("select t_id from dh_tag where t_id > :t", array(':t' => $start['dh_tag']))->fetchCol();
  if ($newtags) { db_delete('dh_tag_log')->condition('tl_tag', $newtags, 'IN')->execute(); db_delete('dh_tag')->condition('t_id', $newtags, 'IN')->execute(); }
  foreach ($made['course'] as $c) { db_delete('dh_course')->condition('c_id', $c)->execute(); db_delete('dh_course_log')->condition('crl_course', $c)->execute(); }
  if ($made['letter']) db_delete('dh_letter')->condition('l_id', $made['letter'], 'IN')->execute();
  if ($made['field']) db_delete('dh_letter_fields')->condition('lf_id', $made['field'], 'IN')->execute();
  db_query("delete from dh_letter_log where ll_id > :l and ll_center = :c", array(':l' => $start['dh_letter_log'], ':c' => (int) $made['centre']));
  if ($made['centre']) db_delete('dh_center')->condition('c_id', $made['centre'])->execute();
  if ($made['teacher']) { db_delete('dh_teacher')->condition('t_id', $made['teacher'], 'IN')->execute(); db_delete('dh_teacher_log')->condition('tl_teacher', $made['teacher'], 'IN')->execute(); }
  @unlink($runner);
  echo '  cleanup: ' . count($made['app']) . ' applicants, ' . count($made['letter']) . ' letter copies, ' . count($made['teacher']) . " teachers\n";
}
t_ok((int) db_query("select count(*) from dh_applicant where a_email = 'zzqtest-tl@example.invalid'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_center where c_name like 'Zzqtest Centre%'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_teacher where t_code in ('ZZQ5','ZZQ6','ZZQ7','ZZQ8','ZZQ9')")->fetchField() === 0, 'C1 nothing left behind');
t_done();
