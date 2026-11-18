<?php
// ============================================================================
// DIPI WIKI (book 10) lines for the referral changes of 18 Nov 2026:
//   - "Central Referral List" (/wiki-referral): the list shows active referrals only, and History;
//   - "Center Referral List" (/wiki-center-referral): History.
// Each page gets a new revision; the rest of its text is left as it is.
//
// Run from the Drupal root, on the command line only (php7.3 on prod, php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-18-wiki-referral-history.php
// Re-runnable: a page that already has the History line is left alone.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$marker = 'every change to that referral';

$central = "  <li>The Referral List shows only <b>active</b> referrals (End Date today or later). An AT sees their expired and deleted referrals in the AT Portal, under <b>My Referral List</b>.</li>\n"
  . "  <li><b>History</b>: click the <b>+</b> at the start of a row to see every change to that referral: what changed, who changed it and when. You see the History only for referrals you can edit.</li>\n";

$center = "\n<p><b>History</b>: in the Center Referral List, click the <b>+</b> at the start of a row to see every change to that referral: what changed, who changed it and when. You see the History only for referrals you can edit.</p>\n";

$pages = array(
  'wiki-referral' => array('title' => 'Central Referral List', 'how' => 'before-last-ol', 'text' => $central),
  'wiki-center-referral' => array('title' => 'Center Referral List', 'how' => 'append', 'text' => $center),
);

foreach ($pages as $alias => $p) {
  $nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = 10 and n.title = :t", array(':t' => $p['title']))->fetchField();
  if (!$nid) { echo "[$alias] ERROR: page '{$p['title']}' not found in book 10\n"; continue; }
  $node = node_load($nid);
  $body = $node->body[LANGUAGE_NONE][0]['value'];
  if (strpos($body, $marker) !== FALSE) { echo "[$alias] nid $nid already has the History line\n"; continue; }
  if ($p['how'] === 'before-last-ol') {
    $i = strrpos($body, '</ol>');
    if ($i === FALSE) { echo "[$alias] WARN: no </ol> in nid $nid; nothing changed\n"; continue; }
    $body = substr($body, 0, $i) . $p['text'] . substr($body, $i);
  }
  else
    $body = rtrim($body) . "\n" . $p['text'];
  $node->body[LANGUAGE_NONE][0]['value'] = $body;
  $node->revision = 1;
  $node->log = 'Referrals: History for editors; main list shows active referrals only (18 Nov 2026)';
  node_save($node);
  echo "[$alias] nid $nid updated (new revision)\n";
}

// The centre tabs (deployed 12 Nov), placed just above the History paragraph of the Center Referral List page.
$tabs_marker = 'Active, Inactive and Deleted tabs';
$tabs = "<p><b>Active, Inactive and Deleted tabs</b>: the Center Referral List has three tabs.</p>\n"
  . "<ul>\n"
  . "<li><b>Active</b>: End Date today or later. These show on matching applications.</li>\n"
  . "<li><b>Inactive (end date passed)</b>: to make one active again, open it and set a later End Date.</li>\n"
  . "<li><b>Deleted</b>: click <b>Undelete</b> to bring a referral back. It returns to Active or Inactive by its End Date.</li>\n"
  . "</ul>\n";
$nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = 10 and n.title = 'Center Referral List'")->fetchField();
if ($nid) {
  $node = node_load($nid);
  $body = $node->body[LANGUAGE_NONE][0]['value'];
  $at = strpos($body, '<p><b>History</b>: in the Center Referral List');
  if (strpos($body, $tabs_marker) !== FALSE)
    echo "[wiki-center-referral] nid $nid already has the tabs section\n";
  elseif ($at === FALSE)
    echo "[wiki-center-referral] WARN: History paragraph not found in nid $nid; tabs section not added\n";
  else {
    $node->body[LANGUAGE_NONE][0]['value'] = substr($body, 0, $at) . $tabs . substr($body, $at);
    $node->revision = 1;
    $node->log = 'Center referrals: Active / Inactive / Deleted tabs and Undelete (18 Nov 2026)';
    node_save($node);
    echo "[wiki-center-referral] nid $nid tabs section added (new revision)\n";
  }
}

drupal_flush_all_caches();
echo "done. Check /wiki-referral and /wiki-center-referral\n";
