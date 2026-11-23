<?php
// ============================================================================
// Takes the merge field [LetterDetailURL] out of every letter (owner, 2026-11-23: "remove this
// merge field from letters", "remove merge field from code ... centers will get confused").
// The field was a "read this letter online" link to applicant.vridhamma.org/l.php, which is
// removed (dipi-applicant ab93791), so the link would only lead to "not found".
//
// What it changes (all centres, centre 0 template too, active and inactive letters):
//   1. dh_letter: subject, email body and SMS text.
//      - Plain text (subject, SMS): the field goes out with its lead-in words, so no half
//        sentence is left: "Please click here for more details - [LetterDetailURL] - [CentreName]"
//        -> "- [CentreName]". Also the broken spellings ([LetterDetail], "[LetterDetailURL" with no
//        closing bracket, [LetterDetailURLYour]).
//      - Email body (HTML): a table row holding the field goes out whole; "[LetterDetailURL]<br />"
//        in a list of fields goes out; the known "view ... at the link below: 👉 [LetterDetailURL]"
//        sentence goes out.
//   2. dh_letter_fields (centre's own fields, Manage Fields): same plain-text rule
//      (centre 58 [More] = "More details - [LetterDetailURL]" -> empty).
//   3. Wiki Letters page (nid 81): the [LetterDetailURL] row of the fields table.
// Every changed column goes to dh_letter_fix_backup first (before + after). The letter triggers
// write the Change Log; the changes are credited to the System user. Re-runnable: nothing left to do
// the second time.
//
// Run from the Drupal root (php7.3 on prod, php7.4 locally). Shows what it would do; writes only
// with --apply. --show also prints each different change (40 characters around the field):
//     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-23-remove-letter-detail-url.php [--show] [--apply]
// ============================================================================

/** The field, and its broken spellings seen in letters. */
function ldu_token_re()
{
	return '/\[LetterDetail(?:URL)?[A-Za-z]*\]?/i';
}

/** Joins the text before and after a removed piece: one space between words, no space before a line break or full stop. */
function ldu_join($left, $right)
{
	$l = rtrim($left, " \t");
	$r = ltrim($right, " \t");
	if ($l === '')
		return ltrim($r, "\r\n");
	if ($r === '')
		return $l;
	$lc = substr($l, -1);
	$rc = substr($r, 0, 1);
	if ($lc === "\n" || $lc === "\r")
		return $l . (($rc === "\n" || $rc === "\r") ? preg_replace('/^\r?\n/', '', $r) : $r);
	if ($rc === "\n" || $rc === "\r" || $rc === '.' || $rc === ',')
		return $l . $r;
	return $l . ' ' . $r;
}

/** Plain text (SMS text, subject, a centre's field): the field out, with its lead-in words. */
function ldu_clean_text($s)
{
	$s = (string) $s;
	for ($guard = 0; $guard < 20 && preg_match(ldu_token_re(), $s, $m, PREG_OFFSET_CAPTURE); $guard++) {
		$start = $m[0][1];
		$before = substr($s, 0, $start);
		$after = substr($s, $start + strlen($m[0][0]));
		// The lead-in starts after the last sentence end, line break or "|" before the field
		// (a full stop right against the field counts too: "next time .[LetterDetailURL]").
		$cs = 0;
		if (preg_match_all('/(?:[.!?|\x{0964}\x{17D4}](?=\s|$)|[\r\n])\s*/u', $before, $mm, PREG_OFFSET_CAPTURE)) {
			$last = end($mm[0]);
			$cs = $last[1] + strlen($last[0]);
		}
		$clause = substr($before, $cs);
		// The field alone on its line under a line that ends with ":" ("... click the following link:"):
		// that line is its lead-in and goes too, however long.
		$own_line = FALSE;
		if (trim($clause) === '' && preg_match('/(?:^|\n)([^\r\n\[]{1,250}:)[ \t]*(?:\r?\n[ \t]*)+$/u', $before, $pl, PREG_OFFSET_CAPTURE)) {
			$cs = $pl[1][1];
			$clause = substr($before, $cs);
			$own_line = TRUE;
		}
		if (preg_match('/^\s*[a-z]/', $after)) {
			// The sentence carries on after the field ("... the information at [LetterDetailURL] and reconfirm
			// your participation by writing to ..."): keep it, take out only the field and the word that points at it.
			$left = preg_replace('/\s*(?:\b(?:at|on|via|from)\b)?\s*[\-\x{2013}:]?\s*$/iu', '', $before);
		}
		elseif ((drupal_strlen(trim($clause)) <= 80 || $own_line) && strpos($clause, '[') === FALSE) {
			// "For more details - [LetterDetailURL]": the whole lead-in goes.
			$left = substr($before, 0, $cs);
			if (preg_match('/^\s*[.!?\x{0964}\x{17D4}]/u', $after, $t))
				$after = substr($after, strlen($t[0]));
			// a separator left hanging at the end of the kept text ("[EndDate] | <lead-in>")
			if (trim($clause) !== '' && preg_match('/[|\-\x{2013}:]\s*$/u', $left) && !preg_match('/[.!?]\s*[|\-\x{2013}:]\s*$/u', $left))
				$left = preg_replace('/\s*[|\-\x{2013}:]\s*$/u', '', $left);
		}
		else {
			// Inside a longer sentence: only the field and the dash or colon just before it, and a known
			// lead-in with no full stop before it (Khmer "please click here for details").
			$left = preg_replace('/\s*[\-\x{2013}:]?\s*$/u', '', $before);
			$left = preg_replace('/\s*(?:សូមចុចនៅទីនេះ)?សម្រាប់(?:ការ|ព័ត៌មាន)លំអិត\s*$/u', '', $left);
		}
		$s = ldu_join($left, $after);
	}
	return $s;
}

