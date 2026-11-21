<?php
// AT Portal "Transfer to another Recommending AT" (R-ATReview): the Recommending AT, or an LC Admin for them, passes the
// review to another Active teacher; the step stays Pending, the application stays in R-ATReview, the log says who passed
// it to whom, the new teacher is sent the review request and can then decide; the former one no longer can. A transfer
// to the present teacher (or with none chosen) is refused - also for the Area AT "Transfer".
// Each save runs in its own PHP process like the page (validate, then submit), on a made-up centre with no letters and
// made-up teachers, all deleted at the end. Mail is switched off in that process.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-reco-transfer.php
require __DIR__ . '/lib.php';

$LC = 13;      // 45-DayOSC
$runner = tempnam(sys_get_temp_dir(), 'zzq-reco-transfer-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$GLOBALS["conf"]["mailgun_key"] = "zzq-no-mail"; $GLOBALS["conf"]["mode_test_emails"] = "";
$in = json_decode($argv[1], TRUE); $GLOBALS["user"] = user_load(1); $GLOBALS["user"]->name = $in["name"];
$_GET["q"] = "lc-review/app/" . (int) $in["app"]; $form = array(); $fs = array("input" => $in["req"]);
dh_atportal_review_form_validate($form, $fs);
if ($e = form_get_errors()) { echo "ERRORS:" . implode(" | ", $e) . "\n"; exit; }
dh_atportal_review_form_submit($form, $fs);
');
$made = array('centre' => 0, 'course' => array(), 'app' => array(), 'teacher' => array(), 'lcadmin' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_log' => $max('dh_log', 'l_id'));
try {
  $teacher = function ($code, $g, $last) use (&$made) {
    return $made['teacher'][] = db_insert('dh_teacher')->fields(array('t_code' => $code, 't_gender' => $g, 't_f_name' => 'Zzq', 't_l_name' => $last, 't_status' => 'Active',
      't_email' => 'zzqtest-' . strtolower($last) . '@example.invalid', 't_created_by' => 0, 't_updated_by' => 0))->execute();
  };
  $reco = $teacher('ZZQ9', 'M', 'Recoteacher');      // login zzq9.m
  $teacher('ZZQ7', 'M', 'Othereco');                  // login zzq7.m
  $teacher('ZZQ8', 'F', 'Areateacher');               // login zzq8.f
  // uid 1 (named "zzqlcadmin" below) is an LC Admin for the first Recommending AT only
  $made['lcadmin'][] = db_insert('dh_lc_admin')->fields(array('la_user' => 1, 'la_teacher' => $reco, 'la_deleted' => 0, 'la_created_by' => 1, 'la_updated_by' => 1))->execute();
  $c = db_query("select * from dh_center where c_id > 0 order by c_id desc limit 1")->fetchAssoc();
  unset($c['c_id']);
  $made['centre'] = $cen = (int) db_insert('dh_center')->fields(array_merge($c, array('c_name' => 'Zzqtest Centre ' . mt_rand(100000, 999999), 'c_email' => '', 'c_lc_admin_email' => '')))->execute();
  $lc = $made['course'][] = t_make_course(array('c_center' => $cen, 'c_course_type' => $LC, 'c_start' => date('Y-m-d', strtotime('+20 days')), 'c_end' => date('Y-m-d', strtotime('+65 days')), 'c_processed' => 0, 'c_finalize_mail' => 0));
  $new = function ($status, $reco_at, $reco, $area_at, $area) use ($lc, &$made) {
    $id = t_make_applicant($lc, array('a_attended' => 0, 'a_status' => $status, 'a_email' => 'zzqtest-rt@example.invalid', 'a_updated_by' => 0));
    db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute();
    db_insert('dh_applicant_lc')->fields(array('al_applicant' => $id, 'al_recommending' => $reco_at, 'al_recommending_approved' => $reco, 'al_area_at' => $area_at, 'al_area_at_approved' => $area))->execute();
    $made['app'][] = $id;
    return $id;
  };
  $run = function ($id, $req, $name) use ($runner) {
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array('app' => $id, 'req' => $req, 'name' => $name))) . ' 2>/dev/null');
    return preg_match('/ERRORS:(.*)/', $out, $m) ? trim($m[1]) : 'saved';
  };
  $row = function ($id) {
    return db_query("select a.a_status, al.al_recommending, al.al_recommending_approved r, al.al_area_at, al.al_area_at_approved ar, al.al_recommending_comments rc
      from dh_applicant a join dh_applicant_lc al on al.al_applicant = a.a_id where a.a_id = :a", array(':a' => $id))->fetchAssoc();
  };
  $logs = function ($id, $like) use ($start) {
    return db_query("select l_user, l_msg from dh_log where l_id > :l and l_identifier = :a and l_msg like :m order by l_id", array(':l' => $start['dh_log'], ':a' => (string) $id, ':m' => $like))->fetchAll(PDO::FETCH_ASSOC);
  };

  // --- the Recommending AT passes the review on
  $a1 = $new('R-ATReview', 'Zzq Recoteacher', 'Pending', '', 'Pending');
  t_eq($run($a1, array('action' => 'TransferReco', 'transfer_reco_teacher' => 'Zzq Recoteacher', 'comments' => 'x', 'area_teacher' => ''), 'zzq9.m'),
    'Zzq Recoteacher is already the Recommending AT. Please select another teacher.', 'T1 to the present Recommending AT: refused');
  t_eq($run($a1, array('action' => 'TransferReco', 'transfer_reco_teacher' => '', 'comments' => 'x', 'area_teacher' => ''), 'zzq9.m'), 'Please select the new Recommending AT.', 'T2 no teacher chosen: refused');
  t_eq($row($a1)['al_recommending'], 'Zzq Recoteacher', 'T3 ... nothing changed');
  t_eq($run($a1, array('action' => 'TransferReco', 'transfer_reco_teacher' => 'Zzq Othereco', 'comments' => 'Lives near your centre', 'area_teacher' => ''), 'zzq9.m'), 'saved', 'T4 to another Active teacher: saved');
  t_eq($row($a1), array('a_status' => 'R-ATReview', 'al_recommending' => 'Zzq Othereco', 'r' => 'Pending', 'al_area_at' => '', 'ar' => 'Pending', 'rc' => NULL),
    'T5 ... new Recommending AT; step still Pending; still R-ATReview; no comment saved as a decision');
  t_eq(array_column($logs($a1, 'RecoAT Transfer from %'), 'l_msg'), array('RecoAT Transfer from Zzq Recoteacher to Zzq Othereco (Comment: Lives near your centre)'), 'T6 ... Activity Log: who to whom, with the comment');
  t_eq(count($logs($a1, 'RecoAT Assigned to Zzq Othereco')), 1, 'T7 ... and the usual "RecoAT Assigned to" line');
  t_ok(count($logs($a1, 'No letter Found!')) >= 1, 'T8 ... the review request was sent for the new teacher (this made-up centre has no letter, so it logs "No letter Found!")');
  // the former teacher can no longer decide; the new one can
  t_eq($run($a1, array('action' => 'Approved', 'comments' => 'late', 'area_teacher' => 'Zzq Areateacher'), 'zzq9.m'), 'saved', 'T9 the former Recommending AT posts an approval ...');
  t_eq($row($a1)['r'], 'Pending', 'T10 ... nothing saved (not a teacher of this application any more)');
  t_eq($run($a1, array('action' => 'Approved', 'comments' => 'Fine', 'area_teacher' => 'Zzq Areateacher'), 'zzq7.m'), 'saved', 'T11 the new Recommending AT approves');
  $r = $row($a1);
  t_eq(array($r['a_status'], $r['r'], $r['rc'], $r['al_area_at']), array('A-ATReview', 'Approved', 'Fine', 'Zzq Areateacher'), 'T12 ... approved in their own name');

  // --- an LC Admin for the Recommending AT passes it on
  $a2 = $new('R-ATReview', 'Zzq Recoteacher', 'Pending', '', 'Pending');
  t_eq($run($a2, array('action' => 'TransferReco', 'transfer_reco_teacher' => 'Zzq Othereco', 'comments' => '', 'area_teacher' => ''), 'zzqlcadmin'), 'saved', 'T13 LC Admin for AT: saved');
  $l = $logs($a2, 'RecoAT Transfer from %');
  t_eq(array(count($l), $l ? $l[0]['l_msg'] : '', $l ? (int) $l[0]['l_user'] : 0), array(1, 'RecoAT Transfer from Zzq Recoteacher to Zzq Othereco', 1), 'T14 ... logged under the LC Admin, no "(Comment:)" when empty');

  // --- the Area AT "Transfer": the same check
  $a3 = $new('A-ATReview', 'Zzq Recoteacher', 'Approved', 'Zzq Areateacher', 'Pending');
  t_eq($run($a3, array('action' => 'Transfer', 'transfer_area_teacher' => 'Zzq Areateacher', 'comments' => 'x'), 'zzq8.f'),
    'Zzq Areateacher is already the Area teacher. Please select another teacher.', 'T15 Area AT transfer to the present Area AT: refused');
  t_eq($row($a3)['al_area_at'], 'Zzq Areateacher', 'T16 ... nothing changed');
}
finally {
  if ($made['app']) {
    foreach (array('dh_applicant_attended' => 'aa_applicant', 'dh_applicant_lc' => 'al_applicant', 'dh_applicant_course' => 'ac_applicant', 'dh_application_tag' => 'at_application',
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
  if ($made['centre']) db_delete('dh_center')->condition('c_id', $made['centre'])->execute();
  if ($made['lcadmin']) db_delete('dh_lc_admin')->condition('la_id', $made['lcadmin'], 'IN')->execute();
  if ($made['teacher']) {
    db_delete('dh_teacher')->condition('t_id', $made['teacher'], 'IN')->execute();
    db_delete('dh_teacher_log')->condition('tl_teacher', $made['teacher'], 'IN')->execute();
  }
  @unlink($runner);
  echo '  cleanup: ' . count($made['app']) . ' made-up applicants, ' . count($made['teacher']) . " teachers\n";
}
t_ok((int) db_query("select count(*) from dh_applicant where a_email = 'zzqtest-rt@example.invalid'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_teacher where t_code in ('ZZQ7', 'ZZQ8', 'ZZQ9')")->fetchField() === 0
  && (int) db_query("select count(*) from dh_center where c_name like 'Zzqtest Centre%'")->fetchField() === 0, 'C1 nothing left behind');
t_done();
