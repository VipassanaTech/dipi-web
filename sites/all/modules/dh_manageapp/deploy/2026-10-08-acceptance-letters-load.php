<?php
// ============================================================================
// Load the teacher acceptance letters (ATAL / CTAL, English + Hindi) into dh_acceptance_doc.
// Run from the Drupal root, on the command line only, after 2026-10-08-teacher-acceptance.sql:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-08-acceptance-letters-load.php <folder> dry   (shows what it would do)
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-08-acceptance-letters-load.php <folder> run   (writes)
// <folder> holds letters.json (doc, version, lang, file, title, ack) and the HTML files it names.
// The letters are kept out of git because this repo is public.
// Re-runnable: a row with the same letter, version and language is updated in place. Loading
// a new version of a letter switches off its older versions (so teachers accept again).
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);

$dir = isset($argv[1]) ? rtrim($argv[1], '/') : '';
$mode = isset($argv[2]) ? $argv[2] : 'dry';
if ($dir === '' || !is_file("$dir/letters.json") || !in_array($mode, array('dry', 'run'))) { echo "Usage: php ... <folder with letters.json> dry|run\n"; exit(1); }
if (!db_table_exists('dh_acceptance_doc')) { echo "dh_acceptance_doc does not exist - run 2026-10-08-teacher-acceptance.sql first.\n"; exit(1); }
$letters = json_decode(file_get_contents("$dir/letters.json"), TRUE);
if (!is_array($letters)) { echo "letters.json is not valid JSON.\n"; exit(1); }

$ok = TRUE;
foreach ($letters as $i => $l) {
  foreach (array('doc', 'version', 'lang', 'file', 'title', 'ack') as $k)
    if (empty($l[$k])) { echo "entry $i: missing $k\n"; $ok = FALSE; }
  if (!empty($l['doc']) && !in_array($l['doc'], array('ATAL', 'CTAL'))) { echo "entry $i: doc must be ATAL or CTAL\n"; $ok = FALSE; }
  if (!empty($l['lang']) && !in_array($l['lang'], array('en', 'hi'))) { echo "entry $i: lang must be en or hi\n"; $ok = FALSE; }
  if (!empty($l['file']) && !is_file("$dir/{$l['file']}")) { echo "entry $i: file {$l['file']} not found\n"; $ok = FALSE; }
}
if (!$ok) exit(1);

foreach ($letters as $l) {
  $body = trim(file_get_contents("$dir/{$l['file']}"));
  $old = db_query("select ad_id, md5(ad_body) b, ad_title, ad_ack, ad_active from dh_acceptance_doc where ad_doc = :d and ad_version = :v and ad_lang = :l",
    array(':d' => $l['doc'], ':v' => $l['version'], ':l' => $l['lang']))->fetchObject();
  $what = !$old ? 'new' : (($old->b === md5($body) && $old->ad_title === $l['title'] && $old->ad_ack === $l['ack'] && $old->ad_active) ? 'unchanged' : 'update');
  $others = db_query("select count(*) from dh_acceptance_doc where ad_doc = :d and ad_version <> :v and ad_active = 1", array(':d' => $l['doc'], ':v' => $l['version']))->fetchField();
  echo "{$l['doc']} {$l['version']} {$l['lang']}: $what (" . mb_strlen(strip_tags($body)) . " characters of text)" . ($others ? ", switches off $others row(s) of other versions" : '') . "\n";
  if ($mode == 'run') {
    db_merge('dh_acceptance_doc')
      ->key(array('ad_doc' => $l['doc'], 'ad_version' => $l['version'], 'ad_lang' => $l['lang']))
      ->fields(array('ad_title' => $l['title'], 'ad_body' => $body, 'ad_ack' => $l['ack'], 'ad_active' => 1, 'ad_updated' => date('Y-m-d H:i:s')))
      ->execute();
    db_query("update dh_acceptance_doc set ad_active = 0 where ad_doc = :d and ad_version <> :v", array(':d' => $l['doc'], ':v' => $l['version']));
  }
}
echo $mode == 'run' ? "done\n" : "dry run - nothing written\n";
