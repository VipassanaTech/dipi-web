<?php
/**
 * Long-course (LC) letter protection.
 *
 * Seven letters per centre drive the LC approval process. DIPI finds the two review letters (to
 * the ATs) by status and the other five by exact name. Their name, status, course type and
 * active/inactive state are locked for everyone in the screens (owner decision, 1 Nov 2026); only
 * an approved script changes them. Every screen and save path asks this file.
 */

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
 * spaces. The COLLATE is the dh_letter column collation (utf8mb4_general_ci). Every value is a placeholder. */
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
			$whens[] = "when :e COLLATE utf8mb4_general_ci = :v$i then :k$i";
			$i++;
		}
	foreach (dh_letter_lc_roles() as $key => $r)
		if ($r['event'] === NULL)
		{
			$args[":v$i"] = $r['name'];
			$args[":k$i"] = $key;
			$whens[] = "when :n COLLATE utf8mb4_general_ci = :v$i then :k$i";
			$i++;
		}
	return db_query("select case " . implode(' ', $whens) . " end", $args)->fetchField();
}

/* TRUE for the two review letters, which go to the ATs. */
function dh_letter_lc_is_review($key)
{
	return $key === 'r_review' || $key === 'a_review';
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
	}
	if ($key !== NULL && dh_letter_lc_is_review($key))
	{
		$body = isset($fields['l_body']) ? $fields['l_body'] : ($stored ? $stored['l_body'] : '');
		if (strpos((string) $body, '[ReviewLink]') === FALSE)
			return array($fields, t('This review letter must contain [ReviewLink] in its text: it is the only way the AT can open the application from the email.'));
	}
	return array($fields, '');
}

/* The active letter named $name at $centre_id (lowest id first), or 0. Inactive letters are never sent. */
function dh_letter_by_name($centre_id, $name)
{
	return (int) db_query("select l_id from dh_letter where l_center = :c and l_name = :n and l_deleted = 0 order by l_id limit 1",
		array(':c' => $centre_id, ':n' => $name))->fetchField();
}