/** Email body (HTML): the field out without leaving an empty table row or a sentence pointing at nothing. */
function ldu_clean_html($h)
{
	$h = (string) $h;
	$tok = '\[LetterDetail(?:URL)?[A-Za-z]*\]?';
	// a table row holding the field ("LetterDetailURL | [LetterDetailURL]")
	$h = preg_replace('#<tr\b(?:(?!<tr\b|</tr>).)*' . $tok . '(?:(?!</tr>).)*</tr>#si', '', $h);
	// the field in a list of fields
	$h = preg_replace('#' . $tok . '\s*<br\s*/?>#i', '', $h);
	// "You may view your complete application and course details at the link below:<br />👉 [LetterDetailURL]"
	$h = preg_replace('#\s*You may view your complete application and course details at the link below:\s*<br\s*/?>\s*(?:\x{1F449}\s*)?' . $tok . '#iu', '', $h);
	// anything else: the field (and a pointing hand just before it)
	$h = preg_replace('#(?:\x{1F449}\s*)?' . $tok . '#iu', '', $h);
	return $h;
}

if (defined('LDU_FUNCTIONS_ONLY'))
	return;

if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
$apply = in_array('--apply', $argv);
$show = in_array('--show', $argv);
if (!db_table_exists('dh_letter_fix_backup')) { echo "ERROR: dh_letter_fix_backup is missing (deploy/2026-10-10-letter-fix-backup.sql)\n"; exit(1); }

$uid = _dh_email_system_uid();
$now = date('Y-m-d H:i:s');
$backup = function ($table, $row, $centre, $column, $before, $after) use ($now) {
	db_insert('dh_letter_fix_backup')->fields(array('b_table' => $table, 'b_row' => $row, 'b_center' => $centre,
		'b_column' => $column, 'b_before' => $before, 'b_after' => $after, 'b_at' => $now))->execute();
};
$seen = array();
$tail = function ($s, $n) { return mb_strlen($s, 'UTF-8') > $n ? mb_substr($s, -$n, NULL, 'UTF-8') : $s; };
$note = function ($what, $old, $new) use (&$seen, $show, $tail) {
	if (!$show || !preg_match(ldu_token_re(), $old, $m, PREG_OFFSET_CAPTURE))
		return;
	$p = $m[0][1];
	$a = preg_replace('/\s+/u', ' ', $tail(substr($old, 0, $p), 40) . '«' . $m[0][0] . '»' . mb_substr(substr($old, $p + strlen($m[0][0])), 0, 30, 'UTF-8'));
	// the new text around the same place: what came before the field, then 60 characters more
	$b = preg_replace('/\s+/u', ' ', $tail(mb_strcut($new, 0, min(strlen($new), $p + 60), 'UTF-8'), 100));
	$k = "$what | $a\n      => $b";
	$seen[$k] = isset($seen[$k]) ? $seen[$k] + 1 : 1;
};

