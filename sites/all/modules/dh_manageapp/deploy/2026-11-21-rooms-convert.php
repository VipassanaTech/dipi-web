<?php
// Rooms: convert every centre's typed room list (dh_center_setting_acco) into rooms + places.
// DRY RUN by default: prints the report, writes nothing. --apply writes. --centre=ID limits to one centre.
// Run from the Drupal root (prod: php7.3):  php sites/all/modules/dh_manageapp/deploy/2026-11-21-rooms-convert.php [--apply] [--centre=ID]
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
$apply = in_array('--apply', $argv, TRUE);
$only = 0;
foreach ($argv as $a)
	if (preg_match('/^--centre=(\d+)$/', $a, $m))
		$only = (int) $m[1];
$ids = $only ? array($only) : db_query("select distinct csa_center from dh_center_setting_acco where csa_deleted = 0 and csa_center <> 0 order by csa_center")->fetchCol();
$cols = array('rooms', 'shared_rooms', 'places', 'duplicates', 'too_long', 'review_added', 'past_only_missing');
$tot = array_fill_keys($cols, 0);
$skipped = 0;
echo ($apply ? "APPLY" : "DRY RUN (nothing is written)") . " — " . count($ids) . " centres\n";
printf("%-6s %-32s %6s %7s %7s %5s %5s %7s %10s %s\n", 'centre', 'name', 'rooms', 'shared', 'places', 'dups', 'long', 'review', 'past-only', 'note');
foreach ($ids as $c)
{
	$r = dh_rooms_convert_centre($c, $apply, dh_system_uid());
	$name = (string) db_query("select c_name from dh_center where c_id = :c", array(':c' => $c))->fetchField();
	printf("%-6s %-32s %6d %7d %7d %5d %5d %7d %10d %s\n", $c, mb_substr($name, 0, 32), $r['rooms'], $r['shared_rooms'], $r['places'], $r['duplicates'],
		$r['too_long'], $r['review_added'], $r['past_only_missing'], $r['skipped']);
	if ($r['skipped'] !== '') { $skipped++; continue; }
	foreach ($cols as $k)
		$tot[$k] += $r[$k];
}
printf("TOTAL  %-32s %6d %7d %7d %5d %5d %7d %10d skipped:%d\n", '', $tot['rooms'], $tot['shared_rooms'], $tot['places'], $tot['duplicates'], $tot['too_long'],
	$tot['review_added'], $tot['past_only_missing'], $skipped);
