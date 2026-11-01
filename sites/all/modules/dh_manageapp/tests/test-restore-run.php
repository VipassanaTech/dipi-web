<?php
require __DIR__ . '/lib.php';
require_once dirname(__DIR__) . '/inc/student-restore.inc';
echo "Restore: the runner (name-grouped chunks, dry run, real run, run again)\n";
dh_restore_source_create();                        // outside any transaction: a CREATE TABLE commits
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
function t_free_sid() {
  static $n = 270000;
  do { $n++; } while (t_n('select (select count(*) from dh_student where s_id = :a) + (select count(*) from dh_student_course where sc_student = :b)
    + (select count(*) from dh_student_course_input where sci_student = :c)', array(':a' => $n, ':b' => $n, ':c' => $n)) > 0);
  return $n;
}
function t_old_course($start) { return t_make_course(array('c_finalized' => 1, 'c_start' => $start, 'c_end' => date('Y-m-d', strtotime($start . ' +10 days')))); }
function t_old($sid, $course, $conf, $rs = array()) {
  $centre = t_n('select c_center from dh_course where c_id = :c', array(':c' => $course));
  db_insert('dh_restore_source')->fields(array_merge(array('rs_src' => 'b', 'rs_old_id' => 1, 'rs_course' => $course, 'rs_conf_no' => $conf, 'rs_type' => 'OM',
    'rs_f_name' => 'Zzqold', 'rs_l_name' => 'Zzqperson', 'rs_gender' => 'M'), $rs))->execute();
  return (int) db_insert('dh_student_course')->fields(array('sc_student' => $sid, 'sc_center' => $centre, 'sc_course' => $course, 'sc_conf_no' => $conf,
    'from_migration_script' => 'Yes'))->execute();
}
function t_snap($ids) {
  return array(
    'lines' => db_query('select * from dh_student_course where sc_student in (:s) order by sc_id', array(':s' => $ids))->fetchAll(PDO::FETCH_ASSOC),
    'students' => db_query('select * from dh_student where s_id in (:s) order by s_id', array(':s' => $ids))->fetchAll(PDO::FETCH_ASSOC));
}

// A batch id that simulates a deadlock. The runner writes the batch id into every log row, which turns it into text. The second time
// that happens (the second person of a chunk), this releases the person's savepoint on the server, as if the database had rolled the
// whole chunk back, and throws. The runner's own rollback to that savepoint then fails with "SAVEPOINT ... does not exist".
class T_Deadlock {
  public $texts = 0;
  function __toString() {
    $this->texts++;
    if ($this->texts == 2) {
      $c = Database::getConnection();
      $p = new ReflectionProperty($c, 'transactionLayers'); $p->setAccessible(TRUE);
      $names = array_keys($p->getValue($c));
      $c->query('RELEASE SAVEPOINT ' . end($names));
      throw new Exception('simulated deadlock');
    }
    return 'test-rs-run3';
  }
}

$x = array(array('sid' => 1, 'l' => 'Zzqa', 'f' => 'Zzqx'), array('sid' => 2, 'l' => 'Zzqa', 'f' => 'Zzqx'), array('sid' => 3, 'l' => 'ZZQA', 'f' => 'zzqx '),
  array('sid' => 4, 'l' => 'Zzqb', 'f' => 'Zzqx'));
t_eq(dh_restore_chunks($x, 2), array(array(1, 2, 3), array(4)), 'T1: a chunk never splits a name (the database compares: any case, trailing space)');
t_eq(dh_restore_chunks($x, 10), array(array(1, 2, 3, 4)), 'T1: a short list is one chunk');

t_in_txn(function () {
  $cur = t_make_student(array('s_f_name' => 'Zzqrun', 's_l_name' => 'Zzqmerge', 's_gender' => 'M', 's_email' => 'zzq.run1@example.com'));
  $r = t_free_sid(); $m = t_free_sid(); $s1 = t_free_sid(); $s2 = t_free_sid(); $u = t_free_sid();
  $c1 = t_old_course('2008-03-01'); $c2 = t_old_course('2011-03-01');
  t_old($r, $c1, 'ZQ 1', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqrestore', 'rs_acco' => 'Block-1'));
  t_old($r, $c1, 'ZQ 1', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqrestore', 'rs_src' => 'c'));          // one application in both tables
  t_old($m, $c2, 'ZQ 2', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqmerge', 'rs_email' => 'zzq.run1@example.com'));
  t_old($s1, $c1, 'ZQ 3', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqsplit', 'rs_email' => 'zzq.run2@example.com'));
  t_old($s2, $c2, 'ZQ 4', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqsplit', 'rs_email' => 'zzq.run2@example.com'));
  t_old($u, $c1, 'ZQ 5', array('rs_l_name' => 'Zzqunclear'));
  t_old($u, $c2, 'ZQ 6', array('rs_l_name' => 'Zzqother'));
  $nums = array($r, $m, $s1, $s2, $u);
  $ids = array_merge($nums, array($cur));
  $before = t_snap($ids);
  $dry = dh_restore_run(FALSE, array('numbers' => $nums, 'chunk' => 1));
  t_eq(t_snap($ids), $before, 'T2: the dry run changes nothing');
  t_eq(t_n("select count(*) from dh_student_log where sl_batch = 'restore-dry-run'"), 0, 'T2: the dry run leaves no log rows');
  t_eq(array($dry['numbers'], $dry['restored'], $dry['merged'], $dry['unclear'], $dry['done'], $dry['failed'], $dry['copies'], $dry['rooms']),
    array(5, 2, 2, 1, 0, 0, 1, 1), 'T2: dry run: 2 restored, 2 merged (one into its split twin, even with chunk size 1), 1 unclear, 1 copy, 1 room');
  t_eq($dry['unclear_ids'], array($u), 'T2: the unclear number is listed');
  $real = dh_restore_run(TRUE, array('numbers' => $nums, 'chunk' => 1, 'batch' => 'test-rs-run'));
  t_eq($real, $dry, 'T2: the real run gives the dry run\'s numbers');
  t_eq(t_n('select count(*) from dh_student where s_id in (:s)', array(':s' => array($r, $s1))), 2, 'T2: the real run saved its changes');
  $again = dh_restore_run(TRUE, array('numbers' => $nums, 'chunk' => 1, 'batch' => 'test-rs-run2'));
  t_eq(array($again['numbers'], $again['restored'], $again['merged'], $again['done'], $again['unclear'], $again['copies'], $again['rooms']),
    array(3, 0, 0, 2, 1, 0, 0), 'T3: run again: only done and unclear numbers, nothing changes');
  t_eq(dh_restore_run(FALSE, array('numbers' => $nums, 'limit' => 1))['numbers'], 1, 'T4: limit takes the first numbers only');
});

// T5: the database rolls back the whole chunk (a deadlock): the savepoint of the failing person is gone too. The run stops with a clear
// message, nothing of the chunk is saved, and the person's own message goes to the Drupal log.
t_in_txn(function () {
  $a = t_free_sid(); $b = t_free_sid();
  $c = t_old_course('2008-05-01');
  t_old($a, $c, 'ZQ 20', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqcrasha'));
  t_old($b, $c, 'ZQ 21', array('rs_f_name' => 'Zzqrun', 'rs_l_name' => 'Zzqcrashb'));
  $msg = '';
  try { dh_restore_run(TRUE, array('numbers' => array($a, $b), 'chunk' => 5, 'batch' => new T_Deadlock())); }
  catch (RuntimeException $e) { $msg = $e->getMessage(); }
  t_eq($msg, "The database rolled back chunk $a-$b (deadlock or lost connection); nothing of that chunk was saved. Chunks before it are saved. "
    . "Run the same command again: done numbers are skipped.", 'T5: the run stops with a clear message');
  t_eq(t_n('select count(*) from dh_student where s_id in (:s)', array(':s' => array($a, $b))), 0, 'T5: the first person of the chunk was not saved either');
  t_eq(t_n('select count(*) from dh_student_log where sl_student in (:s)', array(':s' => array($a, $b))), 0, 'T5: no log rows were saved');
  t_eq(t_n("select count(*) from watchdog where type = 'StudentRestore' and variables like :v", array(':v' => '%simulated deadlock%')), 1,
    'T5: the person\'s own message went to the Drupal log');
  $again = dh_restore_run(TRUE, array('numbers' => array($a, $b), 'chunk' => 5, 'batch' => 'test-rs-run3'));
  t_eq(array($again['restored'], $again['failed']), array(2, 0), 'T5: run again restores both');
});
t_done();
