<?php
// Manage Fields (inc/letters.inc dh_letter_fields_editor) and new-centre copies (inc/centre.inc
// copy_letters_to_centre): who is recorded, and another centre's field is refused.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-fields.php
require __DIR__ . '/lib.php';
include_once 'sites/all/libraries/editor/php/DataTables.php';   // gives $db, the Editor's own connection

// One Manage Fields request for $centre as $uid, on the Editor connection; returns the response array.
function tf_req($db, $centre, $uid, $post) {
  return dh_letter_fields_editor($db, $centre, $uid)->transaction(false)->process($post)->data();
}
function tf_log($db, $id) {
  return $db->sql("select ll_action, ll_field, ll_old, ll_new, ll_user from dh_letter_log where ll_area = 'field' and ll_record = ".(int) $id." order by ll_id")->fetchAll();
}
function tf_last($db, $id) { $log = tf_log($db, $id); return end($log); }
function tf_row($db, $id) { return $db->sql("select * from dh_letter_fields where lf_id = ".(int) $id)->fetch(); }
// Same, and collects the PHP warnings/notices the request raised into $warn.
function tf_req_w($db, $centre, $uid, $post, &$warn) {
  $warn = array();
  set_error_handler(function ($no, $str) use (&$warn) { $warn[] = $str; return TRUE; });
  try { return tf_req($db, $centre, $uid, $post); } finally { restore_error_handler(); }
}
function tf_edit($id, $name, $value, $deleted) {
  return array('action' => 'edit', 'data' => array("row_$id" => array('dh_letter_fields' => array('lf_name' => $name, 'lf_value' => $value, 'lf_deleted' => $deleted))));
}

