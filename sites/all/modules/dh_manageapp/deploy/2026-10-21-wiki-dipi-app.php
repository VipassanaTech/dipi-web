<?php
// ============================================================================
// Help for the Dipi App: a "Dipi App" page in the centre wiki (book 10, alias wiki-dipi-app)
// and one in the AT Portal wiki (book 46, alias at-wiki-dipi-app), each linked from its
// book's root contents list. The download page is /dipi-app (dh_courseviewer).
//
// Run from the Drupal root, on the command line only (php7.3 on prod, which has DOMDocument;
// php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-21-wiki-dipi-app.php
// Re-runnable: updates a page if it exists (matched by title in its book), and sets the
// alias and the root contents link once.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// What a teacher and the centre team both do in the app.
$using = <<<'HTML'
<ul>
<li>Tap a seat to see the student's full details.</li>
<li>Press and hold a seat for a quick look: courses, room, cell and health notes.</li>
<li><b>Search</b> (the magnifying glass): find a student by name or seat number, or a Dhamma server by name.</li>
<li>The section button (for example <b>Male ▾</b>) switches between the loaded plans and their servers lists (for example <b>Male servers</b>).</li>
<li><b>Notes</b>: press and hold a seat, or open the student's details, and tap <b>+ Add note</b>. The note gets the course day. Notes stay on this tablet only.</li>
<li>Tap the <b>?</b> button to see what each colour and mark on the plan means.</li>
</ul>
HTML;

$centre_body = <<<HTML
<p>The <b>Dipi App</b> shows the seating plan and the students of a running course on an Android tablet or phone. You load the course once, with internet. After that it works without internet, so a teacher can use it in the Dhamma hall.</p>

<p><b>Contents</b></p>
<ol>
<li><a href="#da-who">Who can use it</a></li>
<li><a href="#da-install">Download and install</a></li>
<li><a href="#da-load">Load a course</a></li>
<li><a href="#da-use">Using the app</a></li>
<li><a href="#da-edit">Changes on the tablet</a></li>
<li><a href="#da-clear">Clear the course</a></li>
<li><a href="#da-safe">Keep student details safe</a></li>
<li><a href="#da-problems">Problems</a></li>
</ol>

<h3 id="da-who">1. Who can use it</h3>
<ul>
<li><b>Centre team</b>: logins with the role <b>Centre Admin</b>, <b>Data Entry Operator</b> or <b>Day Zero User</b> can load any running course of their centre. The app shows the same side as this website: a <b>Data Entry Operator Male</b> login gets only the Male side, a <b>Data Entry Operator Female</b> login only the Female side. A <b>Day Zero User</b> login with no Male or Female access, or a <b>View Only</b> login, cannot use the app.</li>
<li><b>Teachers</b>: an Active teacher can load the courses they serve as Conducting or Assisting teacher, after you <b>Confirm</b> them on <b>Assign Teachers</b>. They see only the side you set for them there (<b>Sees</b>: Male, Female or Both).</li>
</ul>
<p>Log in to the app with the same username and password as on this website.</p>

<h3 id="da-install">2. Download and install</h3>
<p>The app is for Android 6.0 or newer. It is not for iPhone or iPad.</p>
<ol>
<li>On the Android tablet or phone, open this website and log in.</li>
<li>On the centre page, click <b>Dipi App (Android)</b>. You can also open <a href="/dipi-app">dipi.vridhamma.org/dipi-app</a>.</li>
<li>Tap <b>Download Dipi App</b>. When the download is done, open the file.</li>
<li>If Android asks, allow your browser to install apps (<b>Install unknown apps</b>). Then go back and tap <b>Install</b>.</li>
<li>If a <b>Play Protect</b> message comes, choose to install anyway (you may need to tap <b>More details</b> first).</li>
</ol>
<p>To update the app, download the new version from the same page and install it in the same way. The course on the tablet stays.</p>

<h3 id="da-load">3. Load a course</h3>
<ol>
<li>Open <b>Dipi App</b> and log in.</li>
<li>Under <b>Choose a course</b>, you see the courses running today. A course shows from Day 0 to its last day.</li>
<li>Tick the seating plans you need, for example <b>Male</b> or <b>Female</b> (in a group-wise course, <b>Group 1 · Male</b> and so on). Only the students of the ticked plans come to the tablet.</li>
<li>Tap <b>Download</b>. When it is done, the plan opens.</li>
</ol>
<p>To load another course, or other plans: <b>Admin</b> &rarr; <b>Log in to load a different course</b>. Loading a course again, even the same course, removes the notes and changes on the tablet.</p>

<h3 id="da-use">4. Using the app</h3>
$using

<h3 id="da-edit">5. Changes on the tablet</h3>
<p><b>Edit</b> changes the plan on the tablet: mark a student Left, move a student, set Chowky, Chair or Backrest, or change the room, cell or dining seat. Then tap <b>Save</b>.</p>
<p>These changes are <b>not sent to DIPI</b>. If DIPI also needs the change (for example a student who left), make it in DIPI too.</p>

