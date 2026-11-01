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
