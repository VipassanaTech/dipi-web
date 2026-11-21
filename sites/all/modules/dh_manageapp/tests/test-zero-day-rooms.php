<?php
// Zero Day save: a place another course uses at the same time needs "use it anyway" (ov=1); an unchanged place does not
// ask again; a saved place with a leading space still counts as taken. Each save runs in its own PHP process (like one
// request), on made-up centre/courses/applicants that are deleted at the end.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-zero-day-rooms.php
require __DIR__ . '/lib.php';

$runner = tempnam(sys_get_temp_dir(), 'zzq-zd-rooms-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$in = json_decode($argv[1], TRUE); $GLOBALS["user"] = user_load(1); $_GET["q"] = "app-update-attended/" . (int) $in["app"];
$_REQUEST = $_POST = $in["req"]; dh_app_update_attended();
');
$made = array('centre' => 0, 'course' => array(), 'app' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_tag_log' => $max('dh_tag_log', 'tl_id'), 'dh_application_tag' => $max('dh_application_tag', 'at_id'),
  'dh_application_tag_log' => $max('dh_application_tag_log', 'atl_id'), 'dh_log' => $max('dh_log', 'l_id'));
try {
  $made['centre'] = $cen = t_make_centre();
  t_make_room($cen, 'M', 'B', '12', array('12A', '12B'));
  t_make_room($cen, 'M', 'B', '14', array('14A', '14B'));
  $A = $made['course'][] = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d'), 'c_end' => date('Y-m-d', strtotime('+10 days')), 'c_processed' => 0, 'c_finalize_mail' => 0));
  $B = $made['course'][] = t_make_course(array('c_center' => $cen, 'c_start' => date('Y-m-d', strtotime('+1 day')), 'c_end' => date('Y-m-d', strtotime('+4 days')), 'c_processed' => 0, 'c_finalize_mail' => 0));
  $made['app'][] = t_make_applicant($B, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => '12A'));
  $made['app'][] = t_make_applicant($A, array('a_gender' => 'M'), array('aa_section' => 'B', 'aa_acco' => ' 14A'));
  $new = function () use ($A, &$made) { $id = t_make_applicant($A, array('a_attended' => 0, 'a_gender' => 'M', 'a_email' => 'zzqtest-zdr@example.invalid')); db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute(); $made['app'][] = $id; return $id; };
  $save = function ($id, $s, $r, $ov = '') use ($runner) {
    $req = array('a' => 'true', 's' => $s, 'r' => $r, 'g' => '1', 'l' => '', 'v' => '', 'c' => '', 'cf' => '0', 'gc' => '', 'gcf' => '0', 'dn' => '', 'df' => '0', 'gdn' => '', 'gdf' => '0', 'chow' => '0', 'chai' => '0', 'back' => '0', 'comment' => '', 'ov' => $ov);
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array('app' => $id, 'req' => $req))) . ' 2>/dev/null');
    $j = json_decode(substr($out, strpos($out, '{')), TRUE);
    return is_array($j) ? (!empty($j['status']) ? 'ok' : (string) $j['msg']) : 'no reply';
  };
  $x = $new();
  $res = $save($x, 'B', '12A');
  t_ok(strpos($res, 'This place is used by') === 0 && strpos($res, 'Choose it again and confirm to use it anyway.') !== FALSE, 'Z1 a place of an overlapping course is refused without ov=1 (' . $res . ')');
  t_eq($save($x, 'B', '12A', '1'), 'ok', 'Z2 with ov=1 it is saved');
  t_eq($save($x, 'B', '12A'), 'ok', 'Z3 saving again with the same place does not ask again');
  t_eq($save($new(), 'B', '12B'), 'ok', 'Z4 a free place is saved as before');
  t_eq($save($new(), 'B', '14A'), 'Room has already been alloted', 'Z5 a place saved with a leading space (" 14A") still counts as taken');
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
t_ok((int) db_query("select count(*) from dh_applicant where a_f_name = 'Zzqtest' and a_email = 'zzqtest-zdr@example.invalid'")->fetchField() === 0
  && (int) db_query("select count(*) from dh_center where c_name like 'Zzqtest Centre%'")->fetchField() === 0, 'Z6 nothing left behind');
t_done();
