<?php
// ============================================================================
// Help for teachers on referrals: a "Referrals" page in the AT Portal wiki (book 46, alias
// at-wiki-referral), linked from the book's root contents list. Covers the new "Add Referral"
// action on My Courses -> Applicants (details filled in), typing a referral by hand, the
// Referral Information fields (at least one "Listed for" box) and My Referral List.
//
// Run from the Drupal root, on the command line only (php7.3 on prod, php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-25-wiki-at-referral.php
// Re-runnable: updates the page if it exists (matched by title in the book), and sets the
// alias and the root contents link once.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$body = <<<'HTML'
<p>You can add a student to the Referral List. When this student applies for a course type you ticked, the centre sees your referral on the application, with your reason and dates.</p>

<h3>Add a referral from your course (details filled in)</h3>
<ol>
<li>On the AT Portal, click <b>My Courses — View Applicants (read-only)</b>.</li>
<li>Click the course, then click <b>Applicants</b>.</li>
<li>Find the student. In the <b>Action</b> column, choose <b>Add Referral</b>.</li>
<li>The form opens with the student's details already filled in: name, phone, email, address, ID numbers, courses done before, and this course's centre, type and start date. Check them and correct anything that is wrong.</li>
<li>Fill in the <b>Referral Information</b> (see below).</li>
<li>Click <b>Add Referral</b>. You come back to the list of applicants.</li>
</ol>
<p>You see <b>Add Referral</b> only for the students you can see in My Courses: your side (Male, Female or Both), and from Day 1 only the students who attended or left.</p>

<h3>Add a referral by typing the details</h3>
<p>On the AT Portal, click <b>Add Referral</b> and type the student's details yourself.</p>

<h3>Referral Information</h3>
<ul>
<li><b>Listed for</b>: tick at least one course type (10-Day, STP, 20-Day … or <b>All</b>). The referral shows only on applications for the course types you tick.</li>
<li><b>List Type</b>: choose <b>Referral List</b> or <b>Special List</b>.</li>
<li><b>Start Date</b> and <b>End Date</b>: the End Date can be at most 2 years from today.</li>
<li><b>Referral Reason/Remarks</b>: write why you are adding this student.</li>
<li><b>ID numbers</b> (Aadhar, Pancard, National ID, Passport): type the real number, or leave it empty. Do not type "No", "-" or "NA".</li>
</ul>

<h3>Your referrals</h3>
<p>On the AT Portal, click <b>My Referral List</b>:</p>
<ul>
<li><b>Update</b>: change the End Date and add a comment.</li>
<li><b>Delete</b>: remove the referral.</li>
</ul>
<p><b>Referral List</b> shows all the referrals.</p>
HTML;

$bid = 46;
$title = 'Referrals';
$alias = 'at-wiki-referral';

$root_mlid = db_query("select mlid from book where bid = :b and nid = :b", array(':b' => $bid))->fetchField();
if (!$root_mlid) { echo "ERROR: book root (bid=$bid) not found on this site\n"; exit(1); }
$nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = :b and n.title = :t", array(':b' => $bid, ':t' => $title))->fetchField();
if ($nid) {
  $node = node_load($nid);
  echo "[$alias] updating existing page nid=$nid\n";
}
else {
  $node = new stdClass();
  $node->type = 'book';
  node_object_prepare($node);
  $node->uid = 1; $node->status = 1; $node->promote = 0; $node->comment = 0;
  $node->language = LANGUAGE_NONE;
  $w = (int) db_query("select max(weight) from menu_links where plid = :p", array(':p' => $root_mlid))->fetchField();
  $node->book = array('bid' => $bid, 'plid' => $root_mlid, 'menu_name' => 'book-toc-' . $bid, 'weight' => $w + 1, 'module' => 'book');
  echo "[$alias] creating new page under root mlid=$root_mlid weight=" . ($w + 1) . "\n";
}
$node->title = $title;
$node->body[LANGUAGE_NONE][0] = array('value' => $body, 'format' => 'full_html');
node_save($node);
echo "[$alias] saved nid={$node->nid}\n";

$src = 'node/' . $node->nid;
if (!db_query("select 1 from url_alias where source = :s and alias = :a", array(':s' => $src, ':a' => $alias))->fetchField()) {
  $p = array('source' => $src, 'alias' => $alias);
  path_save($p);
  echo "[$alias] alias set: /$alias -> $src\n";
}

$root = node_load($bid);
$rb = $root->body[LANGUAGE_NONE][0]['value'];
if (strpos($rb, '/' . $alias . '"') !== FALSE) {
  echo "[$alias] root contents already link the page\n";
}
elseif (strpos($rb, '</ul>') === FALSE) {
  echo "[$alias] WARN: no </ul> in the root body; add the link by hand\n";
}
else {
  $root->body[LANGUAGE_NONE][0]['value'] = preg_replace('#</ul>#', '<li><a href="/' . $alias . '">' . $title . "</a></li>\n</ul>", $rb, 1);
  $root->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  node_save($root);
  echo "[$alias] root contents updated\n";
}

drupal_flush_all_caches();
echo "done. Check /$alias\n";
