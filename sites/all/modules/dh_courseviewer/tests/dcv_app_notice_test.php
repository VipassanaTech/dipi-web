<?php
// sites/all/modules/dh_courseviewer/tests/dcv_app_notice_test.php
//
// "A new Dipi App is ready": the course list (dcv/courses) tells the app which release the
// /dipi-app page offers (version, its Android versionCode, date), and only when that page can
// really offer it (the APK named in dcv_app_release() is there with the right sha256). The app
// compares the versionCode with its own. Runs against THIS worktree's code; read-only.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc', 'dcv-download.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }

$rel = dcv_app_release();
ok(isset($rel['version_code']) && is_int($rel['version_code']) && $rel['version_code'] > 0, 'the release names its Android versionCode');

// --- the rule (pure)
$r = array('version' => '1.1.9', 'version_code' => 20, 'date' => '2026-12-01', 'file' => 'x.apk', 'sha256' => 'abc');
ok(dcv_app_notice($r, 'private://dipi-app/x.apk') === array('version' => '1.1.9', 'version_code' => 20, 'date' => '2026-12-01'),
  'offered: version, versionCode and date (no file name or checksum)');
ok(dcv_app_notice($r, FALSE) === NULL, 'the APK is missing or wrong: nothing to tell');
ok(dcv_app_notice(array('version' => '1.1.9', 'date' => '') + array('file' => 'x'), 'private://x') === NULL, 'no versionCode: nothing to tell');
ok(dcv_app_notice(array('version' => '', 'version_code' => 20, 'date' => ''), 'private://x') === NULL, 'no version: nothing to tell');

// --- live values on this Dipi
$latest = dcv_app_latest();
$file = dcv_app_file();
ok($file ? ($latest !== NULL && $latest['version'] === $rel['version'] && $latest['version_code'] === $rel['version_code'])
         : ($latest === NULL), 'dcv_app_latest(): the offered release, or nothing when the page has no file');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