<h3 id="da-clear">6. Clear the course</h3>
<ul>
<li>To remove the course from the tablet: <b>Admin</b> &rarr; <b>Clear course</b>, and log in. Only the centre team or a teacher of this course can clear it.</li>
<li>After the course, the app removes it by itself: from the second day after the last day, when the app is opened.</li>
</ul>

<h3 id="da-safe">7. Keep student details safe</h3>
<ul>
<li>The tablet has the students' personal details. Keep it with a trusted person, and lock it with a PIN or pattern.</li>
<li>Load only the plans you need.</li>
<li>Clear the course when it is no longer needed.</li>
<li>Screenshots are blocked in the app.</li>
</ul>

<h3 id="da-problems">8. Problems</h3>
<table>
<tr><th>Problem</th><th>What to do</th></tr>
<tr><td>"This login can't load courses in the Dipi App."</td><td>This login has no access to the app. Ask your centre admin.</td></tr>
<tr><td>"No course is running today for your account."</td><td>A course shows only from Day 0 to its last day. A teacher sees only the courses they serve, after you Confirm them.</td></tr>
<tr><td>"No students in this course yet."</td><td>No student has been registered yet. Try again after registration.</td></tr>
<tr><td>"Couldn't save the course on this tablet."</td><td>Free some space on the tablet and load the course again.</td></tr>
<tr><td>The file does not install.</td><td>The tablet may be older than Android 6.0. Use a newer tablet or phone.</td></tr>
</table>
HTML;

$teacher_body = <<<HTML
<p>The <b>Dipi App</b> shows the seating plan and the students of a course on an Android tablet or phone. After the course is loaded, it works without internet, so you can use it in the Dhamma hall.</p>

<h3>Which courses</h3>
<ul>
<li>The courses you serve as Conducting or Assisting teacher, after the centre has <b>Confirmed</b> you.</li>
<li>From Day 0 to the last day of the course.</li>
<li>You see the students of the side the centre set for you: Male, Female or Both.</li>
</ul>

<h3>Getting the app</h3>
<p>Often the centre gives you a tablet with the course already loaded. You can also use your own Android tablet or phone (Android 6.0 or newer; not iPhone or iPad):</p>
<ol>
<li>On the tablet or phone, open dipi.vridhamma.org and log in with your AT Portal username and password.</li>
<li>On the AT Portal dashboard, tap <b>Dipi App (Android)</b>, then <b>Download Dipi App</b>. You can also open <a href="/dipi-app">dipi.vridhamma.org/dipi-app</a>.</li>
<li>Open the downloaded file and tap <b>Install</b>. If Android asks, allow your browser to install apps. If a <b>Play Protect</b> message comes, choose to install anyway.</li>
<li>Open <b>Dipi App</b> and log in with the same username and password.</li>
<li>Under <b>Choose a course</b>, tick your plan and tap <b>Download</b>.</li>
</ol>

<h3>Using the app</h3>
$using

<h3>Keep in mind</h3>
<ul>
<li>Notes and changes stay on the tablet only. They are not sent to DIPI.</li>
<li>You can clear a course you serve: <b>Admin</b> &rarr; <b>Clear course</b>. After the course, the app also removes it by itself.</li>
<li>The tablet has the students' personal details. Keep it locked with a PIN or pattern.</li>
</ul>
HTML;

// The theme gives tables no borders or spacing, and its fixed top bar hides a heading the
// contents link jumps to: add both inline.
function dipi_app_wiki_style($body) {
  $body = str_replace(
    array('<table>', '<th>', '<td>'),
    array('<table style="border-collapse:collapse;margin:6px 0 12px">',
      '<th style="border:1px solid #ccc;padding:4px 8px;text-align:left;background:#f5f5f5">',
      '<td style="border:1px solid #ccc;padding:4px 8px;vertical-align:top">'),
    $body);
  return preg_replace('#<h3 id="([^"]+)">#', '<h3 id="$1" style="scroll-margin-top:180px">', $body);
}

/**
 * Create or update one book page, set its alias, and add it to the root page's contents
 * list ($list = 'ol' or 'ul') once.
 */
function dipi_app_wiki_page($bid, $title, $alias, $body, $list) {
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
  $node->body[LANGUAGE_NONE][0] = array('value' => dipi_app_wiki_style($body), 'format' => 'full_html');
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
  elseif (strpos($rb, "</$list>") === FALSE) {
    echo "[$alias] WARN: no </$list> in the root body; add the link by hand\n";
  }
  else {
    $root->body[LANGUAGE_NONE][0]['value'] = preg_replace("#</$list>#", '<li><a href="/' . $alias . '">' . $title . "</a></li>\n</$list>", $rb, 1);
    $root->body[LANGUAGE_NONE][0]['format'] = 'full_html';
    node_save($root);
    echo "[$alias] root contents updated\n";
  }
}

dipi_app_wiki_page(10, 'Dipi App', 'wiki-dipi-app', $centre_body, 'ol');
dipi_app_wiki_page(46, 'Dipi App', 'at-wiki-dipi-app', $teacher_body, 'ul');

drupal_flush_all_caches();
echo "done. Check /wiki-dipi-app and /at-wiki-dipi-app\n";
