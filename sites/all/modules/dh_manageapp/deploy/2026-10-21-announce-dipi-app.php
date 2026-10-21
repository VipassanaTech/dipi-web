<?php
// ============================================================================
// Announcements for the Dipi App download page: one for centres, one for the AT Portal
// (dh_announcement, inc/announcements.inc). Posted as uid 1, like announcement 36.
//
// Run from the Drupal root, on the command line only:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-21-announce-dipi-app.php
// Re-runnable: an announcement with the same audience and title is not posted twice; its
// text is updated if it changed (editing keeps the posted date, as in the form).
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$items = array(
  array(
    'an_audience' => 'centre',
    'an_title' => 'New: Dipi App for Android tablets',
    'an_body' => '<p>The <b>Dipi App</b> shows the seating plan and the students of a running course on an Android tablet or phone, without internet. Teachers can use it in the Dhamma hall. On the centre page, click <a href="/dipi-app"><b>Dipi App (Android)</b></a> to download it. <a href="/wiki-dipi-app">Read how it works</a>.</p>',
  ),
  array(
    'an_audience' => 'at',
    'an_title' => 'New: Dipi App for Android',
    'an_body' => '<p>The <b>Dipi App</b> shows the seating plan and the students of a course you serve, on an Android tablet or phone, without internet. The centre may give you a tablet with the course loaded, or you can install it yourself: on the AT Portal dashboard, click <a href="/dipi-app"><b>Dipi App (Android)</b></a>. <a href="/at-wiki-dipi-app">Read how it works</a>.</p>',
  ),
);

foreach ($items as $it) {
  $id = db_query("select an_id from dh_announcement where an_audience = :a and an_title = :t and an_deleted = 0",
    array(':a' => $it['an_audience'], ':t' => $it['an_title']))->fetchField();
  if ($id) {
    $n = db_update('dh_announcement')->fields(array('an_body' => $it['an_body'], 'an_updated' => date('Y-m-d H:i:s'), 'an_updated_by' => 1))
      ->condition('an_id', $id)->condition('an_body', $it['an_body'], '<>')->execute();
    echo "[{$it['an_audience']}] already posted (an_id $id)" . ($n ? ", text updated" : "") . "\n";
    continue;
  }
  $id = db_insert('dh_announcement')->fields($it + array('an_posted' => date('Y-m-d H:i:s'), 'an_created_by' => 1))->execute();
  echo "[{$it['an_audience']}] posted an_id $id\n";
}
