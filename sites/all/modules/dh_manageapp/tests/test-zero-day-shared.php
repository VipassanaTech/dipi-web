<?php
// Zero Day save with a shared (men + women) section: a place a woman holds is taken for a man; going into a shared section
// that has the other side needs "place anyway" (mx=1); moving inside it does not ask again; same-name men's and women's
// sections (two buildings) still do not clash. Each save runs in its own PHP process (like one request), on made-up
// centre/courses/applicants that are deleted at the end.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-zero-day-shared.php
require __DIR__ . '/lib.php';

$runner = tempnam(sys_get_temp_dir(), 'zzq-zd-shared-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$in = json_decode($argv[1], TRUE); $GLOBALS["user"] = user_load(1); $_GET["q"] = "app-update-attended/" . (int) $in["app"];
$_REQUEST = $_POST = $in["req"]; dh_app_update_attended();
');
$made = array('centre' => 0, 'course' => array(), 'app' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_log' => $max('dh_log', 'l_id'));
try {
  $made['centre'] = $cen = t_make_centre();
  t_make_room($cen, 'B', 'S', '5', array('5A', '5B', '5C'));
  t_make_room($cen, 'B', 'S', '7', array('7A'));
  t_make_room($cen, 'M', 'T', '1', array('1'));
  t_make_room($cen, 'F', 'T', '1', array('1'));
  $A = $made['course'][] = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d'), 'c_end' => date('Y-m-d', strtotime('+10 days')), 'c_processed' => 0, 'c_finalize_mail' => 0));
  $made['app'][] = t_make_applicant($A, array('a_gender' => 'F'), array('aa_section' => 'S', 'aa_acco' => ' 5A'));
  $made['app'][] = t_make_applicant($A, array('a_gender' => 'F'), array('aa_section' => 'T', 'aa_acco' => '1'));
  $new = function ($g) use ($A, &$made) { $id = t_make_applicant($A, array('a_attended' => 0, 'a_gender' => $g, 'a_email' => 'zzqtest-zds@example.invalid')); db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute(); $made['app'][] = $id; return $id; };
  $save = function ($id, $s, $r, $mx = '') use ($runner) {
    $req = array('a' => 'true', 's' => $s, 'r' => $r, 'g' => '1', 'l' => '', 'v' => '', 'c' => '', 'cf' => '0', 'gc' => '', 'gcf' => '0', 'dn' => '', 'df' => '0', 'gdn' => '', 'gdf' => '0', 'chow' => '0', 'chai' => '0', 'back' => '0', 'comment' => '', 'ov' => '', 'mx' => $mx);
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array('app' => $id, 'req' => $req))) . ' 2>/dev/null');
    $j = json_decode(substr($out, strpos($out, '{')), TRUE);
    return is_array($j) ? (!empty($j['status']) ? 'ok' : (string) $j['msg'] . (!empty($j['mixed']) ? ' [mixed]' : '')) : 'no reply';
  };
  $man = $new('M');
  t_eq($save($man, 'S', '5A'), 'Room has already been alloted', 'ZS1 a shared place a woman holds (saved as " 5A") is taken for a man');
  t_eq($save($man, 'S', '7A'), 'Section S has women in this course. Place a man here anyway? [mixed]', 'ZS2 a man into a shared section with women: asked first');
  t_eq($save($man, 'S', '7A', '1'), 'ok', 'ZS3 with mx=1 it is saved');
  t_eq($save($man, 'S', '5B'), 'Room 5 has women in this course. Place a man here anyway? [mixed]', 'ZS4 inside the section, into the room where a woman is: asked again');
  t_eq($save($man, 'S', '5B', '1'), 'ok', 'ZS4b with mx=1 it is saved');
  t_eq($save($man, 'S', '5C'), 'ok', 'ZS4c moving inside the same room does not ask again');
  t_eq($save($new('M'), 'T', '1'), 'ok', 'ZS5 men\'s T-1 and women\'s T-1 are two buildings: no clash');
  $al = dh_get_alloted($cen, $A);
  t_ok(in_array('5C', $al['F']['S'], TRUE) && in_array('5A', $al['M']['S'], TRUE) && $al['M']['T'] === array('1') && $al['F']['T'] === array('1'),
    'ZS6 Zero Day taken list: shared places for both sides; men\'s and women\'s T each only their own');
}
finally {
  if ($made['app']) {
    foreach (array('dh_applicant_attended' => 'aa_applicant', 'dh_application_tag' => 'at_application', 'dh_application_tag_log' => 'atl_application', 'dh_applicant' => 'a_id') as $t => $col)
      db_delete($t)->condition($col, $made['app'], 'IN')->execute();
    db_query("delete from dh_log where l_id > :l and l_identifier in (:a)", array(':l' => $start['dh_log'], ':a' => array_map('strval', $made['app'])));
  }
  $newtags = db_query("select t_id from dh_tag where t_id > :t", array(':t' => $start['dh_tag']))->fetchCol();
  if ($newtags) {
    db_delete('dh_tag_log')->condition('tl_tag', $newtags, 'IN')->execute();
    db_delete('dh_tag')->condition('t_id', $newtags, 'IN')->execute();
  }
  foreach ($made['course'] as $c) { db_delete('dh_course')->condition('c_id', $c)->execute(); db_delete('dh_course_log')->condition('crl_course', $c)->execute(); }
  if ($made['centre']) {
    $rids = db_query("select cr_id from dh_center_room where cr_center = :c", array(':c' => $made['centre']))->fetchCol();
    if ($rids) db_delete('dh_center_room_place')->condition('crp_room', $rids, 'IN')->execute();
    db_delete('dh_center_room')->condition('cr_center', $made['centre'])->execute();
    db_delete('dh_center')->condition('c_id', $made['centre'])->execute();
  }
  @unlink($runner);
  echo '  cleanup: ' . count($made['app']) . ' made-up applicants, ' . count($made['course']) . ' courses, ' . count($newtags) . " new system tags removed\n";
}
t_ok((int) db_query("select count(*) from dh_applicant where a_f_name = 'Zzqtest' and a_email = 'zzqtest-zds@example.invalid'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_center where c_name like 'Zzqtest Centre%'")->fetchField() === 0, 'ZS7 nothing left behind');
t_done();
