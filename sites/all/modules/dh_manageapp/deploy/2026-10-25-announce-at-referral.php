<?php
// ============================================================================
// AT Portal announcement for the "Add Referral" action on My Courses -> Applicants
// (dh_announcement, inc/announcements.inc). Posted as uid 1, like announcements 36-38.
//
// Run from the Drupal root, on the command line only:
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-25-announce-at-referral.php
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
    'an_audience' => 'at',
    'an_title' => 'Add a referral from your course',
    'an_body' => '<p>You can now add a student to the Referral List straight from your course:</p>'
      . '<ol><li>Click <a href="/at-portal/my-courses"><b>My Courses — View Applicants (read-only)</b></a>, click the course, then click <b>Applicants</b>.</li>'
      . '<li>In the <b>Action</b> column, choose <b>Add Referral</b>.</li></ol>'
      . '<p>The form opens with the student\'s details already filled in. Tick at least one course type under <b>Listed for</b>, then click <b>Add Referral</b>.</p>'
      . '<p>More help: <a href="/at-wiki-referral">AT Portal Wiki &rarr; Referrals</a>.</p>',
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
