<?php
require __DIR__ . '/lib.php';
echo "An older course never overwrites a student's newer data\n";
function t_row($table, $key, $id) { return db_query("select * from $table where $key = :i", array(':i' => $id))->fetchAssoc(); }
function t_n($sql, $args = array()) { return (int) db_query($sql, $args)->fetchField(); }
function t_s3_head($bucket, $key) { return $key === 'photos/zzq/older.jpg' ? 1000 : FALSE; }
function t_s3_copy($bucket, $from, $to) { return array('success' => TRUE, 'message' => ''); }

// Which course is "later": later start, or the same start and a higher id.
t_in_txn(function () {
  $c1 = t_make_course(array('c_start' => '2022-02-02', 'c_end' => '2022-02-12'));
  $c2 = t_make_course(array('c_start' => '2022-02-02', 'c_end' => '2022-02-12'));          // same start, higher id
  $c3 = t_make_course(array('c_start' => '2021-01-01', 'c_end' => '2021-01-11'));          // earlier start, highest id
  $centre = (int) t_n("select c_center from dh_course where c_id = :c", array(':c' => $c1));
  $s = t_make_student(array('s_l_name' => 'Order'));
  db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_center' => $centre, 'sc_course' => $c2))->execute();
  t_ok(dh_student_has_newer_course($s, $c1), 'same start, higher course id counts as later');
  t_ok(!dh_student_has_newer_course($s, $c2), 'the course itself is not later than itself');
  t_ok(dh_student_has_newer_course($s, $c3), 'a course with an earlier start is older, whatever its id');
  t_ok(!dh_student_has_newer_course(t_make_student(array('s_l_name' => 'NoCourse')), $c1), 'a student with no course record has nothing later');
  $s2 = t_make_student(array('s_l_name' => 'Order2'));
  db_insert('dh_student_course')->fields(array('sc_student' => $s2, 'sc_center' => $centre, 'sc_course' => $c1))->execute();
  t_ok(!dh_student_has_newer_course($s2, $c2), 'a student whose other course is older is not protected from this one');
});

// dh_student_apply: the older course only fills empty fields.
t_in_txn(function () {
  $older = t_make_course(array('c_start' => '2020-01-01', 'c_end' => '2020-01-11'));
  $newer = t_make_course(array('c_start' => '2021-01-01', 'c_end' => '2021-01-11'));
  $centre = (int) t_n("select c_center from dh_course where c_id = :c", array(':c' => $older));
  $aid = t_make_applicant($older, array('a_l_name' => 'Older', 'a_dob' => '1970-01-01'));
  // The applicant row as the older course has it; the contact and work values exist only in memory (nothing is stored or sent).
  $a = db_query("select * from dh_applicant where a_id = :a", array(':a' => $aid))->fetchAssoc();
  $a = array('a_email' => 'older@example.invalid', 'a_phone_mobile' => 'not-a-phone-old', 'a_occupation' => 'Older job', 'a_company' => 'Older company',
    'a_address' => 'Older address', 'a_photo' => 'private://older/photo.jpg', 'a_education' => 'Older education', 'a_f_name' => 'Zzqtest', 'a_l_name' => 'Older',
    'a_problem_physical' => 1) + $a;
  $keeps = array('s_f_name' => 'Zzqtest', 's_l_name' => 'Newer', 's_dob' => '1970-01-01', 's_gender' => 'M', 's_email' => 'newer@example.invalid', 's_phone_mobile' => 'not-a-phone-new',
    's_occupation' => 'Newer job', 's_company' => 'Newer company', 's_address' => 'Newer address', 's_photo' => 'private://newer/photo.jpg', 's_education' => '', 's_problem_physical' => 0);
  $s = t_make_student($keeps);
  db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_center' => $centre, 'sc_course' => $newer))->execute();
  $ctx = array('course' => $older, 'applicant' => $aid, 'batch' => 'test-older', 'uid' => 0);
  dh_student_apply($s, $a, TRUE, $ctx);                                                  // a STRONG match: still no change
  $r = t_row('dh_student', 's_id', $s);
  foreach (array('s_l_name', 's_email', 's_phone_mobile', 's_occupation', 's_company', 's_address', 's_photo', 's_problem_physical', 's_dob', 's_gender') as $f)
    t_eq($r[$f], $keeps[$f], "newer data kept: $f");
  t_eq($r['s_education'], 'Older education', 'an empty field is still filled');
  t_eq(t_n("select count(*) from dh_student_log where sl_student = :s and sl_action = 'name-change'", array(':s' => $s)), 0, 'no name change is logged');
  $logged = db_query("select sl_field from dh_student_log where sl_student = :s and sl_action = 'updated'", array(':s' => $s))->fetchCol();
  t_eq(array_values(array_intersect($logged, array_diff(array_keys($keeps), array('s_education')))), array(), 'nothing that already had a value is logged as updated');
  t_ok(in_array('s_education', $logged), 'the filled field is logged');

  // Control: the same data from a NEWER course updates a student whose other course is older.
  $s2 = t_make_student($keeps);
  db_insert('dh_student_course')->fields(array('sc_student' => $s2, 'sc_center' => $centre, 'sc_course' => $older))->execute();
  dh_student_apply($s2, $a, FALSE, array('course' => $newer) + $ctx);
  $r2 = t_row('dh_student', 's_id', $s2);
  t_eq(array($r2['s_email'], $r2['s_occupation'], $r2['s_company'], $r2['s_address'], $r2['s_photo']),
    array('older@example.invalid', 'Older job', 'Older company', 'Older address', 'private://older/photo.jpg'), 'control: the same data from a newer course does update the stored values');
});

