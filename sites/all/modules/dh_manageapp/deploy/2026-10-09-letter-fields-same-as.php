<?php
// ============================================================================
// Add missing course-type letter fields, as the user decided (2026-10-09/10):
//  - copy the same centre's field for another course type where the two are the same
//    (Teenager, 10 Day Executive and 10 Day Special end at the same time as the 10 Day
//    course; STP-9, Teenager and 10 Day Executive have the same notice period as 10 Day),
//  - or a fixed value (STP-9 ends at 7:00 a.m., an STP Course at 5:00 p.m.).
//
// Merge fields are filled recursively: [Time-[CourseType]] becomes [Time-Teenager Course]
// for a Teenager course, which is then the centre's letter field "Time-Teenager Course".
// When that field is missing, students get the brackets. For every centre (and the
// new-centre template, centre 0) whose letters or letter fields use a combined field
// listed in $same_as, the missing field is added with the value of the same centre's
// field for the source type, or the fixed value. Nothing is changed where the field
// exists (a centre's own value is kept), or where the centre has no field for the source
// type either.
//
// Run from the Drupal root on the command line (php7.3 on prod). Shows what it would do;
// writes only with --apply:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-09-letter-fields-same-as.php [--apply]
// Re-runnable. Prints the ids of the rows it adds (dh_letter_fields.lf_id), so they can be
// removed again if needed.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
$apply = in_array('--apply', $argv);

// Which combined fields, and what each missing course type gets: the name of the course
// type to copy from (the same centre's field), or array('value' => fixed text). Only the
// template's own combined fields [Time-[CourseType]] and [Day-[CourseType]]; variants a
// centre made itself (TimeHindi-, EndTime-, ...) are left alone (user, 2026-10-10).
$same_as = array(
  // [Time-[CourseType]]: the time the course ends
  '/^Time-$/' => array(
    'Teenager Course'         => '10 Day Course',
    '10 Day Executive Course' => '10 Day Course',
    '10 Day Special Course'   => '10 Day Course',
    'STP-9'                   => array('value' => '7:00 a.m.'),
    'STP Course'              => array('value' => '5:00 p.m.'),
  ),
  // [Day-[CourseType]]: how long before the start the applicant is told the status
  '/^Day-$/' => array(
    'STP-9'                   => '10 Day Course',
    'Teenager Course'         => '10 Day Course',
    '10 Day Executive Course' => '10 Day Course',
  ),
);
$part = '(?:[^\[\]<]|<[^>\[\]]*>)';
// combined [pre[CourseType]suf] fields each centre uses, in its active letters and its letter fields
$uses = array();
$texts = db_query("select l_center c, concat(l_subject, '\n', l_body, '\n', l_sms) t from dh_letter where l_deleted = 0
  union all select lf_center, lf_value from dh_letter_fields where lf_deleted = 0");
foreach ($texts as $r)
  if (preg_match_all('/\[(' . $part . '*?)\[CourseType\](' . $part . '*?)\]/u', $r->t, $mm, PREG_SET_ORDER))
    foreach ($mm as $m)
      $uses[$r->c][strip_tags($m[1]) . '|' . strip_tags($m[2])] = 1;

$fields = array();
foreach (db_query("select lf_center, lf_name, lf_value from dh_letter_fields where lf_deleted = 0 order by lf_id") as $f)
  $fields[$f->lf_center][$f->lf_name] = $f->lf_value; // the last one wins, as when sending

$added = array(); $centres = array(); $no_source = 0; $by_name = array();
ksort($uses);
foreach ($uses as $c => $patterns) {
  foreach (array_keys($patterns) as $k) {
    list($pre, $suf) = explode('|', $k, 2);
    foreach ($same_as as $re => $map) {
      if (!preg_match($re, $pre))
        continue;
      foreach ($map as $target => $source) {
        $tname = $pre . $target . $suf;
        if (isset($fields[$c][$tname]))
          continue;
        if (is_array($source))
          $value = $source['value'];
        else {
          $sname = $pre . $source . $suf;
          if (!isset($fields[$c][$sname])) { $no_source++; continue; }
          $value = $fields[$c][$sname];
        }
        $fields[$c][$tname] = $value; // also stops a second pattern adding it twice
        $centres[$c] = 1;
        $by_name[$tname] = (isset($by_name[$tname]) ? $by_name[$tname] : 0) + 1;
        if ($apply)
          $added[] = db_insert('dh_letter_fields')->fields(array('lf_center' => $c, 'lf_name' => $tname, 'lf_value' => $value, 'lf_deleted' => 0))->execute();
        else
          $added[] = "$c:$tname";
      }
    }
  }
}
printf("%s %d letter fields in %d centres%s\n", $apply ? 'added' : 'would add', count($added), count($centres), isset($centres[0]) ? ' (including the new-centre template)' : '');
foreach ($by_name as $n => $cnt)
  printf("  %4d x [%s]\n", $cnt, $n);
if ($no_source)
  echo "not added, the centre has no field for the source course type either: $no_source\n";
if ($apply && $added)
  echo "new lf_id: " . implode(',', $added) . "\n";
echo $apply ? "done\n" : "preview only: run with --apply to add them\n";
