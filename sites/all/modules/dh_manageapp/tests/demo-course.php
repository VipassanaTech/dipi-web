<?php
// Local only: a made-up ended course with 3 attended applicants (no email, no mobile) for browser checks.
//   php7.4 sites/all/modules/dh_manageapp/tests/demo-course.php --centre=<id>        creates it, prints the id
//   php7.4 sites/all/modules/dh_manageapp/tests/demo-course.php --remove=<course id>  removes it and what finalize made
require __DIR__ . '/lib.php';
// lib.php makes a script that ends without t_done() look like a crash; this one ends by design.
$GLOBALS['t_finished'] = TRUE;
$args = array();
foreach ($argv as $a) if (preg_match('/^--(centre|remove)=(\d+)$/', $a, $m)) $args[$m[1]] = (int) $m[2];
if (!empty($args['remove'])) {
  $c = $args['remove'];
  if (db_query("select c_description from dh_course where c_id = :c", array(':c' => $c))->fetchField() !== 'ZZQ finalize demo') { echo "not a demo course\n"; exit(1); }
  $apps = db_query("select a_id from dh_applicant where a_course = :c", array(':c' => $c))->fetchCol();
  $students = db_query("select distinct sl_student from dh_student_log where sl_course = :c and sl_action = 'created'", array(':c' => $c))->fetchCol();
  db_delete('dh_student_course')->condition('sc_course', $c)->execute();
  if ($students) {
    db_delete('dh_student_course_input')->condition('sci_student', $students, 'IN')->execute();
    db_delete('dh_student')->condition('s_id', $students, 'IN')->condition('s_f_name', 'Zzqtest')->execute();
  }
  db_delete('dh_student_log')->condition('sl_course', $c)->execute();
  db_delete('dh_course_stat')->condition('cs_course', $c)->execute();
  if ($apps) {
    db_delete('dh_applicant_attended')->condition('aa_applicant', $apps, 'IN')->execute();
    db_delete('dh_log')->condition('l_module', 'Applicant')->condition('l_identifier', $apps, 'IN')->execute();
    db_delete('dh_applicant')->condition('a_id', $apps, 'IN')->execute();
  }
  db_delete('dh_course_log')->condition('crl_course', $c)->execute();
  db_delete('dh_course')->condition('c_id', $c)->execute();
  echo "removed demo course $c\n"; exit(0);
}
if (empty($args['centre']) || $args['centre'] == 1) { echo "use --centre=<id, not 1> or --remove=<course id>\n"; exit(1); }
$c = t_make_course(array('c_center' => $args['centre'], 'c_description' => 'ZZQ finalize demo'));
foreach (array('Demoone', 'Demotwo', 'Demothree') as $n) t_make_applicant($c, array('a_l_name' => $n));
echo "demo course $c at centre {$args['centre']}: 3 attended applicants, no email or mobile\n";
