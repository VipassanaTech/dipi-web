<?php
// Help for centres: section "12. Warnings when a status would get no letter" on the Letters wiki page
// (nid 81, alias wiki-letters, book 10). Explains the letter warnings (inc/letter-coverage.inc).
// Run from the Drupal root (php7.3 on prod, php7.4 locally):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-17-wiki-letter-warnings.php
// Re-runnable: replaces the section between its markers, or adds it at the end. Saves a new
// node revision, so the previous text stays in the page's revision history.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$section = <<<'HTML'
<!-- letter-warnings -->
<h3>12. Warnings when a status would get no letter</h3>
<p>DIPI warns you when a status would have no letter.</p>
<ul>
<li><b>When you save a letter:</b> if your change means some course types will get no letter for a status (for example, you change Course Type from All to one course type), DIPI does not save it yet. It shows a red list of the status and course types that would get no letter. If that is what you want, tick <b>Save anyway</b> and save again.</li>
<li><b>When you make a letter inactive or active:</b> DIPI checks the same way. Click <b>Make inactive anyway</b> (or <b>Make active anyway</b>) only if you are sure.</li>
<li><b>Review:</b> Review is an internal status. If you give a letter the status Review, DIPI warns you, because that letter will be emailed to every applicant moved to Review.</li>
<li><b>Letter check:</b> on the Letters page, open <b>Letter check</b> to see which letter DIPI sends for each status and each course type you run. <b>No letter</b> in red means the applicant gets no letter for that status.</li>
<li><b>Search page:</b> when you choose a status for an applicant and there is no letter for it, a red note under the status says so. You can still click Update: the status changes, but no letter goes out.</li>
</ul>
<!-- /letter-warnings -->
HTML;

$node = node_load(81);
if (!$node || $node->title !== 'Letters') { echo "ERROR: node 81 is not the Letters wiki page on this site\n"; exit(1); }
$body = $node->body[LANGUAGE_NONE][0]['value'];
if (strpos($body, '<!-- letter-warnings -->') !== FALSE) {
  $new = preg_replace('#<!-- letter-warnings -->.*?<!-- /letter-warnings -->#s', str_replace('$', '\\$', $section), $body, 1);
  $what = 'updated the section';
}
else {
  $new = rtrim($body) . "\n" . $section . "\n";
  $what = 'added the section at the end';
}
if ($new === $body) { echo "[wiki-letters] no change\n"; exit(0); }
$node->body[LANGUAGE_NONE][0]['value'] = $new;
$node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
$node->revision = TRUE;
$node->log = 'Letter warnings section (2026-11-17)';
node_save($node);
echo "[wiki-letters] $what (nid 81, new revision " . $node->vid . ")\n";