// 1. Letters
$counts = array('l_subject' => 0, 'l_body' => 0, 'l_sms' => 0);
$letters = 0; $centres = array(); $long = 0;
$rows = db_query("select l_id, l_center, l_subject, l_body, l_sms from dh_letter
	where l_subject like '%LetterDetail%' or l_body like '%LetterDetail%' or l_sms like '%LetterDetail%' order by l_id")->fetchAll(PDO::FETCH_ASSOC);
foreach ($rows as $L) {
	$new = array();
	foreach ($counts as $k => $x) {
		$old = (string) $L[$k];
		if (!preg_match(ldu_token_re(), $old))
			continue;
		$v = ($k == 'l_body') ? ldu_clean_html($old) : ldu_clean_text($old);
		if ($v === $old)
			continue;
		$note($k, $old, $v);
		$new[$k] = $v;
		$counts[$k]++;
	}
	if (!$new)
		continue;
	$letters++;
	$centres[$L['l_center']] = 1;
	if ($apply) {
		$txn = db_transaction();
		try {
			foreach ($new as $k => $v)
				$backup('dh_letter', $L['l_id'], $L['l_center'], $k, (string) $L[$k], $v);
			db_update('dh_letter')->fields($new + array('l_updated' => $now, 'l_updated_by' => $uid))->condition('l_id', $L['l_id'])->execute();
		}
		catch (Throwable $e) {
			$txn->rollback();
			echo "ERROR at letter {$L['l_id']}: " . $e->getMessage() . "\n";
			exit(1);
		}
		unset($txn);
	}
}
printf("letters: %s %d letters in %d centres (subject %d, email body %d, SMS text %d)\n", $apply ? 'changed' : 'would change',
	$letters, count($centres), $counts['l_subject'], $counts['l_body'], $counts['l_sms']);

// 2. Centres' own fields
$fields = 0;
foreach (db_query("select lf_id, lf_center, lf_name, lf_value from dh_letter_fields where lf_value like '%LetterDetail%'")->fetchAll(PDO::FETCH_ASSOC) as $F) {
	$v = ldu_clean_text($F['lf_value']);
	if ($v === (string) $F['lf_value'])
		continue;
	$note('field c' . $F['lf_center'] . ' [' . $F['lf_name'] . ']', (string) $F['lf_value'], $v);
	$fields++;
	if ($apply) {
		$txn = db_transaction();
		try {
			$backup('dh_letter_fields', $F['lf_id'], $F['lf_center'], 'lf_value', (string) $F['lf_value'], $v);
			db_update('dh_letter_fields')->fields(array('lf_value' => $v, 'lf_updated_by' => $uid))->condition('lf_id', $F['lf_id'])->execute();
		}
		catch (Throwable $e) {
			$txn->rollback();
			echo "ERROR at field {$F['lf_id']}: " . $e->getMessage() . "\n";
			exit(1);
		}
		unset($txn);
	}
}
printf("centre fields: %s %d\n", $apply ? 'changed' : 'would change', $fields);

// 3. Wiki Letters page: the row of the fields table ("[LetterDetailURL] | A link where the applicant can read this letter online")
$node = node_load(81);
if (!$node || $node->title !== 'Letters')
	echo "wiki: node 81 is not the Letters page on this site, left as is\n";
else {
	$body = $node->body[LANGUAGE_NONE][0]['value'];
	$wiki = preg_replace('#<tr\b(?:(?!<tr\b|</tr>).)*\[LetterDetailURL\](?:(?!</tr>).)*</tr>[ \t]*\r?\n?#s', '', $body, -1, $n_rows);
	if ($n_rows > 1 || strpos($wiki, 'LetterDetailURL') !== FALSE) {
		echo "wiki: the Letters page holds the field outside the one table row, left as is (check by hand)\n";
		$wiki = $body;
	}
	if ($wiki === $body)
		echo "wiki: nothing to change\n";
	elseif (!$apply)
		echo "wiki: would take the [LetterDetailURL] row out of the Letters page (nid 81)\n";
	else {
		$node->body[LANGUAGE_NONE][0]['value'] = $wiki;
		$node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
		$node->revision = TRUE;
		$node->log = 'Merge field [LetterDetailURL] removed (2026-11-23)';
		node_save($node);
		echo "wiki: took the [LetterDetailURL] row out of the Letters page (nid 81, new revision {$node->vid})\n";
	}
}

if ($show) {
	arsort($seen);
	echo "\n" . count($seen) . " different changes (count | before => after):\n";
	foreach ($seen as $k => $n)
		echo "$n | $k\n";
}

// What is left anywhere (should be 0 after --apply)
$left = db_query("select count(*) from dh_letter where l_subject like '%LetterDetail%' or l_body like '%LetterDetail%' or l_sms like '%LetterDetail%'")->fetchField()
	+ db_query("select count(*) from dh_letter_fields where lf_value like '%LetterDetail%'")->fetchField();
echo "rows still holding the field: $left\n";
$pages = db_query("select entity_id from field_data_body where entity_type = 'node' and body_value like '%LetterDetail%'")->fetchCol();
echo "wiki/site pages still naming it: " . ($pages ? 'nid ' . implode(', ', $pages) : 'none') . "\n";
echo $apply ? "done (backup: dh_letter_fix_backup, b_at = '$now')\n" : "preview only: run with --apply to change\n";
