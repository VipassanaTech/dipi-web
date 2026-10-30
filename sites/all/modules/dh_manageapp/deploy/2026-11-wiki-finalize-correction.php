<?php
// Help for centres: the "Correcting a Finalized Course" wiki page (book 10, alias wiki-finalize-correction),
// linked from the wiki's root contents, and a centre announcement pointing to it.
// Run from the Drupal root (php7.3 on prod, php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-wiki-finalize-correction.php
// Re-runnable: updates the page and the announcement if they exist.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$title = 'Correcting a Finalized Course';
$alias = 'wiki-finalize-correction';
$body = <<<'HTML'
<p>After you finalize a course, it is locked. If you find a mistake, you can reopen the course, fix it, and finalize it again.</p>
<h3>When to reopen</h3>
<ul>
<li>You forgot to assign the teachers.</li>
<li>A student left, but you did not mark them Left.</li>
<li>A student attended, but you did not mark them Attended.</li>
</ul>
<h3>How to reopen</h3>
<ol>
<li>Open the course page: on your Dashboard choose the course in <b>Select Course</b> or <b>Go to any course</b>, or in <b>Manage Courses</b> open <b>Finalized Courses</b> and click <b>View</b>.</li>
<li>Click <b>Reopen for correction</b>.</li>
<li>Write the reason, for example "Teacher was not assigned", and click <b>Reopen</b>.</li>
</ol>
<p>Only people who can finalize courses see the <b>Reopen for correction</b> button.</p>
<p>You can reopen a course within <b>30 days</b> of finalizing it. After 30 days, ask the DIPI team to reopen it.</p>
<h3>Fix the course</h3>
<p>The course page now shows <b>Reopened for correction</b>. You can use Zero Day, Assign Teachers and edit applications as before.</p>
<h3>Finalize again</h3>
<ol>
<li>Go to <b>Manage Courses</b>, select the course and click <b>Finalize Course</b>. Click <b>OK</b> when it asks if you are sure.</li>
<li>A box asks: <b>Send the Attended email to students newly marked attended</b>. Keep it ticked to send the email to the students you newly marked Attended. Untick it if you do not want to send it.</li>
<li>Click <b>Finalize again</b>.</li>
</ol>
<p>Finalizing runs in the background and can take up to 10 minutes.</p>
<p>Students who already got the Attended email will not get it again. Students you marked Left are taken out of this course in the student records.</p>
<p>The <b>Change Log</b> on the course page shows who reopened the course, when, and why.</p>
HTML;

$root_mlid = db_query("select mlid from book where bid = 10 and nid = 10")->fetchField();
if (!$root_mlid) { echo "ERROR: book root (bid=10, nid=10) not found on this site\n"; exit(1); }
$nid = db_query("select b.nid from book b join node n on n.nid = b.nid where b.bid = 10 and n.title = :t", array(':t' => $title))->fetchField();
if ($nid) { $node = node_load($nid); echo "[$alias] updating nid=$nid\n"; }
else {
  $node = new stdClass();
  $node->type = 'book';
  node_object_prepare($node);
  $node->uid = 1; $node->status = 1; $node->promote = 0; $node->comment = 0; $node->language = LANGUAGE_NONE;
  $w = (int) db_query("select max(weight) from menu_links where plid = :p", array(':p' => $root_mlid))->fetchField();
  $node->book = array('bid' => 10, 'plid' => $root_mlid, 'menu_name' => 'book-toc-10', 'weight' => $w + 1, 'module' => 'book');
  echo "[$alias] creating under root mlid=$root_mlid\n";
}
$node->title = $title;
$node->body[LANGUAGE_NONE][0] = array('value' => $body, 'format' => 'full_html');
node_save($node);
$src = 'node/' . $node->nid;
if (!db_query("select 1 from url_alias where source = :s and alias = :a", array(':s' => $src, ':a' => $alias))->fetchField()) {
  $p = array('source' => $src, 'alias' => $alias);
  path_save($p);
  echo "[$alias] alias set\n";
}
$root = node_load(10);
$rb = $root->body[LANGUAGE_NONE][0]['value'];
$close = (strpos($rb, '</ol>') !== FALSE) ? '</ol>' : ((strpos($rb, '</ul>') !== FALSE) ? '</ul>' : '');
if (strpos($rb, '/' . $alias . '"') !== FALSE) echo "[$alias] root contents already link the page\n";
elseif ($close === '') echo "[$alias] WARN: no list in the root body; add the link by hand\n";
else {
  $root->body[LANGUAGE_NONE][0]['value'] = preg_replace('#' . preg_quote($close, '#') . '#', '<li><a href="/' . $alias . '">' . $title . "</a></li>\n" . $close, $rb, 1);
  $root->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  node_save($root);
  echo "[$alias] root contents updated\n";
}

$an = array('an_audience' => 'centre', 'an_title' => 'You can now correct a finalized course',
  'an_body' => '<p>If you finalized a course with a mistake, for example teachers not assigned or Left students not marked, you can now fix it:</p>'
    . '<ol><li>Open the course page and click <b>Reopen for correction</b>.</li>'
    . '<li>Fix the course in Zero Day or Assign Teachers.</li>'
    . '<li>Finalize it again from <b>Manage Courses</b>.</li></ol>'
    . '<p>You can reopen within 30 days of finalizing. More help: <a href="/' . $alias . '">DIPI Wiki &rarr; ' . $title . '</a>.</p>');
$id = db_query("select an_id from dh_announcement where an_audience = :a and an_title = :t and an_deleted = 0", array(':a' => $an['an_audience'], ':t' => $an['an_title']))->fetchField();
if ($id) {
  $n = db_update('dh_announcement')->fields(array('an_body' => $an['an_body'], 'an_updated' => date('Y-m-d H:i:s'), 'an_updated_by' => 1))
    ->condition('an_id', $id)->condition('an_body', $an['an_body'], '<>')->execute();
  echo "announcement an_id $id already posted" . ($n ? ", text updated" : "") . "\n";
}
else echo 'announcement posted an_id ' . db_insert('dh_announcement')->fields($an + array('an_posted' => date('Y-m-d H:i:s'), 'an_created_by' => 1))->execute() . "\n";
drupal_flush_all_caches();
echo "done. Check /$alias\n";
