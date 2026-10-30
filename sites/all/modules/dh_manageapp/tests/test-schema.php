<?php
require __DIR__ . '/lib.php';
echo "Schema for the finalize sync\n";
foreach (array(array('dh_student_course', 'sc_applicant'), array('dh_applicant', 'a_attended_mail'), array('dh_course', 'c_finalize_requested'),
  array('dh_course', 'c_finalize_mail'), array('dh_course', 'c_reopened'), array('dh_course', 'c_reopened_by')) as $c)
  t_ok((bool) db_query("select 1 from information_schema.columns where table_schema = database() and table_name = :t and column_name = :c",
    array(':t' => $c[0], ':c' => $c[1]))->fetchField(), "$c[0].$c[1] exists");
t_ok((bool) db_query("select 1 from information_schema.statistics where table_schema = database() and table_name = 'dh_student_course' and index_name = 'sc_applicant'")->fetchField(), 'sc_applicant is indexed');
t_ok((bool) db_query("select 1 from information_schema.statistics where table_schema = database() and table_name = 'dh_patrika' and column_name = 'p_student' and seq_in_index = 1")->fetchField(), 'dh_patrika.p_student is the first column of an index');
t_ok(db_table_exists('dh_student_log'), 'dh_student_log exists');
$perms = module_invoke('dh_manageapp', 'permission');
t_ok(isset($perms['reopen any finalized course']), 'permission declared');
t_done();
