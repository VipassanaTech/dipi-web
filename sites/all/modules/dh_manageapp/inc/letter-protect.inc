<?php
/**
 * Long-course (LC) letter protection.
 *
 * Ten letters per centre drive the LC approval process (seven, plus three teacher-change letters to the student since
 * 21 Nov 2026). DIPI finds the two review letters (to
 * the ATs) by status and the other five by exact name. Their name, status, course type and
 * active/inactive state are locked for everyone in the screens (owner decision, 1 Nov 2026); only
 * an approved script changes them. Every screen and save path asks this file.
 */

/* The status of the automatic LC letters found by name (all but the two review letters), set once on 26 Nov 2026
 * (deploy/2026-11-26-lc-automatic-status.php). No application ever has this status, so these letters are never offered
 * as a Custom letter, in bulk email or by a send by status; DIPI sends them by name. Only they may have it, and their
 * status is locked (dh_letter_protect; owner, 26 Nov 2026). */
define('DH_LETTER_LC_AUTO_STATUS', 'LC Automatic');

/* The seven LC letters: how DIPI finds each (status, or exact name when 'event' is NULL), who gets it, what it is for. */
function dh_letter_lc_roles()
{
	return array(
		'r_review'   => array('event' => 'R-ATReview', 'name' => 'Recommending Review', 'to' => 'the recommending AT',
			'purpose' => 'Sent to the recommending AT when a long-course application arrives. It asks the AT to review the application.'),
		'a_review'   => array('event' => 'A-ATReview', 'name' => 'Area Teacher Review', 'to' => 'the area AT',
			'purpose' => 'Sent to the area AT after the recommending AT approves. It asks the area AT to review the application.'),
		'r_approved' => array('event' => NULL, 'name' => 'Recommending AT Approved', 'to' => 'the student',
			'purpose' => 'Sent to the student when the recommending AT approves the application.'),
		'r_rejected' => array('event' => NULL, 'name' => 'Recommending AT Rejected', 'to' => 'the student',
			'purpose' => 'Sent to the student when the recommending AT does not approve the application.'),
		'a_approved' => array('event' => NULL, 'name' => 'Area AT Approved', 'to' => 'the student',
			'purpose' => 'Sent to the student when the area AT approves the application.'),
		'a_rejected' => array('event' => NULL, 'name' => 'Area AT Rejected', 'to' => 'the student',
			'purpose' => 'Sent to the student when the area AT does not approve the application.'),
		'lc_admin'   => array('event' => NULL, 'name' => 'LC Admin Notification', 'to' => "the centre's LC admin",
			'purpose' => "Sent to the centre's LC admin email when a long-course application arrives."),
		'r_changed'  => array('event' => NULL, 'name' => 'Recommending AT Changed', 'to' => 'the student',
			'purpose' => 'Sent to the student when the application is passed to another recommending AT. Use [PreviousRecommendingAT] and [RecommendingAT].'),
		'a_changed'  => array('event' => NULL, 'name' => 'Area AT Changed', 'to' => 'the student',
			'purpose' => 'Sent to the student when the application is passed to another area AT. Use [PreviousAreaTeacher] and [AreaTeacher].'),
		'r_returned' => array('event' => NULL, 'name' => 'Recommending AT Returned', 'to' => 'the student',
			'purpose' => 'Sent to the student when the application comes back to the centre without the recommending AT\'s approval. Keep [ChooseTeacherLink] in the text: with it the student can choose another teacher.'),
	);
}

/* Close to how MySQL compares names (utf8mb4_general_ci): capitals and trailing spaces do not count.
 * dh_letter_protect() asks the database itself before a save. */
function _dh_letter_lc_norm($s)
{
	return drupal_strtolower(rtrim((string) $s, ' '));
}

/* The LC role of a letter (l_event, l_name), or NULL. A review status decides first, whatever the name. */
function dh_letter_lc_role($letter)
{
	$event = isset($letter['l_event']) ? (string) $letter['l_event'] : '';
	$name = _dh_letter_lc_norm(isset($letter['l_name']) ? $letter['l_name'] : '');
	foreach (dh_letter_lc_roles() as $key => $r)
		if ($r['event'] !== NULL && strcasecmp($event, $r['event']) == 0)
			return $key;
	foreach (dh_letter_lc_roles() as $key => $r)
		if ($r['event'] === NULL && $name === _dh_letter_lc_norm($r['name']))
			return $key;
	return NULL;
}

/* The LC role of a letter as the database compares it: one query, so MySQL decides accents and trailing
 * spaces. The COLLATE is the dh_letter column collation (utf8mb4_general_ci); CONVERT first makes it work on a
 * connection that is not utf8mb4 too. Every value is a placeholder. */
