<?php
// ============================================================================
// Announcement for centres (22 Nov 2026): the Rooms page and the Room Board are live; each centre's old
// room list was copied to the Rooms page. dh_announcement, audience centre, posted as uid 1 like the
// other announcements. Text approved by the owner on 22 Nov 2026 ("yes post them on prod").
//
// Run from the Drupal root, on the command line only:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-22-announce-rooms.php
// Re-runnable: an announcement with the same audience and title is not posted twice; its text is updated
// if it changed (editing keeps the posted date, as in the form).
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$items = array(
  array(
    'an_audience' => 'centre',
    'an_title' => 'New: Rooms page and Room Board',
    'an_body' => '<p>DIPI now has a <b>Rooms</b> page for each centre. We have copied your old room list there for you: same sections, same rooms. You can keep giving rooms in <b>Zero Day</b> as before. Nothing else has to change.</p>'
      . '<p>What is new:</p><ul>'
      . '<li><b>Rooms page</b> &mdash; open <b>Centre Settings</b>, then <b>Accommodation Settings</b>, then <b>Open the Rooms page</b>. Here you can see your rooms section by section, add or change rooms, set the toilet type, and give a section a full name (for example &quot;M &mdash; Old Students Block&quot;). Use <b>Print&hellip; &rarr; Rooms only</b> for a blank room chart.</li>'
      . '<li><b>Room Board</b> &mdash; open a course and click <b>Room Board</b>. You see all rooms and can place students in them. <b>Print</b> gives a room chart with each student&#39;s confirmation number and name.</li>'
      . '<li><b>Zero Day</b> &mdash; the <b>Section</b> and <b>Room</b> boxes can now be searched, and they show how many beds are free.</li>'
      . '</ul>'
      . '<p>The toilet type of every room starts as &quot;Not set&quot;. You can set it any time; nothing stops working while it is &quot;Not set&quot;.</p>'
      . '<p>Help: the <a href="/wiki-rooms"><b>Rooms</b></a> page in the DIPI wiki.</p>',
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
