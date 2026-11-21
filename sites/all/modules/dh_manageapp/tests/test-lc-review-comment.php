<?php
// Long-course review comments "on behalf of": when someone other than that step's teacher decides a review step - the
// centre team on the search page (Change Status), an "LC Admin for AT" or the other teacher on the AT Portal - the saved
// Recommending AT / Area AT comment starts with: Approved by user "abc" on behalf of "AT Name" with comment: ...
// The teacher's own decision is saved as typed. Search page: comment optional for Approved, reason required for Rejected.
// Status changes run in their own PHP process (the pages answer with JSON / a redirect and exit), on made-up courses,
// applicants and teachers that are deleted at the end. Mail is switched off in that process (no Mailgun key).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-review-comment.php
require __DIR__ . '/lib.php';

$LC = 13;      // 45-DayOSC (td_val3 = 'LC')
$runner = tempnam(sys_get_temp_dir(), 'zzq-lc-comment-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$GLOBALS["conf"]["mailgun_key"] = "zzq-no-mail"; $GLOBALS["conf"]["mode_test_emails"] = "";
$in = json_decode($argv[1], TRUE); $GLOBALS["user"] = user_load(1);
if (!empty($in["anon"])) $GLOBALS["user"] = drupal_anonymous_user();
if (!empty($in["name"])) $GLOBALS["user"]->name = $in["name"];
if ($in["page"] == "search") { $_GET["q"] = "change-status/" . (int) $in["app"]; $_REQUEST = $_GET = array_merge(array("q" => $_GET["q"]), $in["req"]); _change_status(); }
else { $_GET["q"] = "lc-review/app/" . (int) $in["app"]; $form = array(); $fs = array("input" => $in["req"]); dh_atportal_review_form_submit($form, $fs); }
');
$made = array('centre' => 0, 'course' => array(), 'app' => array(), 'teacher' => array(), 'lcadmin' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_log' => $max('dh_log', 'l_id'));
$me = user_load(1)->name;
try {
  $made['teacher'][] = db_insert('dh_teacher')->fields(array('t_code' => 'ZZQ9', 't_gender' => 'M', 't_f_name' => 'Zzq', 't_l_name' => 'Recoteacher', 't_status' => 'Active', 't_created_by' => 0, 't_updated_by' => 0))->execute();
  $made['teacher'][] = db_insert('dh_teacher')->fields(array('t_code' => 'ZZQ8', 't_gender' => 'F', 't_f_name' => 'Zzq', 't_l_name' => 'Areateacher', 't_status' => 'Active', 't_created_by' => 0, 't_updated_by' => 0))->execute();
  // The AT Portal test user (uid 1, named "zzqlcadmin" below) is an "LC Admin for AT" for both made-up teachers.
  foreach ($made['teacher'] as $tid)
    $made['lcadmin'][] = db_insert('dh_lc_admin')->fields(array('la_user' => 1, 'la_teacher' => $tid, 'la_deleted' => 0, 'la_created_by' => 1, 'la_updated_by' => 1))->execute();

  // --- the helper
  $sh = (object) array('uid' => 0, 'name' => 'sushama.hp');
  t_eq(dh_lc_review_comment('Approved', 'Madhu Hooda', 'Looks fine', $sh), 'Approved by user "sushama.hp" on behalf of "Madhu Hooda" with comment: Looks fine', 'H1 approved for a teacher, with a comment');
  t_eq(dh_lc_review_comment('Approved', 'Madhu Hooda', '  ', $sh), 'Approved by user "sushama.hp" on behalf of "Madhu Hooda".', 'H2 no comment: still says who decided for whom');
  t_eq(dh_lc_review_comment('Rejected', 'Madhu Hooda', 'Duplicate', $sh), 'Rejected by user "sushama.hp" on behalf of "Madhu Hooda" with comment: Duplicate', 'H3 rejected');
  t_eq(dh_lc_review_comment('Transfer to Registrar', 'Madhu Hooda', 'Not my area', $sh), 'Transferred to the registrar by user "sushama.hp" on behalf of "Madhu Hooda" with comment: Not my area', 'H4 transfer to registrar');
  t_eq(dh_lc_review_comment('Approved', '', 'Ok', $sh), 'Approved by user "sushama.hp" on behalf of the teacher with comment: Ok', 'H5 no teacher chosen yet');
  t_eq(dh_lc_review_comment('Approved', 'Zzq Recoteacher', 'My own review', (object) array('uid' => 0, 'name' => 'ZZQ9.M')), 'My own review', 'H6 the teacher themselves: saved as typed');
  t_eq(dh_lc_review_comment('Approved', ' zzq  RECOTEACHER ', 'Own', (object) array('uid' => 0, 'name' => 'zzq9.m')), 'Own', 'H7 ... matched without minding case or extra spaces');
  t_eq(dh_lc_review_comment('Approved', 'Zzq Recoteacher', 'For the reco', (object) array('uid' => 0, 'name' => 'zzq8.f')),
    'Approved by user "zzq8.f" on behalf of "Zzq Recoteacher" with comment: For the reco', 'H8 the other teacher (Area AT deciding the Recommending step): prefixed');
  t_eq(dh_lc_review_comment('', 'Madhu Hooda', 'Just a note', $sh), 'Just a note', 'H9 no decision: saved as typed');

  // --- made-up centre (no letters, no emails: nothing is looked up or sent) + course + applications
  $c = db_query("select * from dh_center where c_id > 0 order by c_id desc limit 1")->fetchAssoc();
  unset($c['c_id']);
  $made['centre'] = $cen = (int) db_insert('dh_center')->fields(array_merge($c, array('c_name' => 'Zzqtest Centre ' . mt_rand(100000, 999999), 'c_email' => '', 'c_lc_admin_email' => '')))->execute();
  $lc = $made['course'][] = t_make_course(array('c_center' => $cen, 'c_course_type' => $LC, 'c_start' => date('Y-m-d', strtotime('+20 days')), 'c_end' => date('Y-m-d', strtotime('+65 days')), 'c_processed' => 0, 'c_finalize_mail' => 0));
  $new = function ($status, $reco, $area, $area_at = 'Zzq Areateacher') use ($lc, &$made) {
    $id = t_make_applicant($lc, array('a_attended' => 0, 'a_status' => $status, 'a_email' => 'zzqtest-lcc@example.invalid', 'a_updated_by' => 0));
    db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute();
    db_insert('dh_applicant_lc')->fields(array('al_applicant' => $id, 'al_recommending' => 'Zzq Recoteacher', 'al_recommending_approved' => $reco,
      'al_area_at' => $area_at, 'al_area_at_approved' => $area))->execute();
    $made['app'][] = $id;
    return $id;
  };
  $run = function ($page, $id, $req, $name = '', $anon = FALSE) use ($runner) {
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array('page' => $page, 'app' => $id, 'req' => $req, 'name' => $name, 'anon' => $anon))) . ' 2>/dev/null');
    $p = strpos($out, '{'); $j = $p === FALSE ? NULL : json_decode(substr($out, $p), TRUE);
    return is_array($j) ? ($j['status'] == 'OK' ? 'OK' : 'Failed: ' . $j['msg']) : 'no json: ' . substr($out, 0, 300);
  };
  $lcrow = function ($id) {
    return db_query("select a.a_status, al.al_recommending_approved r, al.al_recommending_comments rc, al.al_area_at_approved ar, al.al_area_at_comments ac
      from dh_applicant a join dh_applicant_lc al on al.al_applicant = a.a_id where a.a_id = :a", array(':a' => $id))->fetchAssoc();
  };

  // --- search page, Change Status (the centre team)
  $a1 = $new('R-ATReview', 'Pending', 'Pending');
  t_eq($run('search', $a1, array('s' => 'Approved', 'c' => 'Looks fine', 'l' => '')), 'OK', 'S1 search: Approved on R-ATReview');
  t_eq($lcrow($a1), array('a_status' => 'A-ATReview', 'r' => 'Approved', 'rc' => 'Approved by user "' . $me . '" on behalf of "Zzq Recoteacher" with comment: Looks fine', 'ar' => 'Pending', 'ac' => NULL),
    'S2 ... Recommending step approved, its comment saved with who and for whom');
  t_ok((int) db_query("select count(*) from dh_log where l_id > :l and l_identifier = :a and l_event = 'LC Workflow' and l_user = 1 and l_msg like 'RecoAT Commented: Approved by user%'",
    array(':l' => $start['dh_log'], ':a' => (string) $a1))->fetchField() === 1, 'S3 ... the Activity Log shows the comment under the user who did it');
  t_eq($run('search', $a1, array('s' => 'Approved', 'c' => '', 'l' => '')), 'OK', 'S4 search: Approved again (now A-ATReview), no comment');
  $r = $lcrow($a1);
  t_eq(array($r['a_status'], $r['ar'], $r['ac']), array('Received', 'Approved', 'Approved by user "' . $me . '" on behalf of "Zzq Areateacher".'), 'S5 ... Area step approved, its comment says who and for whom');
  t_eq($r['rc'], 'Approved by user "' . $me . '" on behalf of "Zzq Recoteacher" with comment: Looks fine', 'S6 ... the Recommending comment is kept');
  $a2 = $new('R-ATReview', 'Pending', 'Pending');
  t_eq($run('search', $a2, array('s' => 'Rejected-R-AT', 'c' => ' ', 'l' => '')), 'Failed: Please give the rejection reason.', 'S7 search: Rejected without a reason is refused');
  t_eq($lcrow($a2), array('a_status' => 'R-ATReview', 'r' => 'Pending', 'rc' => NULL, 'ar' => 'Pending', 'ac' => NULL), 'S8 ... and nothing changed');
  t_eq($run('search', $a2, array('s' => 'Rejected-R-AT', 'c' => 'Applied twice', 'l' => '')), 'OK', 'S9 search: Rejected with a reason');
  t_eq($lcrow($a2), array('a_status' => 'Rejected', 'r' => 'Rejected', 'rc' => 'Rejected by user "' . $me . '" on behalf of "Zzq Recoteacher" with comment: Applied twice', 'ar' => 'Pending', 'ac' => NULL),
    'S10 ... Recommending step rejected with the reason');
  $a3 = $new('A-ATReview', 'Approved', 'Pending');
  t_eq($run('search', $a3, array('s' => 'Rejected-A-AT', 'c' => 'Not now', 'l' => '')), 'OK', 'S11 search: Area step rejected with a reason');
  t_eq($lcrow($a3)['ac'], 'Rejected by user "' . $me . '" on behalf of "Zzq Areateacher" with comment: Not now', 'S12 ... Area comment saved');
  $a4 = $new('R-ATReview', 'Pending', 'Pending', '');
  t_eq($run('search', $a4, array('s' => 'Approved', 'c' => 'x', 'l' => '')), 'Failed: Please Edit application and choose Area teacher before approving!', 'S13 search: no Area teacher yet: refused as before');
  t_eq($lcrow($a4)['rc'], NULL, 'S14 ... and no comment saved');
  $a5 = $new('R-ATReview', 'Pending', 'Pending');
  t_eq($run('search', $a5, array('s' => 'Approved', 'c' => 'Mine', 'l' => ''), 'zzq9.m'), 'OK', 'S15 search: the Recommending teacher\'s own login approves');
  t_eq($lcrow($a5)['rc'], 'Mine', 'S16 ... comment saved as typed (no prefix)');

  // --- AT Portal review form (an "LC Admin for AT", or a teacher)
  $p1 = $new('R-ATReview', 'Pending', 'Pending', '');
  $run('portal', $p1, array('action' => 'Approved', 'comments' => 'Good practice', 'area_teacher' => 'Zzq Areateacher'), 'zzqlcadmin');
  $r = $lcrow($p1);
  t_eq(array($r['a_status'], $r['r'], $r['rc']), array('A-ATReview', 'Approved', 'Approved by user "zzqlcadmin" on behalf of "Zzq Recoteacher" with comment: Good practice'),
    'P1 portal: LC Admin for AT approves the Recommending step: prefixed');
  $run('portal', $p1, array('action' => 'Approved', 'comments' => ''), 'zzqlcadmin');
  $r = $lcrow($p1);
  t_eq(array($r['a_status'], $r['ar'], $r['ac']), array('Received', 'Approved', 'Approved by user "zzqlcadmin" on behalf of "Zzq Areateacher".'), 'P2 portal: ... and the Area step: prefixed');
  $p2 = $new('R-ATReview', 'Pending', 'Pending', '');
  $run('portal', $p2, array('action' => 'Approved', 'comments' => 'Known to me', 'area_teacher' => 'Zzq Areateacher'), 'zzq9.m');
  t_eq($lcrow($p2)['rc'], 'Known to me', 'P3 portal: the Recommending teacher\'s own decision: as typed');
  $run('portal', $p2, array('action' => 'Approved', 'comments' => 'Fine'), 'zzq8.f');
  t_eq($lcrow($p2)['ac'], 'Fine', 'P4 portal: the Area teacher\'s own decision: as typed');
  $p3 = $new('R-ATReview', 'Pending', 'Pending', '');
  $run('portal', $p3, array('action' => 'Transfer to Registrar', 'comments' => 'Not my student', 'area_teacher' => ''), 'zzqlcadmin');
  $r = $lcrow($p3);
  t_eq(array($r['a_status'], $r['rc']), array('R-ATTransfer', 'Transferred to the registrar by user "zzqlcadmin" on behalf of "Zzq Recoteacher" with comment: Not my student'), 'P5 portal: transfer to registrar on behalf: prefixed');
  $p4 = $new('R-ATReview', 'Pending', 'Pending');
  $run('portal', $p4, array('action' => 'Rejected', 'comments' => 'Too soon', 'area_teacher' => ''), 'zzq8.f');
  t_eq($lcrow($p4)['rc'], 'Rejected by user "zzq8.f" on behalf of "Zzq Recoteacher" with comment: Too soon', 'P6 portal: the Area teacher deciding the Recommending step: prefixed');
  $p5 = $new('R-ATReview', 'Pending', 'Pending', '');
  $run('portal', $p5, array('action' => 'ApproveOwnApplication', 'comments' => 'Own application', 'area_teacher' => ''), 'zzq9.m');
  $r = $lcrow($p5);
  t_eq(array($r['r'], $r['rc'], $r['ar'], $r['ac']), array('Approved', 'Own application', 'Approved', 'Self approved by AT.'), 'P7 portal: a teacher approving their own application: unchanged');

  // --- the retired centre form: a user who is not a teacher of the application (nor an LC Admin for one) saves nothing
  $q1 = $new('R-ATReview', 'Pending', 'Pending');
  $before = $lcrow($q1);
  $run('portal', $q1, array('owner_center_app' => '1', 'reco_teacher' => 'Zzq Recoteacher', 'reco_action' => 'Approved', 'reco_comments' => 'centre form',
    'area_teacher' => 'Zzq Areateacher', 'area_action' => 'Approved', 'area_comments' => 'centre form'), 'zzqcentre', TRUE);
  t_eq($lcrow($q1), $before, 'R1 portal: the old centre form\'s fields from a non-teacher: nothing saved');
  $run('portal', $q1, array('action' => 'Approved', 'comments' => 'x', 'area_teacher' => 'Zzq Areateacher'), 'zzqcentre', TRUE);
  t_eq($lcrow($q1), $before, 'R2 portal: a review from a non-teacher: nothing saved');
  $q2 = $new('R-ATReview', 'Pending', 'Pending');
  $run('portal', $q2, array('owner_center_app' => '1', 'reco_action' => 'Approved', 'reco_comments' => 'old field', 'action' => 'Approved', 'comments' => 'Ok', 'area_teacher' => 'Zzq Areateacher'), 'zzq9.m');
  $r = $lcrow($q2);
  t_eq(array($r['r'], $r['rc']), array('Approved', 'Ok'), 'R3 portal: a teacher\'s post that also carries the old centre fields: only the teacher\'s own review is used');
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
    db_delete('dh_teacher_log')->condition('tl_teacher', $made['teacher'], 'IN')->execute();   // rows written by the dh_teacher triggers
  }
  @unlink($runner);
  echo '  cleanup: ' . count($made['app']) . ' made-up applicants, ' . count($made['course']) . ' course, ' . count($made['teacher']) . " teachers\n";
}
t_ok((int) db_query("select count(*) from dh_applicant where a_email = 'zzqtest-lcc@example.invalid'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_teacher where t_code in ('ZZQ8', 'ZZQ9')")->fetchField() === 0
  && (int) db_query("select count(*) from dh_center where c_name like 'Zzqtest Centre%'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_teacher_log where tl_teacher in (:t)", array(':t' => $made['teacher'] ? $made['teacher'] : array(0)))->fetchField() === 0, 'C1 nothing left behind');
t_done();
