<?php
// Golden copy of the pages that show groups (Dhamma Halls, 23 Nov 2026): each page is rendered by
// halls-page-runner.php with NO halls at the centre, so the copy made before the label changes and the copy made
// after them must be byte-identical (the one-hall rule). Local only. Usage, from the Drupal root:
//   php7.4 sites/all/modules/dh_manageapp/tests/tool-halls-golden.php <out dir> <course id> [<course id> ...]
// The files hold student names: the folder is private (0700); delete it after the compare.
if (php_sapi_name() !== 'cli') { exit(1); }
$out = isset($argv[1]) ? $argv[1] : '';
$ids = array_slice($argv, 2);
if ($out === '' || !$ids) { echo "Usage: tool-halls-golden.php <out dir> <course id> ...\n"; exit(1); }
if (!is_dir($out)) { mkdir($out, 0700, TRUE); }
$runner = __DIR__ . '/halls-page-runner.php';
foreach ($ids as $id) {
  foreach (array('teacher-list', 'student-chit', 'checking-slip', 'group-seating') as $page) {
    $in = json_encode(array('page' => $page, 'course' => (int) $id, 'halls' => array()));
    $html = (string) shell_exec('php7.4 ' . escapeshellarg($runner) . ' ' . escapeshellarg($in) . ' 2>/dev/null');
    // Leave out what changes on every run: form build ids and times of day.
    $html = preg_replace(array('#form-[A-Za-z0-9_-]{20,}#', '#\b\d{1,2}:\d{2}(:\d{2})?\b#'), '', $html);
    file_put_contents("$out/$id-$page.html", $html);
    echo "  course $id $page: " . strlen($html) . " bytes\n";
  }
}
