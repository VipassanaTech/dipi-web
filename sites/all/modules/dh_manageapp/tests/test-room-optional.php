<?php
// Zero Day: a room is optional for 1 Day Course and Children Course (dh_room_optional); other courses still need
// a section and a room up to the start day. Each save runs as Zero Day calls it (one PHP process per save, like
// one request), on made-up courses and applicants (status already Confirmed, so no status change) that are
// deleted at the end.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-room-optional.php
require __DIR__ . '/lib.php';

$type = db_query("select td_key, td_id from dh_type_detail where td_key in ('1-DayOSC', 'Child-Teen', '10-Day', '7-DayTeen')")->fetchAllKeyed();
t_ok(count($type) === 4, 'R0 test data: the four course types exist locally');
t_eq(dh_room_optional_types(), array('1-DayOSC', 'Child-Teen'), 'R1 room is optional only for 1 Day Course and Children Course');

$runner = tempnam(sys_get_temp_dir(), 'zzq-room-save-');
file_put_contents($runner, '<?php
define("DRUPAL_ROOT", getcwd()); $_SERVER["REMOTE_ADDR"] = "127.0.0.1"; $_SERVER["HTTP_HOST"] = "localhost"; $_SERVER["SCRIPT_NAME"] = "/index.php";
require_once DRUPAL_ROOT . "/includes/bootstrap.inc"; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (stripos(db_query("select @@hostname")->fetchField(), "vri") !== FALSE) exit("refusing\n");
$in = json_decode($argv[1], TRUE);
$GLOBALS["user"] = user_load(1);
$_GET["q"] = "app-update-attended/" . (int) $in["app"];
$_REQUEST = $_POST = $in["req"];
dh_app_update_attended();
');
$made = array('course' => array(), 'app' => array());
$max = function ($t, $col) { return (int) db_query("select coalesce(max($col), 0) from $t")->fetchField(); };
$start = array('dh_tag' => $max('dh_tag', 't_id'), 'dh_tag_log' => $max('dh_tag_log', 'tl_id'), 'dh_application_tag' => $max('dh_application_tag', 'at_id'),
  'dh_application_tag_log' => $max('dh_application_tag_log', 'atl_id'), 'dh_log' => $max('dh_log', 'l_id'));

try {
  foreach (array('1-DayOSC' => TRUE, 'Child-Teen' => TRUE, '10-Day' => FALSE, '7-DayTeen' => FALSE) as $k => $opt) {
    $made['course'][$k] = t_make_course(array('c_course_type' => $type[$k], 'c_name' => "Zzqtest $k", 'c_start' => date('Y-m-d'),
      'c_end' => date('Y-m-d', strtotime('+1 day')), 'c_processed' => 0, 'c_finalize_mail' => 0));
    t_eq(dh_room_optional($made['course'][$k]), $opt, "R2 $k: room " . ($opt ? 'optional' : 'required'));
  }
  t_eq(dh_room_optional(0), FALSE, 'R3 a course that does not exist: room required (as before)');

  $app = function ($k) use (&$made) {
    $id = t_make_applicant($made['course'][$k], array('a_attended' => 0, 'a_email' => 'zzqtest-room@example.invalid'));
    db_delete('dh_applicant_attended')->condition('aa_applicant', $id)->execute();
    $made['app'][] = $id;
    return $id;
  };
  $save = function ($id, $s, $r) use ($runner) {
    $req = array('a' => 'true', 's' => $s, 'r' => $r, 'g' => '1', 'l' => '', 'v' => '', 'c' => '', 'cf' => '0', 'gc' => '', 'gcf' => '0',
      'dn' => '', 'df' => '0', 'gdn' => '', 'gdf' => '0', 'chow' => '0', 'chai' => '0', 'back' => '0', 'comment' => '');
    $out = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg(json_encode(array('app' => $id, 'req' => $req))) . ' 2>/dev/null');
    $j = json_decode(substr($out, strpos($out, '{')), TRUE);
    $row = db_query("select a.a_attended, a.a_status, aa.aa_section, aa.aa_acco from dh_applicant a left join dh_applicant_attended aa on aa.aa_applicant = a.a_id where a.a_id = :a",
      array(':a' => $id))->fetchAssoc();
    return array(is_array($j) ? (!empty($j['status']) ? 'ok' : (string) $j['msg']) : 'no reply', $row);
  };

  list($res, $row) = $save($app('1-DayOSC'), '', '');
  t_ok($res === 'ok' && $row['a_attended'] == 1 && (string) $row['aa_section'] === '' && (string) $row['aa_acco'] === '', 'S1 1 Day Course, no room: marked arrived (' . $res . ')');
  list($res, $row) = $save($app('Child-Teen'), '', '');
  t_ok($res === 'ok' && $row['a_attended'] == 1, 'S2 Children Course, no room: marked arrived (' . $res . ')');
  list($res, $row) = $save($app('10-Day'), '', '');
  t_ok($res === 'Please select room section' && $row['a_attended'] == 0 && $row['aa_section'] === NULL, 'S3 10 Day Course, no room: refused "' . $res . '", not marked arrived');
  list($res, $row) = $save($app('10-Day'), 'A', '');
  t_ok($res === 'Please select room no' && $row['a_attended'] == 0, 'S4 10 Day Course, section but no room: refused "' . $res . '"');
  list($res, $row) = $save($app('7-DayTeen'), '', '');
  t_ok($res === 'Please select room section' && $row['a_attended'] == 0, 'S5 Teenager Course, no room: still refused "' . $res . '"');
  list($res, $row) = $save($app('1-DayOSC'), 'Zq', '7');
  t_ok($res === 'ok' && $row['aa_section'] === 'Zq' && $row['aa_acco'] === '7', 'S6 1 Day Course with a room: the room is saved (still allowed)');
  list($res, $row) = $save($app('10-Day'), 'Zq', '8');
  t_ok($res === 'ok' && $row['a_attended'] == 1 && $row['aa_acco'] === '8', 'S7 10 Day Course with a room: saved as before');
  list($res, $row) = $save($app('1-DayOSC'), '', '');
  t_ok($res === 'ok' && $row['a_attended'] == 1, 'S10 a SECOND 1 Day Course student with no room: also marked arrived, no "already alloted" (' . $res . ')');
  list($res, $row) = $save($app('Child-Teen'), '', '');
  t_ok($res === 'ok' && $row['a_attended'] == 1, 'S11 a SECOND Children Course student with no room: also marked arrived (' . $res . ')');
  list($res, $row) = $save($app('1-DayOSC'), 'Zq', '7');
  t_ok($res === 'Room has already been alloted' && $row['a_attended'] == 0, 'S12 1 Day Course: a room already given (Zq-7) is still refused "' . $res . '"');
  list($res, $row) = $save($app('10-Day'), 'Zq', '8');
  t_ok($res === 'Room has already been alloted' && $row['a_attended'] == 0, 'S13 10 Day Course: a room already given (Zq-8) is still refused "' . $res . '"');
  t_eq(db_query("select count(*) from dh_applicant where a_id in (:a) and a_status <> 'Confirmed'", array(':a' => $made['app']))->fetchField(), 0,
    'S8 no status changed (no letters can follow)');
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
  foreach ($made['course'] as $c) {
    db_delete('dh_course')->condition('c_id', $c)->execute();
    db_delete('dh_course_log')->condition('crl_course', $c)->execute();
  }
  @unlink($runner);
  echo '  cleanup: ' . count($made['app']) . ' made-up applicants, ' . count($made['course']) . ' courses, ' . count($newtags) . " new system tags removed\n";
}
t_ok((int) db_query("select count(*) from dh_applicant where a_f_name = 'Zzqtest' and a_email = 'zzqtest-room@example.invalid'")->fetchField() === 0, 'S9 nothing left behind');
t_done();
