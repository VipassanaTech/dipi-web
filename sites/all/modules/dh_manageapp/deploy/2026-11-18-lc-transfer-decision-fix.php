<?php
// One-time fix: LC applications that a recommending AT transferred to the registrar from the email review link
// before 18 Nov 2026, still waiting for the registrar, for a course not yet started. That page did not save the
// AT's decision (a stray comma), so the recommending decision still shows "Pending". This sets it to
// "Transfer to Registrar" and writes one Activity Log line each (system user). The AT's comment was never saved
// and cannot be brought back. Nothing else changes (status, ATs, codes).
// Which applications: status R-ATTransfer, course start today or later, decision Pending, and the last move to
// R-ATTransfer was by the email page (logged under the applicant user, uid COURSE-APPLICANT-UID, before the fix).
// Run from the Drupal root (php7.3 on prod as www-data, php7.4 locally):
//   dry run (default):  php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-18-lc-transfer-decision-fix.php [--ids=1,2]
//   apply:              ... --apply --ids=<the ids from the dry run>   (--apply needs --ids: only reviewed rows change)
//   rollback:           ... --rollback --ids=<ids>   (back to Pending, only rows this script changed and still waiting)
// Prints application ids only. Re-runnable: fixed rows are skipped.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$msg_fix = "Recommending AT decision set to 'Transfer to Registrar': the AT transferred this application from the email link, which did not save it before 18 Nov 2026 (the AT's comment was not saved).";
$msg_back = "Recommending AT decision set back to 'Pending' (rollback of the 18 Nov 2026 fix).";

$apply = in_array('--apply', $argv); $rollback = in_array('--rollback', $argv);
$ids = array();
foreach ($argv as $a)
	if (strpos($a, '--ids=') === 0)
		$ids = array_values(array_filter(array_map('intval', explode(',', substr($a, 6)))));
if (($apply || $rollback) && !$ids) { echo "--apply needs --ids=<ids from the dry run> (and so does --rollback)\n"; exit(1); }
if ($apply && $rollback) { echo "Choose one of --apply / --rollback\n"; exit(1); }
$applicant_uid = (int) db_query("select td_val1 from dh_type_detail where td_type = 'COURSE-APPLICANT' and td_key = 'COURSE-APPLICANT-UID'")->fetchField();

// Why an application is (not) one to fix: '' = fix it.
function df_reason($id, $applicant_uid)
{
	$a = db_query("select a.a_status s, a.a_center ce, c.c_start cs, al.al_recommending_approved d from dh_applicant a
		join dh_applicant_lc al on al.al_applicant = a.a_id join dh_course c on c.c_id = a.a_course where a.a_id = :a", array(':a' => $id))->fetchAssoc();
	if (!$a) return array('not an LC application', NULL);
	if ($a['s'] != 'R-ATTransfer') return array("status is {$a['s']}", $a);
	if ($a['cs'] < date('Y-m-d')) return array('course already started', $a);
	if ($a['d'] != 'Pending') return array("decision is already {$a['d']}", $a);
	$u = db_query("select l_user from dh_log where l_module = 'Applicant' and l_identifier = :a and l_event = 'Status Change' and l_msg like :m order by l_id desc limit 1",
		array(':a' => (string) $id, ':m' => '%To R-ATTransfer'))->fetchField();
	if ((int) $u !== $applicant_uid) return array('not moved there by the email page', $a);
	return array('', $a);
}

if (!$ids) {
	// Dry run over all candidates
	$ids = db_query("select a.a_id from dh_applicant a join dh_applicant_lc al on al.al_applicant = a.a_id join dh_course c on c.c_id = a.a_course
		where a.a_status = 'R-ATTransfer' and c.c_start >= curdate() and al.al_recommending_approved = 'Pending' order by a.a_id")->fetchCol();
	echo "Waiting in R-ATTransfer, course not started, decision Pending: " . count($ids) . "\n";
}

$fix = array(); $done = 0;
foreach ($ids as $id) {
	if ($rollback) {
		$st = db_query("select a.a_status s, a.a_center ce, al.al_recommending_approved d from dh_applicant a join dh_applicant_lc al on al.al_applicant = a.a_id where a.a_id = :a", array(':a' => $id))->fetchAssoc();
		$ours = db_query("select 1 from dh_log where l_module = 'Applicant' and l_identifier = :a and l_event = 'LC Workflow' and l_msg = :m", array(':a' => (string) $id, ':m' => $msg_fix))->fetchField();
		if (!$st || $st['s'] != 'R-ATTransfer' || $st['d'] != 'Transfer to Registrar' || !$ours) { echo "app $id: skip (not changed by this script, or no longer waiting)\n"; continue; }
		$txn = db_transaction();
		db_update('dh_applicant_lc')->fields(array('al_recommending_approved' => 'Pending'))->condition('al_applicant', $id)->condition('al_recommending_approved', 'Transfer to Registrar')->execute();
		logit($st['ce'], 'LC Workflow', $id, $msg_back);
		unset($txn);
		echo "app $id: set back to Pending\n"; $done++;
		continue;
	}
	list($why, $a) = df_reason($id, $applicant_uid);
	if ($why !== '') { echo "app $id: skip ($why)\n"; continue; }
	if (!$apply) { echo "app $id: will be fixed (course {$a['cs']})\n"; $fix[] = $id; continue; }
	$txn = db_transaction();
	$n = db_update('dh_applicant_lc')->fields(array('al_recommending_approved' => 'Transfer to Registrar'))
		->condition('al_applicant', $id)->condition('al_recommending_approved', 'Pending')->execute();
	if ($n) logit($a['ce'], 'LC Workflow', $id, $msg_fix);   // CLI: logged under the system user
	unset($txn);
	echo "app $id: " . ($n ? 'fixed' : 'skip (changed meanwhile)') . "\n"; $done += $n ? 1 : 0;
}
if ($rollback) echo "set back: $done\n";
elseif ($apply) echo "fixed: $done\n";
else echo "will be fixed: " . count($fix) . ($fix ? " — ids: " . implode(',', $fix) : '') . "\n";