$db->transaction();
try {
  $res = tf_req($db, 9900001, 777041, array('action' => 'create', 'data' => array(0 => array('dh_letter_fields' => array('lf_name' => 'Zzqtest Field', 'lf_value' => 'Value one', 'lf_deleted' => 0)))));
  t_ok(empty($res['error']) && empty($res['fieldErrors']), 'E1 add a field');
  $id = (int) str_replace('row_', '', $res['data'][0]['DT_RowId']);
  $row = tf_row($db, $id);
  t_eq(array($row['lf_center'], $row['lf_name'], $row['lf_value'], $row['lf_deleted'], $row['lf_updated_by']), array(9900001, 'Zzqtest Field', 'Value one', 0, 777041), 'E1 stored as before, plus who');
  t_ok(!array_key_exists('lf_updated_by', $res['data'][0]['dh_letter_fields']), 'E1 who is not sent to the browser');
  t_eq(array_column(tf_log($db, $id), 'll_user'), array(777041, 777041), 'E1 logged as created (name, value), by who');

  tf_req($db, 9900001, 777042, tf_edit($id, 'Zzqtest Field', 'Value two', 0));
  $last = tf_last($db, $id);
  t_eq(array($last['ll_action'], $last['ll_field'], $last['ll_new'], $last['ll_user']), array('update', 'lf_value', 'Value two', 777042), 'E2 an edit is logged, by the editor');
  tf_req($db, 9900001, 777043, tf_edit($id, 'Zzqtest Field', 'Value two', 1));
  $last = tf_last($db, $id);
  t_eq(array($last['ll_action'], $last['ll_user']), array('delete', 777043), 'E3 Remove is logged, by who');

  $other = (int) $db->insert('dh_letter_fields', array('lf_center' => 9900002, 'lf_name' => 'Zzqtest Other', 'lf_value' => 'x', 'lf_deleted' => 0, 'lf_updated_by' => 777044), 'lf_id')->insertId();
  $res = tf_req($db, 9900001, 777045, tf_edit($other, 'Zzqtest Hacked', 'y', 0));
  t_ok(isset($res['error']) && strpos($res['error'], 'does not belong') !== FALSE, "E4 editing another centre's field is refused");
  $row = tf_row($db, $other);
  t_eq(array($row['lf_center'], $row['lf_name'], $row['lf_value']), array(9900002, 'Zzqtest Other', 'x'), 'E4 and the field is unchanged, not moved');
  t_eq(count(tf_log($db, $other)), 2, 'E4 nothing logged but its creation');

  // E5: only create / edit / remove are accepted (and a read, which has no action). Any other action value is
  // routed by the Editor to its edit code, which saves by row id alone - it must not reach another centre's field.
  $nlog = count(tf_log($db, $other));
  foreach (array('Edit', 'editx', 'read', '') as $odd) {
    $post = tf_edit($other, 'Zzqtest Hacked', 'y', 0);
    $post['action'] = $odd;
    $res = tf_req($db, 9900001, 777045, $post);
    $row = tf_row($db, $other);
    t_ok(isset($res['error']) && $res['error'] === 'Unknown action.' && empty($res['data']), "E5 action '$odd' on another centre's field is refused");
    t_eq(array($row['lf_center'], $row['lf_name'], $row['lf_value'], count(tf_log($db, $other))), array(9900002, 'Zzqtest Other', 'x', $nlog), "E5 action '$odd': field unchanged, nothing logged");
  }
  foreach (array('array' => array('edit'), 'zero' => 0) as $what => $odd) {
    $post = tf_edit($other, 'Zzqtest Hacked', 'y', 0);
    $post['action'] = $odd;
    $res = tf_req_w($db, 9900001, 777045, $post, $warn);
    $row = tf_row($db, $other);
    t_ok(isset($res['error']) && $res['error'] === 'Unknown action.' && empty($res['data']), "E5 action as $what is refused");
    t_eq(array($row['lf_center'], $row['lf_name'], $row['lf_value'], count(tf_log($db, $other)), $warn), array(9900002, 'Zzqtest Other', 'x', $nlog, array()), "E5 action as $what: field unchanged, nothing logged, no PHP warnings");
  }

  // E6: a request that names an own row and another centre's row is refused as a whole.
  $own = (int) $db->insert('dh_letter_fields', array('lf_center' => 9900001, 'lf_name' => 'Zzqtest Own', 'lf_value' => 'a', 'lf_deleted' => 0, 'lf_updated_by' => 777044), 'lf_id')->insertId();
  $nown = count(tf_log($db, $own));
  $post = array('action' => 'edit', 'data' => array(
    "row_$own" => array('dh_letter_fields' => array('lf_name' => 'Zzqtest Own', 'lf_value' => 'changed', 'lf_deleted' => 0)),
    "row_$other" => array('dh_letter_fields' => array('lf_name' => 'Zzqtest Hacked', 'lf_value' => 'y', 'lf_deleted' => 0))));
  $res = tf_req($db, 9900001, 777045, $post);
  t_ok(isset($res['error']) && strpos($res['error'], 'does not belong') !== FALSE, 'E6 a two-row edit with another centre\'s row is refused');
  $r1 = tf_row($db, $own); $r2 = tf_row($db, $other);
  t_eq(array($r1['lf_value'], $r2['lf_center'], $r2['lf_name'], $r2['lf_value'], count(tf_log($db, $own)), count(tf_log($db, $other))), array('a', 9900002, 'Zzqtest Other', 'x', $nown, $nlog), 'E6 and neither row changed, nothing logged');

  // E7: a row that does not exist is refused.
  $missing = 2000000000;
  t_ok(!tf_row($db, $missing), 'E7 setup: the row id is free');
  $res = tf_req($db, 9900001, 777045, tf_edit($missing, 'Zzqtest Ghost', 'y', 0));
  t_ok(isset($res['error']) && strpos($res['error'], 'does not belong') !== FALSE, 'E7 editing a row that does not exist is refused');

  // E8: an edit without a usable data part is refused, without PHP warnings.
  foreach (array('no data' => array('action' => 'edit'), 'empty data' => array('action' => 'edit', 'data' => array()), 'data not an array' => array('action' => 'edit', 'data' => 'x')) as $what => $post) {
    $res = tf_req_w($db, 9900001, 777045, $post, $warn);
    t_ok(isset($res['error']) && $res['error'] === 'Nothing to save.', "E8 edit with $what is refused");
    t_eq($warn, array(), "E8 edit with $what raises no PHP warnings");
  }

  // E9: the centre may arrive as the URL text (06 for 6): an own field still saves, another centre's is still refused.
  $res = tf_req($db, '0' . 9900001, 777047, tf_edit($own, 'Zzqtest Own', 'b', 0));
  t_ok(empty($res['error']) && empty($res['fieldErrors']), 'E9 centre given as 0-padded text: own field saves');
  $r1 = tf_row($db, $own);
  t_eq(array($r1['lf_value'], $r1['lf_updated_by']), array('b', 777047), 'E9 and it is stored, by who');
  $res = tf_req($db, '0' . 9900001, 777047, tf_edit($other, 'Zzqtest Hacked', 'y', 0));
  t_ok(isset($res['error']) && strpos($res['error'], 'does not belong') !== FALSE, 'E9 0-padded centre: another centre\'s field is still refused');

  // E10: normal use still works: two own rows in one edit; a read (no action) lists this centre's fields only.
  $own2 = (int) $db->insert('dh_letter_fields', array('lf_center' => 9900001, 'lf_name' => 'Zzqtest Own Two', 'lf_value' => 'c', 'lf_deleted' => 0, 'lf_updated_by' => 777044), 'lf_id')->insertId();
  $post = array('action' => 'edit', 'data' => array(
    "row_$own" => array('dh_letter_fields' => array('lf_name' => 'Zzqtest Own', 'lf_value' => 'd', 'lf_deleted' => 0)),
    "row_$own2" => array('dh_letter_fields' => array('lf_name' => 'Zzqtest Own Two', 'lf_value' => 'e', 'lf_deleted' => 0))));
  $res = tf_req($db, 9900001, 777048, $post);
  t_ok(empty($res['error']) && empty($res['fieldErrors']) && count($res['data']) == 2, 'E10 a two-row edit of own fields saves');
  t_eq(array(tf_row($db, $own)['lf_value'], tf_row($db, $own2)['lf_value']), array('d', 'e'), 'E10 both rows stored');
  $res = tf_req_w($db, 9900001, 777048, array(), $warn);
  $ids = array_column($res['data'], 'DT_RowId');
  t_ok(empty($res['error']) && in_array("row_$own", $ids) && in_array("row_$own2", $ids) && !in_array("row_$other", $ids), 'E10 a read lists this centre\'s fields, not the other centre\'s');
  t_eq($warn, array(), 'E10 a read raises no PHP warnings');
} catch (Throwable $e) {
  t_ok(FALSE, 'unexpected error: ' . $e->getMessage());
}
$db->rollback();

t_in_txn(function () {
  $GLOBALS['user'] = (object) array('uid' => 777046, 'roles' => array());
  $nl = (int) db_query("select count(*) from dh_letter where l_center = 0")->fetchField();
  $nf = (int) db_query("select count(*) from dh_letter_fields where lf_center = 0")->fetchField();
  t_ok($nl > 0 && $nf > 0, 'C0 local has template letters and fields (centre 0)');
  copy_letters_to_centre(9900003);
  t_eq(db_query("select count(distinct ll_record) from dh_letter_log where ll_area = 'letter' and ll_center = 9900003 and ll_action = 'create' and ll_user = 777046")->fetchField(), $nl, 'C1 every template letter logged as created, by who added the centre');
  t_eq(db_query("select count(distinct ll_record) from dh_letter_log where ll_area = 'field' and ll_center = 9900003 and ll_action = 'create' and ll_user = 777046")->fetchField(), $nf, 'C2 every template field logged as created, by who added the centre');
  t_eq(db_query("select count(*) from dh_letter_fields where lf_center = 9900003 and lf_updated_by = 777046")->fetchField(), $nf, 'C3 the copied fields record who');
});
t_done();
