<?php
// ============================================================================
// Announcements for Dipi App 1.1.9 (24 Nov 2026): ask centres and teachers to update the app:
// the hall is shown next to the group (centres with more than one Dhamma hall), and long names keep two lines. One for
// centres, one for the AT Portal (dh_announcement, inc/announcements.inc). Posted as uid 1,
// like the 2026-10-21 Dipi App announcements. Text approved by the owner on 24 Nov 2026.
//
// Run from the Drupal root, on the command line only:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-24-announce-dipi-app-1-1-9.php
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
    'an_title' => 'Dipi App 1.1.9: please update tablets and phones',
    'an_body' => '<p>A new version of the <b>Dipi App (1.1.9)</b> is ready. At centres with more than one Dhamma hall, the app now shows the hall next to the group, for example <i>Group 2 · Dhamma Hall 3 · Male</i>, in the plan list, the servers lists and the student page. Long names now keep two lines and the age, even with a bigger font, and are shortened with initials, like the Dipi seating plan. Please update the app on every tablet and phone: when the app says <b>A new Dipi App is ready</b>, tap <b>Download</b>, or on the centre page click <a href="/dipi-app"><b>Dipi App (Android)</b></a>, then download and install it. After that, download your course again in the app.</p>',
  ),
  array(
    'an_audience' => 'at',
    'an_title' => 'Dipi App 1.1.9: please update',
    'an_body' => '<p>A new version of the <b>Dipi App (1.1.9)</b> is ready. At centres with more than one Dhamma hall, the app now shows the hall next to the group, for example <i>Group 2 · Dhamma Hall 3 · Male</i>. Long names now keep two lines and the age, even with a bigger font. If you use the app on your own tablet or phone, please update it: when the app says <b>A new Dipi App is ready</b>, tap <b>Download</b>, or on the AT Portal dashboard click <a href="/dipi-app"><b>Dipi App (Android)</b></a>, then download and install it. After that, download the course again.</p>',
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
