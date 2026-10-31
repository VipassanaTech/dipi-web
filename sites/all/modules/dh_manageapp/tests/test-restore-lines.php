<?php
require __DIR__ . '/lib.php';
echo "Restore: tidying the old lines (rooms, seats, copies)\n";
dh_restore_source_create();                        // outside any transaction: a CREATE TABLE commits
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
function t_line_row($id) { return db_query('select * from dh_student_course where sc_id = :i', array(':i' => $id))->fetchAssoc(); }
function t_there($id) { return t_n('select count(*) from dh_student_course where sc_id = :i', array(':i' => $id)); }
// An old line; with $rs (an array) also its old row.
function t_line($sid, $course, $conf, $line = array(), $rs = NULL) {
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $course));
  if ($rs !== NULL)
    db_insert('dh_restore_source')->fields(array_merge(array('rs_src' => 'b', 'rs_old_id' => 1, 'rs_course' => $course, 'rs_conf_no' => $conf,
      'rs_f_name' => 'Zzqtidy', 'rs_l_name' => 'Zzqlines', 'rs_gender' => 'M'), $rs))->execute();
  return (int) db_insert('dh_student_course')->fields(array_merge(array('sc_student' => $sid, 'sc_center' => $centre, 'sc_course' => $course,
    'sc_conf_no' => $conf, 'from_migration_script' => 'Yes'), $line))->execute();
}
$LOG = array('sl_batch' => 'test-rs-lines', 'sl_user' => dh_system_uid());

t_in_txn(function () use ($LOG) {
  $sid = t_make_student(array('s_l_name' => 'Zzqlines'));
  $c = t_make_course(array('c_finalized' => 1));
  $room = t_line($sid, $c, 'ZQ 1', array(), array('rs_acco' => 'Block-12'));
  $dash = t_line($sid, $c, 'ZQ 2', array('sc_acco' => '-'), array('rs_acco' => 'Block-13'));
  $kept = t_line($sid, $c, 'ZQ 3', array('sc_acco' => 'X-1'), array('rs_acco' => 'Block-14'));
  $seat = t_line($sid, $c, 'ZQ 4', array(), array('rs_seat' => '17'));
  $noseat = t_line($sid, $c, 'ZQ 5', array(), array('rs_seat' => 'A1'));
  $zero = t_line($sid, $c, 'ZQ 6', array(), array('rs_seat' => '0'));
  $hasrow = t_line($sid, $c, 'ZQ 7', array('sc_seat_row' => 2, 'sc_seat_col' => 3), array('rs_seat' => '9'));
  $first = t_line($sid, $c, 'ZQ 8', array(), array());
  $copy = t_line($sid, $c, 'ZQ 8');                                   // the same old application twice
  $other = t_line($sid, $c, 'ZQ 9', array(), array());                // same course, another application
  $normal = (int) db_insert('dh_student_course')->fields(array('sc_student' => $sid, 'sc_center' => t_n('select c_center from dh_course where c_id = :c', array(':c' => $c)),
    'sc_course' => $c, 'sc_conf_no' => 'ZQ 1'))->execute();          // not an old line
  t_eq(dh_restore_tidy_lines($sid, $LOG), array('rooms' => 2, 'seats' => 1, 'copies' => 1), 'L1: two rooms, one seat, one copy');
  t_eq(array(t_line_row($room)['sc_acco'], t_line_row($dash)['sc_acco'], t_line_row($kept)['sc_acco']), array('Block-12', 'Block-13', 'X-1'),
    'L2: an empty room and a dash are filled; a room that is there is kept');
  t_eq(array(t_line_row($seat)['sc_seat_row'], t_line_row($seat)['sc_seat_col']), array(NULL, 17), 'L3: the seat number goes in the seat column, no row');
  t_eq(array(t_line_row($noseat)['sc_seat_col'], t_line_row($zero)['sc_seat_col'], t_line_row($hasrow)['sc_seat_col']), array(NULL, NULL, 3),
    'L3: a non-number, a zero and a seat that is there are left alone');
  t_eq(array(t_there($first), t_there($copy), t_there($other)), array(1, 0, 1), 'L4: the copy goes; the first line and another application stay');
  t_eq(t_n("select count(*) from dh_student_log where sl_batch = 'test-rs-lines' and sl_action = 'copy-removed'"), 1, 'L4: the copy is logged');
  t_eq(t_line_row($normal)['sc_acco'], NULL, 'L5: a line that is not old is never touched');
  t_eq(dh_restore_tidy_lines($sid, $LOG), array('rooms' => 0, 'seats' => 0, 'copies' => 0), 'L6: a second run changes nothing');
});
t_done();
