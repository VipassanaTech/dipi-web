<?php
// Group labels with Dhamma Halls: with 2 halls the hall follows the group (name in headings, code in narrow spots);
// with no halls every label is as before. Pages are rendered by halls-page-runner.php (own process, rolled back).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-halls-labels.php
require __DIR__ . '/lib.php';
function hl_page($page, $course, $with_halls) {
  $in = array('page' => $page, 'course' => (int) $course,
    'halls' => $with_halls ? array(array('HA', 'Hall Alpha'), array('HB', 'Hall Beta')) : array(),
    'usual' => $with_halls ? array('2' => 'HB') : array());
  return (string) shell_exec('php7.4 ' . escapeshellarg(__DIR__ . '/halls-page-runner.php') . ' ' . escapeshellarg(json_encode($in)) . ' 2>/dev/null');
}

$k = (int) db_query("select a.a_course from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id join dh_course c on c.c_id = a.a_course
  where a.a_attended = 1 and aa.aa_group in (1, 2) and c.c_id < 9000000 and c.c_combined_seat_course = 0
  group by a.a_course having count(distinct aa.aa_group) = 2 order by a.a_course desc limit 1")->fetchField();
t_ok($k > 0, 'L0 a local course with students in groups 1 and 2');
$c = (int) db_query("select c_center from dh_course where c_id = :k", array(':k' => $k))->fetchField();

// No halls: as today.
t_ok(strpos(hl_page('checking-slip', $k, FALSE), '&ndash; Group 1</div>') !== FALSE, 'L1 no halls: checking slip header as before');
t_ok(strpos(hl_page('student-chit', $k, FALSE), '&ndash; Group 1</div>') !== FALSE, 'L1 no halls: student chit header as before');
t_ok(strpos(hl_page('teacher-list', $k, FALSE), '</span>Group 2<span class="sep">|</span>') !== FALSE, 'L1 no halls: teacher list block row as before');
t_ok(strpos(hl_page('group-seating', $k, FALSE), 'data-section-label="Group 1 - ') !== FALSE, 'L1 no halls: group seating section label as before');

// Two halls, group 2 usually in HB (Hall Beta), the others in HA (Hall Alpha).
t_ok(strpos(hl_page('checking-slip', $k, TRUE), '&ndash; Group 1 · Hall Alpha</div>') !== FALSE, 'L2 checking slip header: hall name');
t_ok(strpos(hl_page('student-chit', $k, TRUE), '&ndash; Group 1 · Hall Alpha</div>') !== FALSE, 'L2 student chit header: hall name');
$tl = hl_page('teacher-list', $k, TRUE);
t_ok(strpos($tl, '</span>Group 2 · Hall Beta<span class="sep">|</span>') !== FALSE, 'L2 teacher list block row: hall name');
t_ok(strpos($tl, 'Group 2 · HB (') !== FALSE, 'L2 teacher list "Print only" choice: hall code');
$gs = hl_page('group-seating', $k, TRUE);
t_ok(strpos($gs, 'GROUP 1 · Hall Alpha') !== FALSE, 'L2 group seating heading: hall name');
t_ok(strpos($gs, 'data-section-label="Group 1 · HA - ') !== FALSE, 'L2 group seating section label: hall code');
t_ok(strpos($gs, 'data-section-title="Group 1 · Hall Alpha - ') !== FALSE, 'L2 group seating section PDF title: hall name');

// The "Regenerate this section" label (in this process, rolled back).
t_in_txn(function () use ($k, $c) {
  db_update('dh_center_hall')->fields(array('ch_deleted' => 1))->condition('ch_center', $c)->execute();
  dh_halls_reset();
  t_eq(dh_seat_regen_parse('group', 'g2_M', $k)[2], 'Group 2 - Male', 'L3 no halls: section label as before');
  t_eq(dh_seat_regen_parse('group', 'g2_M')[2], 'Group 2 - Male', 'L3 no course given: as before');
  t_make_hall($c, 'HA', 'Hall Alpha', 0);
  $hb = t_make_hall($c, 'HB', 'Hall Beta', 1);
  db_merge('dh_center_hall_group')->key(array('chg_center' => $c, 'chg_group' => 2))->fields(array('chg_hall' => $hb))->execute();
  db_delete('dh_course_hall_group')->condition('cg_course', $k)->execute();
  dh_halls_reset();
  t_eq(dh_seat_regen_parse('group', 'g2_M', $k)[2], 'Group 2 · HB - Male', 'L4 section label: hall code');
  t_eq(dh_seat_regen_parse('group', 'gUnassigned_F', $k)[2], 'Group Unassigned - Female', 'L4 students with no group: no hall');
  t_eq(dh_seat_regen_parse('main', 'M', $k)[2], 'Male', 'L4 main plan: unchanged');
});
dh_halls_reset();
t_done();
