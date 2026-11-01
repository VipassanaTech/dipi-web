<?php
// Letter change log triggers (deploy/2026-11-01-letter-change-log.php): one log row per changed
// letter / merge field, with who and when. Made-up rows in rolled-back transactions.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-log-triggers.php
require __DIR__ . '/lib.php';

t_ok(db_table_exists('dh_letter_log'), 'dh_letter_log exists (run the installer first)');
if (!db_table_exists('dh_letter_log'))
  t_done();
$modes = db_query("select trigger_name, sql_mode from information_schema.triggers where trigger_schema = database() and event_object_table in ('dh_letter', 'dh_letter_fields')")->fetchAllKeyed();
t_eq(count($modes), 5, 'five letter triggers');
foreach ($modes as $t => $m)
  t_eq($m, 'ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER', "$t uses prod's sql_mode");

// A made-up letter of fake centre 9900001, created by 777001. Returns its id.
function tl_letter($over = array()) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => 9900001, 'l_event' => 'Received', 'l_course_type' => 0,
    'l_name' => 'Zzqtest Letter', 'l_subject' => 'Zzqtest subject', 'l_body' => '<p>Line one</p><p>Line two</p>', 'l_sms' => '',
    'l_deleted' => 0, 'l_created' => $now, 'l_created_by' => 777001, 'l_updated' => $now, 'l_updated_by' => 777001), $over))->execute();
}
// The log rows of one letter or field, oldest first.
function tl_rows($area, $id) {
  return db_query("select ll_center, ll_name, ll_action, ll_field, ll_old, ll_new, ll_user from dh_letter_log where ll_area = :a and ll_record = :r order by ll_id",
    array(':a' => $area, ':r' => $id))->fetchAll(PDO::FETCH_ASSOC);
}
// A letter save the way the site does it: the change plus who and when.
function tl_upd($id, $fields, $uid) {
  db_update('dh_letter')->fields($fields + array('l_updated' => date('Y-m-d H:i:s'), 'l_updated_by' => $uid))->condition('l_id', $id)->execute();
}

t_in_txn(function () {
  $id = tl_letter();
  $r = tl_rows('letter', $id);
  t_eq(array_column($r, 'll_field'), array('l_name', 'l_event', 'l_subject', 'l_body'), 'T1 a new letter logs its filled fields only');
  t_eq(array_values(array_unique(array_column($r, 'll_action'))), array('create'), 'T1 as create');
  t_eq(array_values(array_unique(array_column($r, 'll_user'))), array(777001), 'T1 by the creator');
  t_eq(array($r[0]['ll_center'], $r[0]['ll_name'], $r[3]['ll_new']), array(9900001, 'Zzqtest Letter', '<p>Line one</p><p>Line two</p>'), 'T1 centre, name and the body in full');

  $n = count(tl_rows('letter', $id));
  tl_upd($id, array('l_subject' => 'Zzqtest subject 2'), 777002);
  t_eq(array_slice(tl_rows('letter', $id), $n), array(array('ll_center' => 9900001, 'll_name' => 'Zzqtest Letter', 'll_action' => 'update',
    'll_field' => 'l_subject', 'll_old' => 'Zzqtest subject', 'll_new' => 'Zzqtest subject 2', 'll_user' => 777002)), 'T2 one changed field gives one row, by the editor');

  $n = count(tl_rows('letter', $id));
  tl_upd($id, array('l_subject' => 'Zzqtest subject 2', 'l_body' => '<p>Line one</p><p>Line two</p>'), 777003);
  t_eq(count(tl_rows('letter', $id)), $n, 'T3 a save with no change logs nothing');
  tl_upd($id, array('l_subject' => 'ZZQTEST subject 2'), 777003);
  t_eq(count(tl_rows('letter', $id)), $n + 1, 'T4 a capital-letters-only change is logged');

  $n = count(tl_rows('letter', $id));
  tl_upd($id, array('l_attachment' => ''), 777003);
  t_eq(count(tl_rows('letter', $id)), $n, 'T5 NULL to empty is not a change');
  tl_upd($id, array('l_attachment' => 'private://letters/zzqtest.pdf'), 777003);
  tl_upd($id, array('l_attachment' => NULL), 777004);
  $r = array_slice(tl_rows('letter', $id), $n);
  t_eq(count($r), 2, 'T5 attachment set, then removed: two rows');
  t_eq(array($r[0]['ll_field'], $r[0]['ll_new'], $r[0]['ll_user'], $r[1]['ll_old'], $r[1]['ll_user']),
    array('l_attachment', 'private://letters/zzqtest.pdf', 777003, 'private://letters/zzqtest.pdf', 777004), 'T5 old and new attachment, by who');
  t_ok($r[1]['ll_new'] === NULL, 'T5 removed attachment is NULL in the log');

  $n = count(tl_rows('letter', $id));
  tl_upd($id, array('l_deleted' => 1), 777005);
  tl_upd($id, array('l_deleted' => 0), 777006);
  $r = array_slice(tl_rows('letter', $id), $n);
  t_eq(count($r), 2, 'T6 inactive then active: two rows');
  t_eq(array($r[0]['ll_action'], $r[0]['ll_field'], $r[0]['ll_old'], $r[0]['ll_new'], $r[0]['ll_user']), array('inactive', 'l_deleted', 0, 1, 777005), 'T6 made inactive, by who did it');
  t_eq(array($r[1]['ll_action'], $r[1]['ll_user']), array('active', 777006), 'T6 made active');

  $n = count(tl_rows('letter', $id));
  tl_upd($id, array('l_course_type' => 10), 777007);
  $r = array_slice(tl_rows('letter', $id), $n);
  t_eq(array($r[0]['ll_field'], $r[0]['ll_old'], $r[0]['ll_new']), array('l_course_type', 0, 10), 'T7 course type change');

  $n = count(tl_rows('letter', $id));
  tl_upd($id, array('l_center' => 9900002), 777008);
  t_eq(array_column(array_slice(tl_rows('letter', $id), $n), 'll_center'), array(9900001, 9900002), 'T8 a move is logged under both centres');

  $big = str_repeat('<p>Zzqtest long line of text.</p>', 7000);
  tl_upd($id, array('l_body' => $big), 777009);
  $last = db_query("select ll_new from dh_letter_log where ll_area = 'letter' and ll_record = :r and ll_field = 'l_body' order by ll_id desc limit 1", array(':r' => $id))->fetchField();
  t_eq(strlen($last), strlen($big), 'T9 a 220 KB body is kept in full');

  $n = count(tl_rows('letter', $id));
  db_delete('dh_letter')->condition('l_id', $id)->execute();
  $r = array_slice(tl_rows('letter', $id), $n);
  t_eq(array_values(array_unique(array_column($r, 'll_action'))), array('delete'), 'T10 a removed letter logs delete rows');
  t_ok(in_array('l_body', array_column($r, 'll_field')), 'T10 with its body, so it can be rebuilt');
  t_eq(array($r[0]['ll_user'], $r[0]['ll_center']), array(0, 9900002), 'T10 by unknown (0), under its centre');
});

