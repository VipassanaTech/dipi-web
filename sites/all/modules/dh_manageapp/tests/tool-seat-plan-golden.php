<?php
// Golden copy of seating plans, for a before / after compare (seat label start, 22 Nov 2026).
// Renders the main and the group plan of each course the way the plan pages do, inside a
// transaction that is rolled back (drawing a plan saves labels). Writes, per course:
//   <out>/<course>-main.html, <out>/<course>-group.html  (the HTML with <script> blocks removed)
//   <out>/<course>-labels.json                             (a_id => [aa_seat, aa_group_seat] after drawing)
// Local only. Usage, from the Drupal root:
//   php7.4 sites/all/modules/dh_manageapp/tests/tool-seat-plan-golden.php <out dir> <course id> [<course id> ...]
require __DIR__ . '/lib.php';
$out = isset($argv[1]) ? $argv[1] : '';
$ids = array_slice($argv, 2);
if ($out === '' || !$ids) { echo "Usage: tool-seat-plan-golden.php <out dir> <course id> ...\n"; t_done(); }
if (!is_dir($out)) mkdir($out, 0700, TRUE);
function gold_strip($html) { return preg_replace('#<script\b.*?</script>#s', '', $html); }
foreach ($ids as $id) {
  $id = (int) $id;
  $txn = db_transaction();
  $centre = (int) db_query("select c_center from dh_course where c_id = :c", array(':c' => $id))->fetchField();
  $other = (int) db_query("select c_combined_seat_course from dh_course where c_id = :c", array(':c' => $id))->fetchField();
  $course = $other > 0 ? array($id, $other) : array($id);
  $_GET['q'] = "seating/$centre/$id";
  file_put_contents("$out/$id-main.html", gold_strip(dh_generate_seating_plan($centre, $course, '', FALSE, TRUE)));
  $_GET['q'] = "group-seating/$centre/$id";
  dh_auto_allocate_group_seats($centre, $course, FALSE);
  file_put_contents("$out/$id-group.html", gold_strip(dh_generate_group_seating_plan($centre, $course, '', TRUE)));
  $labels = array();
  foreach (db_query("select a.a_id, aa.aa_seat, aa.aa_group_seat from dh_applicant a join dh_applicant_attended aa on aa.aa_applicant = a.a_id
    where a.a_course in (:c) and a.a_attended = 1 order by a.a_id", array(':c' => $course)) as $r)
    $labels[$r->a_id] = array($r->aa_seat, $r->aa_group_seat);
  file_put_contents("$out/$id-labels.json", json_encode($labels, JSON_PRETTY_PRINT));
  $txn->rollback();
  echo "course $id: written\n";
}
t_ok(TRUE, 'golden files written to ' . $out);
t_done();
