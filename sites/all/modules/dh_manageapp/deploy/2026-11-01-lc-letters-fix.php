<?php
// One-time long-course letter fix (design "LC Letter Protection", 1 Nov 2026). Dry run by default; --apply changes.
// Run from the Drupal root:
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-01-lc-letters-fix.php [--apply]
//   prod:  sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-01-lc-letters-fix.php [--apply]
// Needs the two review-link settings (DIPI admin settings > Applicant Settings) first. Writes as System in one
// transaction, so the Letter Change Log records every change; a second run changes nothing.
// Prints centre ids and names, letter ids and names, and counts only.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once dirname(__DIR__) . '/inc/lc-letters-fix.inc';   // one-time code: the site itself does not load it

$apply = FALSE;
foreach (array_slice($argv, 1) as $arg)
{
	if ($arg === '--apply')
		$apply = TRUE;
	else
	{
		echo "Unknown option: $arg (nothing was done)\n";
		exit(1);
	}
}
try
{
	$plan = dh_lc_fix_plan();
	if ($plan && $plan[0]['do'] == 'settings_missing')
	{
		echo "The review-link settings are empty. Set both first (DIPI admin settings, Applicant Settings). Nothing was done.\n";
		exit(1);
	}
	$roles = dh_letter_lc_roles();
	$names = db_query("select c_id, c_name from dh_center")->fetchAllKeyed();
	$by = array();
	$tot = array();
	foreach ($plan as $a)
	{
		if ($a['do'] == 'add')
			$what = 'add "'.$roles[$a['key']]['name'].'"';
		elseif ($a['do'] == 'activate')
			$what = 'make letter '.$a['l_id'].' ("'.$roles[$a['key']]['name'].'") active';
		elseif ($a['do'] == 'status')
			$what = 'status R-ATReview on letter '.$a['l_id'];
		else
			$what = 'link '.$a['how'].' on letter '.$a['l_id'];
		$by[$a['centre']][] = $what;
		$k = $a['do'].($a['do'] == 'link' ? '-'.$a['how'] : '');
		$tot[$k] = (isset($tot[$k]) ? $tot[$k] : 0) + 1;
	}
	foreach ($by as $c => $list)
		echo $c.' '.($c == 0 ? '(template)' : (isset($names[$c]) ? $names[$c] : '?')).': '.implode('; ', $list)."\n";
	echo ($apply ? 'APPLY' : 'DRY RUN').': '.count($by).' centres, '.count($plan).' changes '.json_encode($tot)."\n";
	if (!$apply)
		exit(0);
	$txn = db_transaction();
	try
	{
		$n = dh_lc_fix_apply($plan, dh_system_uid());
	}
	catch (Throwable $e)
	{
		$txn->rollback();
		throw $e;
	}
	unset($txn);   // commit
	echo 'applied: '.json_encode($n)."\n";
	$again = dh_lc_fix_plan();
	echo 'second plan: '.count($again)." changes (0 expected)\n";
	exit(count($again) ? 1 : 0);
}
catch (Throwable $e)
{
	echo 'FAILED: '.get_class($e).': '.$e->getMessage()."\n";
	exit(1);
}