function _dh_letter_lc_role_sql($letter)
{
	$args = array(
		':e' => isset($letter['l_event']) ? (string) $letter['l_event'] : '',
		':n' => isset($letter['l_name']) ? (string) $letter['l_name'] : '',
	);
	$whens = array();
	$i = 0;
	foreach (dh_letter_lc_roles() as $key => $r)
		if ($r['event'] !== NULL)
		{
			$args[":v$i"] = $r['event'];
			$args[":k$i"] = $key;
			$whens[] = "when CONVERT(:e USING utf8mb4) COLLATE utf8mb4_general_ci = :v$i then :k$i";
			$i++;
		}
	foreach (dh_letter_lc_roles() as $key => $r)
		if ($r['event'] === NULL)
		{
			$args[":v$i"] = $r['name'];
			$args[":k$i"] = $key;
			$whens[] = "when CONVERT(:n USING utf8mb4) COLLATE utf8mb4_general_ci = :v$i then :k$i";
			$i++;
		}
	return db_query("select case " . implode(' ', $whens) . " end", $args)->fetchField();
}

/* TRUE for the two review letters, which go to the ATs. */
function dh_letter_lc_is_review($key)
{
	return $key === 'r_review' || $key === 'a_review';
}

/* The names of the LC letters DIPI finds by exact name (all but the two review letters, which it finds by status):
 * the letters that have the status DH_LETTER_LC_AUTO_STATUS. */
function dh_letter_lc_names()
{
	$names = array();
	foreach (dh_letter_lc_roles() as $r)
		if ($r['event'] === NULL)
			$names[] = $r['name'];
	return $names;
}

/**
 * The LC rules for one save of letter $id at $centre_id ($action: add, copy or edit). An LC letter keeps
 * its stored name, status and course type whatever was posted; no other letter may take a reserved
 * name or a review status; a review letter must contain [ReviewLink]. Returns array($fields, $error)
 * with the fields to save; $error is '' when the save may go ahead.
 */
function dh_letter_protect($centre_id, $action, $id, $fields)
{
	$roles = dh_letter_lc_roles();
	$stored = ($action == 'edit') ? db_query("select * from dh_letter where l_id = :l and l_center = :c", array(':l' => $id, ':c' => $centre_id))->fetchAssoc() : FALSE;
	if ($stored && ($key = dh_letter_lc_role($stored)) !== NULL)
	{
		foreach (array('l_name', 'l_event', 'l_course_type') as $col)
			$fields[$col] = $stored[$col];
	}
	else
	{
		$check = $fields + ($stored ? $stored : array());
		$key = dh_letter_lc_role($check);
		if ($key === NULL)
			$key = _dh_letter_lc_role_sql($check);
		if ($key !== NULL)
			return array($fields, t('This name or status belongs to the long-course letter "@r", which DIPI uses in the long-course approval process. Please choose another name and status.', array('@r' => $roles[$key]['name'])));
		// "LC Automatic" belongs to the automatic LC letters only (capitals and spaces around it do not count).
		if (isset($check['l_event']) && drupal_strtolower(trim((string) $check['l_event'])) === drupal_strtolower(DH_LETTER_LC_AUTO_STATUS))
			return array($fields, t('The status "@s" is only for the automatic long-course letters. Please choose another status.', array('@s' => DH_LETTER_LC_AUTO_STATUS)));
	}
	if ($key !== NULL && dh_letter_lc_is_review($key))
	{
		$body = isset($fields['l_body']) ? $fields['l_body'] : ($stored ? $stored['l_body'] : '');
		if (strpos((string) $body, '[ReviewLink]') === FALSE)
			return array($fields, t('This review letter must contain [ReviewLink] in its text: it is the only way the AT can open the application from the email.'));
	}
	if ($key === 'r_returned')
	{
		$body = isset($fields['l_body']) ? $fields['l_body'] : ($stored ? $stored['l_body'] : '');
		if (strpos((string) $body, '[ChooseTeacherLink]') === FALSE)
			return array($fields, t('This letter must contain [ChooseTeacherLink] in its text: it is how the student chooses another teacher.'));
	}
	return array($fields, '');
}

/* The active letter named $name at $centre_id (lowest id first), or 0. Inactive letters are never sent. */
function dh_letter_by_name($centre_id, $name)
{
	return (int) db_query("select l_id from dh_letter where l_center = :c and l_name = :n and l_deleted = 0 order by l_id limit 1",
		array(':c' => $centre_id, ':n' => $name))->fetchField();
}

/* The [ReviewLink] for an AT: the start of the link from the admin settings ('r' recommending, 'a' area) plus the
   AT's code. '' when the setting is empty, so a broken link is never sent. No address is written in code. */
function dh_letter_review_link($step, $code)
{
	$base = trim((string) variable_get($step == 'a' ? 'dh_lc_review_link_a' : 'dh_lc_review_link_r', ''));
	return $base === '' ? '' : $base.$code;
}

/* The [ChooseTeacherLink] for the student: the start of the link from the admin settings plus the application's
   choose code. '' when the setting is empty, so a broken link is never sent. */
function dh_letter_choose_link($code)
{
	$base = trim((string) variable_get('dh_lc_choose_link', ''));
	return $base === '' ? '' : $base.$code;
}

