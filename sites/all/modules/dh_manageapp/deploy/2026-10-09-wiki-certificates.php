<?php
// ============================================================================
// Help for course certificates: the "Course Certificates" wiki page (Drupal book, bid 10)
// and a centre announcement that links to it.
//
// Run from the Drupal root, on the command line only (php7.3 on prod, which has DOMDocument;
// php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-09-wiki-certificates.php
// Re-runnable: updates the page if it exists (matched by title in book 10), sets the alias and
// the root-TOC link once, and posts the announcement once (later runs update its text only,
// so the date it was posted stays the same).
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$title = 'Course Certificates';
$alias = 'wiki-certificates';
$body = <<<'HTML'
<p>DIPI can print a <b>course certificate</b> for a student or a Dhamma server who attended a course. Each certificate gets its own number, for example <code>CAKKA/2026/0001</code>.</p>

<p><b>Who can get a certificate</b></p>
<ul>
<li>A student or a server who was marked <b>attended</b> on Day 0.</li>
<li>From <b>Day 1</b> of the course (the day after the start date) onwards.</li>
<li>For all courses except <b>Work Period</b>, <b>AT Workshop / Meeting</b> and <b>Other</b>.</li>
</ul>
<p>Students who left the course do not get a certificate.</p>

<p><b>1. Check your centre's certificate text (only once)</b></p>
<ol>
<li>On your centre page, click <b>Certificate Settings</b>.</li>
<li>Check the heading, the centre description, the address and contact lines, and the number prefix. DIPI fills them from your centre details, so please correct anything that is wrong.</li>
<li>Click <b>Save</b>.</li>
<li>Click <b>Preview a sample certificate</b> to see how it looks.</li>
</ol>

<p><b>2. Print a certificate</b></p>
<ol>
<li>Open the applicant list of the course (or use <b>Search</b>).</li>
<li>Find the student. In the <b>Action</b> column, choose <b>Certificate</b>.</li>
<li>The certificate opens in a new tab. Print it, sign it on the <b>Authorised Signatory</b> line, and give it to the student.</li>
</ol>
<p>If you do not see <b>Certificate</b> in the list, the person did not attend, the course has not reached Day 1, or this type of course does not give certificates.</p>

<p><b>3. Printing again</b></p>
<p>If you choose <b>Certificate</b> again for the same person, you get the <b>same number</b> and the <b>same date of issue</b>. It does not make a new certificate.</p>

<p><b>4. Finding a certificate by its number</b></p>
<p>If someone shows you a certificate and asks about it:</p>
<ol>
<li>Open <b>Search</b>.</li>
<li>Type the number (for example <code>CAKKA/2026/0001</code>) in the <b>Confirmation / Certificate No</b> box.</li>
<li>Click <b>Search</b>. DIPI shows the student it was given to.</li>
</ol>
<p>The student's activity also shows when the certificate was issued and each time it was printed.</p>

<p><b>Who can print certificates</b></p>
<p>Only some logins can print certificates (for example Centre Admin). If you do not see the option, ask your centre admin.</p>
HTML;

// ---- Wiki page -------------------------------------------------------------
$root_mlid = db_query("select mlid from book where bid = 10 and nid = 10")->fetchField();
if (!$root_mlid) { echo "ERROR: book root (bid=10, nid=10) not found on this site\n"; exit(1); }
$nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = 10 and n.title = :t", array(':t' => $title))->fetchField();
if ($nid) {
  $node = node_load($nid);
  echo "updating existing page nid=$nid\n";
}
else {
  $node = new stdClass();
  $node->type = 'book';
  node_object_prepare($node);
  $node->uid = 1; $node->status = 1; $node->promote = 0; $node->comment = 0;
  $node->language = LANGUAGE_NONE;
  $w = (int) db_query("select max(weight) from menu_links where plid = :p", array(':p' => $root_mlid))->fetchField();
  $node->book = array('bid' => 10, 'plid' => $root_mlid, 'menu_name' => 'book-toc-10', 'weight' => $w + 1, 'module' => 'book');
  echo "creating new page under root mlid=$root_mlid weight=" . ($w + 1) . "\n";
}
$node->title = $title;
$node->body[LANGUAGE_NONE][0] = array('value' => $body, 'format' => 'full_html');
node_save($node);
echo "saved nid={$node->nid}\n";

$src = 'node/' . $node->nid;
if (!db_query("select 1 from url_alias where source = :s and alias = :a", array(':s' => $src, ':a' => $alias))->fetchField()) {
  $p = array('source' => $src, 'alias' => $alias);
  path_save($p);
  echo "alias set: /$alias -> $src\n";
}
else {
  echo "alias already present\n";
}

// Root TOC (node 10 body): link via the alias, once, before </ol>.
$root = node_load(10);
$rb = $root->body[LANGUAGE_NONE][0]['value'];
if (strpos($rb, 'href="/' . $alias . '"') !== FALSE) {
  echo "root TOC already links the page\n";
}
elseif (strpos($rb, '</ol>') === FALSE) {
  echo "WARN: no </ol> in the root body; add the TOC link by hand\n";
}
else {
  $root->body[LANGUAGE_NONE][0]['value'] = preg_replace('#</ol>#', '<li><a href="/' . $alias . '">' . $title . '</a></li></ol>', $rb, 1);
  $root->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  node_save($root);
  echo "root TOC updated\n";
}

// ---- Centre announcement --------------------------------------------------
$an_title = 'New: Course certificates';
$an_body = '<p>You can now print a <b>course certificate</b> for a student or server who attended a course. In the applicant list, choose <b>Certificate</b> in the <b>Action</b> column. Please first check your centre\'s text once in <b>Certificate Settings</b> on the centre page. <a href="/' . $alias . '">Read how it works</a>.</p>';
$an = db_query("select an_id from dh_announcement where an_audience = 'centre' and an_title = :t and an_deleted = 0", array(':t' => $an_title))->fetchField();
if ($an) {
  db_update('dh_announcement')->fields(array('an_body' => $an_body, 'an_updated_by' => 1))->condition('an_id', $an)->execute();
  echo "announcement $an: text updated (posted date unchanged)\n";
}
else {
  $an = db_insert('dh_announcement')->fields(array('an_audience' => 'centre', 'an_title' => $an_title, 'an_body' => $an_body,
    'an_posted' => date('Y-m-d H:i:s'), 'an_deleted' => 0, 'an_created_by' => 1, 'an_updated_by' => 1))->execute();
  echo "announcement posted: id $an\n";
}

drupal_flush_all_caches();
echo "done. Check /wiki, /$alias and a centre page (/centre/{id}) for the announcement.\n";
