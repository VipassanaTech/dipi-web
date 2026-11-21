<?php
// The three teacher-change letters to the student (21 Nov 2026): "Recommending AT Changed", "Area AT Changed",
// "Recommending AT Returned". Adds each one a centre does not have yet (by exact name, active) to the template
// (centre 0), every active centre (as the 1 Nov LC letter fix: dh_lc_fix_centres) and the Dummy Centre (302, for
// testing). Written as System (uid 15), so the Letter Change Log records them. A second run adds nothing.
// Dry run by default. Run from the Drupal root:
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-21-lc-teacher-change-letters.php [--apply | --rollback]
//   prod:  sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-21-lc-teacher-change-letters.php [--apply | --rollback]
// --rollback marks deleted the letters with these names that System added (l_created_by 15) and nobody changed since.
// Prints counts and centre ids only.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once dirname(__DIR__) . '/inc/lc-letters-fix.inc';   // dh_lc_fix_centres(); the site itself does not load this file

$mode = 'dry';
foreach (array_slice($argv, 1) as $arg)
{
	if ($arg === '--apply')
		$mode = 'apply';
	elseif ($arg === '--rollback')
		$mode = 'rollback';
	else
	{
		echo "Unknown option: $arg (nothing was done)\n";
		exit(1);
	}
}
$SYSTEM = 15;
$letters = array(
	'Recommending AT Changed' => array(
		'subject' => 'Recommending Assistant Teacher changed for [CourseType] from [StartDate] to [EndDate]',
		'body' => '<p>Dear [FirstName]&nbsp;[LastName],</p><p>Your long course application for [CourseName] has been reassigned from the Recommending Assistant Teacher [PreviousRecommendingAT] to the Recommending Assistant Teacher [RecommendingAT] for review.</p><p>You will be contacted if there are any questions about your application.</p><p>Regards,</p><p>[Footer]</p>',
		'sms' => 'Your long course application for [CourseName] has been reassigned from the Recommending Assistant Teacher [PreviousRecommendingAT] to [RecommendingAT] for review.'),
	'Area AT Changed' => array(
		'subject' => 'Area Teacher changed for [CourseType] from [StartDate] to [EndDate]',
		'body' => '<p>Dear [FirstName]&nbsp;[LastName],</p><p>Your long course application for [CourseName] has been reassigned from the Area Teacher [PreviousAreaTeacher] to the Area Teacher [AreaTeacher] for review.</p><p>You will be contacted if there are any questions about your application.</p><p>Regards,</p><p>[Footer]</p>',
		'sms' => 'Your long course application for [CourseName] has been reassigned from the Area Teacher [PreviousAreaTeacher] to [AreaTeacher] for review.'),
	'Recommending AT Returned' => array(
		'subject' => 'Your long course application for [CourseType] from [StartDate] to [EndDate]',
		'body' => '<p>Dear [FirstName]&nbsp;[LastName],</p><p>Your long course application for [CourseName] has been returned to the centre without approval from your Recommending Assistant Teacher [RecommendingAT].</p><p>If you know another Assistant Teacher who is willing to approve your application, please use this link to choose them: [ChooseTeacherLink]</p><p>Regards,</p><p>[Footer]</p>',
		'sms' => 'Your long course application for [CourseName] has been returned to the centre without approval from your Recommending Assistant Teacher [RecommendingAT]. If you know another Assistant Teacher who is willing to approve it, please choose them here: [ChooseTeacherLink]'),
);
$centres = array_values(array_unique(array_merge(array(0, 302), dh_lc_fix_centres())));
sort($centres);

if ($mode == 'rollback')
{
	$ids = db_query("select l_id from dh_letter where l_name in (:n) and l_created_by = :s and l_updated_by = :s and l_updated = l_created and l_deleted = 0",
		array(':n' => array_keys($letters), ':s' => $SYSTEM))->fetchCol();
	if ($ids)
		db_update('dh_letter')->fields(array('l_deleted' => 1, 'l_updated' => date('Y-m-d H:i:s'), 'l_updated_by' => $SYSTEM))->condition('l_id', $ids, 'IN')->execute();
	echo "Rollback: " . count($ids) . " letters marked deleted (letters changed since by a centre were left).\n";
	exit(0);
}

$add = array();
foreach ($centres as $c)
	foreach ($letters as $name => $l)
		if (!dh_letter_by_name($c, $name))
			$add[] = array($c, $name);
$per = array();
foreach ($add as $a)
	$per[$a[1]] = (isset($per[$a[1]]) ? $per[$a[1]] : 0) + 1;
echo count($centres) . " centres (template 0, Dummy Centre 302 and the active centres). Letters to add: " . count($add) . " " . json_encode($per) . "\n";
if ($mode == 'dry')
{
	echo "DRY RUN: nothing was added. Run again with --apply.\n";
	exit(0);
}
$now = date('Y-m-d H:i:s');
$txn = db_transaction();
try
{
	foreach ($add as $a)
	{
		$l = $letters[$a[1]];
		db_insert('dh_letter')->fields(array('l_center' => $a[0], 'l_event' => 'Custom', 'l_course_type' => 0, 'l_name' => $a[1],
			'l_subject' => $l['subject'], 'l_body' => $l['body'], 'l_sms' => $l['sms'], 'l_attachment' => '', 'l_deleted' => 0,
			'l_created' => $now, 'l_created_by' => $SYSTEM, 'l_updated' => $now, 'l_updated_by' => $SYSTEM))->execute();
	}
}
catch (Exception $e)
{
	$txn->rollback();
	echo "FAILED, nothing was added: " . $e->getMessage() . "\n";
	exit(1);
}
unset($txn);
$left = 0;
foreach ($centres as $c)
	foreach ($letters as $name => $l)
		if (!dh_letter_by_name($c, $name))
			$left++;
echo "APPLIED: " . count($add) . " letters added; centres still missing one: $left\n";