/* The student's letter about a teacher change - no reasons, only the facts:
     'reco'     Recommending AT Changed   ($previous = the former recommending AT)
     'area'     Area AT Changed           ($previous = the former area AT)
     'returned' Recommending AT Returned  (with a new one-time code for [ChooseTeacherLink])
   Found by exact name at the applicant's centre. When a centre has no such letter: logged like the other LC letters;
   for 'returned' the centre's own letter for the R-ATTransfer status is sent instead, as before (2 centres have one). */
function dh_lc_teacher_letter($app_id, $kind, $previous = '')
{
	$names = array('reco' => 'Recommending AT Changed', 'area' => 'Area AT Changed', 'returned' => 'Recommending AT Returned');
	$centre = db_query("select a_center from dh_applicant where a_id = :a", array(':a' => $app_id))->fetchField();
	$lid = dh_letter_by_name($centre, $names[$kind]);
	if (!$lid)
	{
		if ($kind == 'returned')
			return dh_send_letter('applicant', $app_id, 'R-ATTransfer');
		logit($centre, 'Letter', $app_id, 'No '.$names[$kind].' letter found!');
		return FALSE;
	}
	$extra = array();
	if ($kind == 'reco')
		$extra['prev_reco'] = $previous;
	elseif ($kind == 'area')
		$extra['prev_area'] = $previous;
	else
	{
		$code = dh_lc_review_code($app_id);
		db_update('dh_applicant_lc')->fields(array('al_choose_auth' => $code))->condition('al_applicant', $app_id)->execute();
		$extra['choose_link'] = dh_letter_choose_link($code);
	}
	dh_letter_extra_values($extra);
	try
	{
		$ret = dh_send_letter('applicant', $app_id, 'Custom', $lid);
	}
	finally
	{
		dh_letter_extra_values(array());
	}
	return $ret;
}

/* After the application form (DIPI Edit) changed a teacher during the review: tell the student who reviews it now.
   Not for a first teacher ($prev ''), and not when the review mail could not go and the application went back to the
   centre (that sends the "returned" letter instead). */
function dh_lc_edit_teacher_letters($app_id, $prev_reco, $prev_area)
{
	$status = db_query("select a_status from dh_applicant where a_id = :a", array(':a' => $app_id))->fetchField();
	if (trim((string) $prev_reco) !== '' && $status == 'R-ATReview')
		dh_lc_teacher_letter($app_id, 'reco', $prev_reco);
	if (trim((string) $prev_area) !== '' && $status == 'A-ATReview')
		dh_lc_teacher_letter($app_id, 'area', $prev_area);
}

/* The recommending AT the application had when it last moved to R-ATTransfer (the last "RecoAT Assigned to" line
   before that move in the Activity Log), or ''. */
function dh_lc_reco_at_return($app_id)
{
	$moved = db_query("select max(l_id) from dh_log where l_module = 'Applicant' and l_identifier = :a and l_event = 'Status Change' and l_msg like :m",
		array(':a' => (string) $app_id, ':m' => '% To R-ATTransfer'))->fetchField();
	if (!$moved)
		return '';
	$line = db_query("select l_msg from dh_log where l_module = 'Applicant' and l_identifier = :a and l_event = 'LC Workflow' and l_msg like :m and l_id < :i order by l_id desc limit 1",
		array(':a' => (string) $app_id, ':m' => 'RecoAT Assigned to %', ':i' => $moved))->fetchField();
	return $line ? trim(substr($line, strlen('RecoAT Assigned to '))) : '';
}

/* A new code for an LC review link: the application number plus 15 random letters and digits (the form the email
   review page accepts). random_int is a secure random source. */
function dh_lc_review_code($app_id)
{
	$chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
	$code = '';
	for ($i = 0; $i < 15; $i++)
		$code .= $chars[random_int(0, 61)];
	return $app_id.$code;
}

/* The "Automatic · LC review" label next to an LC letter's name. */
function dh_letter_lc_badge()
{
	return '<span class="lc-letter-badge">'.t('Automatic · LC review').'</span>';
}

/* The box above the Letters list. */
function dh_letter_lc_notice_list()
{
	return '<div class="lc-letter-notice">'.t('The letters marked Automatic · LC review are used by the long-course approval process. DIPI sends them automatically to the ATs, the student and the LC admin, so their status is LC Automatic (or the review status) and they are never offered as a Custom letter or for bulk email. You can change their subject and text, but not their name, status, course type or whether they are active.').'</div>';
}

/* The box at the top of an LC letter's edit page ($editing TRUE) or view page. */
function dh_letter_lc_notice_letter($key, $editing)
{
	$roles = dh_letter_lc_roles();
	$out = '<div class="lc-letter-notice"><strong>'.t('Long-course letter').'</strong> '.dh_letter_lc_badge().'<br>'.check_plain(t($roles[$key]['purpose']));
	if ($editing)
	{
		$out .= '<br>'.t('Change the subject and text carefully: ATs, students and the LC admin rely on this letter. Its name, status and course type are locked because DIPI finds the letter by them.');
		if (dh_letter_lc_is_review($key))
			$out .= '<br>'.t('Keep [ReviewLink] in the text: it is the only way the AT can open the application from the email.');
	}
	return $out.'</div>';
}