// dh_course_sync: the older course keeps the numbers row and the photo of a student who has a later course.
t_in_txn(function () {
  $older = t_make_course(array('c_finalized' => 1, 'c_start' => '2020-01-01', 'c_end' => '2020-01-11'));
  $newer = t_make_course(array('c_finalized' => 1, 'c_start' => '2021-01-01', 'c_end' => '2021-01-11'));
  $centre = (int) t_n("select c_center from dh_course where c_id = :c", array(':c' => $older));
  t_make_applicant($older, array('a_l_name' => 'HasNumbers', 'a_dob' => '1975-05-05', 'a_photo' => 'private://photos/zzq/older.jpg'));
  t_make_applicant($older, array('a_l_name' => 'NoNumbers', 'a_dob' => '1976-06-06'));
  $s1 = t_make_student(array('s_l_name' => 'HasNumbers', 's_dob' => '1975-05-05', 's_photo' => 'private://newer/photo.jpg'));
  $s2 = t_make_student(array('s_l_name' => 'NoNumbers', 's_dob' => '1976-06-06'));
  foreach (array($s1, $s2) as $s) db_insert('dh_student_course')->fields(array('sc_student' => $s, 'sc_center' => $centre, 'sc_course' => $newer))->execute();
  $sci1 = (int) db_insert('dh_student_course_input')->fields(array('sci_student' => $s1, 'sci_10d' => 7, 'sci_service' => 3))->execute();
  $before = t_row('dh_student_course_input', 'sci_id', $sci1);
  $s = dh_course_sync($older, array('mail' => FALSE, 'files' => 's3copy', 's3_head' => 't_s3_head', 's3_copy' => 't_s3_copy', 'batch' => 'test-older-sync'));
  t_eq($s['added'], 2, 'both students get a record for the older course');
  t_eq(t_row('dh_student_course_input', 'sci_id', $sci1), $before, 'the existing numbers row is not overwritten');
  t_eq(t_n("select count(*) from dh_student_course_input where sci_student = :s", array(':s' => $s2)), 1, 'a student with no numbers row still gets one');
  t_eq(t_row('dh_student', 's_id', $s1)['s_photo'], 'private://newer/photo.jpg', 'the older course does not replace the photo (s3copy)');
  t_eq(t_n("select count(*) from dh_student_log where sl_batch = 'test-older-sync' and sl_student = :s and (sl_action = 'sci-updated' or sl_field = 's_photo')", array(':s' => $s1)), 0, 'no numbers or photo change is logged for the protected student');

  // The same course against a student whose other course is older: the numbers and photo do update.
  $older2 = t_make_course(array('c_finalized' => 1, 'c_start' => '2019-01-01', 'c_end' => '2019-01-11'));
  db_update('dh_student_course')->fields(array('sc_course' => $older2))->condition('sc_student', $s1)->condition('sc_course', $newer)->execute();
  db_delete('dh_student_course')->condition('sc_student', $s1)->condition('sc_course', $older)->execute();
  dh_course_sync($older, array('mail' => FALSE, 'files' => 's3copy', 's3_head' => 't_s3_head', 's3_copy' => 't_s3_copy'));
  t_eq(t_row('dh_student_course_input', 'sci_id', $sci1)['sci_10d'], 1, 'control: with only an older other course the numbers row is overwritten');
  t_ok(strpos(t_row('dh_student', 's_id', $s1)['s_photo'], 'private://final/photo-id/') === 0, 'control: and the photo is replaced');
});
t_done();
