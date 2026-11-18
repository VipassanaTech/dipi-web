<?php
// ============================================================================
// AT Portal help for the referral changes of 18 Nov 2026 (full edit, Active / Inactive /
// Deleted tabs with Undelete, History):
//   1. the "Your referrals" section of the AT Portal wiki page "Referrals" (book 46,
//      /at-wiki-referral) is replaced; the rest of the page is left as it is;
//   2. an AT announcement (dh_announcement, audience 'at'), posted as uid 1 like the
//      25 Oct one.
//
// Run from the Drupal root, on the command line only (php7.3 on prod, php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-18-at-referral-edit-tabs.php
// Re-runnable: the wiki section is replaced only when it still holds the 25 Oct text (or
// already holds this text: nothing to do); the announcement is posted once (same audience
// and title), its text updated if it changed.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// ---- 1. Wiki: the "Your referrals" section -------------------------------------------
$old_section = <<<'HTML'
<h3>Your referrals</h3>
<p>On the AT Portal, click <b>My Referral List</b>:</p>
<ul>
<li><b>Update</b>: change the End Date and add a comment.</li>
<li><b>Delete</b>: remove the referral.</li>
</ul>
<p><b>Referral List</b> shows all the referrals.</p>
HTML;

$new_section = <<<'HTML'
<h3>Your referrals</h3>
<p>On the AT Portal, click <b>My Referral List</b>. Your referrals are in three tabs:</p>
<ul>
<li><b>Active</b>: the End Date is today or later. The centre sees these referrals on the student's application.</li>
<li><b>Inactive (end date passed)</b>: the End Date has passed. These are deleted automatically 100 days after the email that tells you the End Date is near. To keep one, open it and set a later End Date.</li>
<li><b>Deleted</b>: referrals that were deleted. Click <b>Undelete</b> to bring one back.</li>
</ul>
<p>On the Active and Inactive tabs:</p>
<ul>
<li><b>Click the student's name</b> to open the full form. Change any detail, then click <b>Update</b>. The End Date can be at most 2 years from today: if an older referral has a later End Date, set a nearer date before you save.</li>
<li><b>Update</b>: change only the End Date and add a comment.</li>
<li><b>Delete</b>: move the referral to the Deleted tab. A referral marked <b>r-only</b> by the referral admin cannot be deleted.</li>
<li>Click the <b>+</b> at the start of a row to see all the details and the <b>History</b>: every change, who made it and when.</li>
</ul>
<p><b>Referral List</b> shows all the active referrals.</p>
HTML;

$nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = 46 and n.title = 'Referrals'")->fetchField();
if (!$nid) {
  echo "[wiki] ERROR: the AT Portal wiki page 'Referrals' (book 46) was not found\n";
}
else {
  $node = node_load($nid);
  $body = $node->body[LANGUAGE_NONE][0]['value'];
  if (strpos($body, $new_section) !== FALSE) {
    echo "[wiki] nid $nid already has the new section\n";
  }
  elseif (substr_count($body, $old_section) !== 1) {
    echo "[wiki] WARN: nid $nid does not hold the 25 Oct section exactly once (edited since?); nothing changed\n";
  }
  else {
    $node->body[LANGUAGE_NONE][0]['value'] = str_replace($old_section, $new_section, $body);
    $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
    $node->revision = 1;
    $node->log = 'Referrals: full edit, Active / Inactive / Deleted tabs, Undelete, History (18 Nov 2026)';
    node_save($node);
    echo "[wiki] nid $nid section replaced (new revision)\n";
  }
}

// ---- 2. Announcement for ATs ----------------------------------------------------------
$items = array(
  array(
    'an_audience' => 'at',
    'an_title' => 'Edit your referrals, and see deleted ones',
    'an_body' => '<p>On <a href="/at-portal/my-referral"><b>My Referral List</b></a> you can now:</p>'
      . '<ul><li><b>Edit a referral in full</b>: click the student\'s name, change any detail, then click <b>Update</b>.</li>'
      . '<li>See your referrals in three tabs: <b>Active</b>, <b>Inactive (end date passed)</b> and <b>Deleted</b>.</li>'
      . '<li><b>Undelete</b> a referral from the Deleted tab.</li>'
      . '<li>See the <b>History</b> of a referral: click the <b>+</b> at the start of its row.</li></ul>'
      . '<p>The End Date can be at most 2 years from today. If an older referral has a later End Date, set a nearer date before you save.</p>'
      . '<p>More help: <a href="/at-wiki-referral">AT Portal Wiki &rarr; Referrals</a>.</p>',
  ),
);
foreach ($items as $it) {
  $id = db_query("select an_id from dh_announcement where an_audience = :a and an_title = :t and an_deleted = 0",
    array(':a' => $it['an_audience'], ':t' => $it['an_title']))->fetchField();
  if ($id) {
    $n = db_update('dh_announcement')->fields(array('an_body' => $it['an_body'], 'an_updated' => date('Y-m-d H:i:s'), 'an_updated_by' => 1))
      ->condition('an_id', $id)->condition('an_body', $it['an_body'], '<>')->execute();
    echo "[announce {$it['an_audience']}] already posted (an_id $id)" . ($n ? ", text updated" : "") . "\n";
    continue;
  }
  $id = db_insert('dh_announcement')->fields($it + array('an_posted' => date('Y-m-d H:i:s'), 'an_created_by' => 1))->execute();
  echo "[announce {$it['an_audience']}] posted an_id $id\n";
}

drupal_flush_all_caches();
echo "done. Check /at-wiki-referral and /at-portal/announcements\n";