t_in_txn(function () {
  $fid = (int) db_insert('dh_letter_fields')->fields(array('lf_center' => 9900001, 'lf_name' => 'Zzqtest Field', 'lf_value' => 'Value one',
    'lf_deleted' => 0, 'lf_updated_by' => 777021))->execute();
  $r = tl_rows('field', $fid);
  t_eq(array(array_column($r, 'll_field'), array_values(array_unique(array_column($r, 'll_action'))), $r[0]['ll_user'], $r[0]['ll_name']),
    array(array('lf_name', 'lf_value'), array('create'), 777021, 'Zzqtest Field'), 'F1 a new field logs its name and value');
  db_update('dh_letter_fields')->fields(array('lf_value' => 'Value two', 'lf_updated_by' => 777022))->condition('lf_id', $fid)->execute();
  db_update('dh_letter_fields')->fields(array('lf_deleted' => 1, 'lf_updated_by' => 777023))->condition('lf_id', $fid)->execute();
  db_update('dh_letter_fields')->fields(array('lf_deleted' => 0, 'lf_updated_by' => 777024))->condition('lf_id', $fid)->execute();
  $r = array_slice(tl_rows('field', $fid), 2);
  t_eq(count($r), 3, 'F2 three changes, three rows');
  t_eq(array($r[0]['ll_action'], $r[0]['ll_field'], $r[0]['ll_old'], $r[0]['ll_new'], $r[0]['ll_user']), array('update', 'lf_value', 'Value one', 'Value two', 777022), 'F2 value change');
  t_eq(array($r[1]['ll_action'], $r[1]['ll_user']), array('delete', 777023), 'F3 removed');
  t_eq(array($r[2]['ll_action'], $r[2]['ll_user']), array('active', 777024), 'F4 added back');
  $n = count(tl_rows('field', $fid));
  db_update('dh_letter_fields')->fields(array('lf_value' => 'Value two', 'lf_updated_by' => 777025))->condition('lf_id', $fid)->execute();
  t_eq(count(tl_rows('field', $fid)), $n, 'F5 an unchanged save logs nothing');
});
t_done();
