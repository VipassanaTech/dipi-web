<?php
require __DIR__ . '/lib.php';
echo "Files: count and copy inside S3 (stand-ins, no real S3)\n";
$GLOBALS['t_s3'] = array('objects' => array('applications/zzq/a.pdf' => 1000, 'photos/zzq/a.jpg' => 200), 'copies' => array(), 'head_calls' => 0);
function t_s3_reset() { $GLOBALS['t_s3'] = array('objects' => array('applications/zzq/a.pdf' => 1000, 'photos/zzq/a.jpg' => 200), 'copies' => array(), 'head_calls' => 0); }
function t_s3_head($bucket, $key) { $GLOBALS['t_s3']['head_calls']++; return isset($GLOBALS['t_s3']['objects'][$key]) ? $GLOBALS['t_s3']['objects'][$key] : FALSE; }
function t_s3_copy($bucket, $from, $to) { $GLOBALS['t_s3']['copies'][] = $to; $GLOBALS['t_s3']['objects'][$to] = $GLOBALS['t_s3']['objects'][$from]; return array('success' => TRUE, 'message' => ''); }
// Stand-ins for the failure cases: the copy fails; every check says "some other error"; only the check of the final file errors.
function t_s3_copy_fail($bucket, $from, $to) { $GLOBALS['t_s3']['copies'][] = $to; return array('success' => FALSE, 'message' => 'stand-in copy failure'); }
function t_s3_head_error($bucket, $key) { return NULL; }
function t_s3_head_final_error($bucket, $key) { return strpos($key, 'final/') === 0 ? NULL : t_s3_head($bucket, $key); }
$r = array('a_photo' => 'private://photos/zzq/a.jpg', 'a_uri' => 'private:///applications/zzq/a.pdf', 'a_center' => 6, 'a_course' => 123);
$stub = array('s3_head' => 't_s3_head', 's3_copy' => 't_s3_copy');
$zero = array('files_copied' => 0, 'files_missing' => 0, 'files_failed' => 0, 'bytes' => 0);
$PDF = 'private://final/applications/6/123/00000042-000123.pdf'; $PHOTO = 'private://final/photo-id/6/123/00000042.jpg';

t_in_txn(function () use ($r, $stub, $zero, $PDF, $PHOTO) {
  $sum = $zero; $out = dh_course_sync_files($r, 42, $stub + array('files' => 'count'), $sum);
  t_eq(array($sum['files_copied'], $sum['bytes'], count($GLOBALS['t_s3']['copies'])), array(2, 1200, 0), 'count: 2 files, 1200 bytes, nothing copied');
  t_eq(array($out['s_photo'], $out['sc_uri']), array(NULL, NULL), 'count: no path is returned (nothing was copied)');
  t_eq(array($sum['files_missing'], $sum['files_failed']), array(0, 0), 'count: none missing, none failed');

  $sum = $zero; $out = dh_course_sync_files($r, 42, $stub + array('files' => 's3copy'), $sum);
  t_eq(count($GLOBALS['t_s3']['copies']), 2, 's3copy: 2 copies made');
  t_eq($out['sc_uri'], $PDF, 'PDF path as finalize stores it');
  t_eq($out['s_photo'], $PHOTO, 'photo path as finalize stores it');
  t_eq(array($sum['files_copied'], $sum['bytes'], $sum['files_missing'], $sum['files_failed']), array(2, 1200, 0, 0), 's3copy: 2 files, 1200 bytes, none missing, none failed');

  $sum = $zero; $out = dh_course_sync_files($r, 42, $stub + array('files' => 's3copy'), $sum);
  t_eq(count($GLOBALS['t_s3']['copies']), 2, 'second run copies nothing (already there)');
  t_eq(array($out['s_photo'], $out['sc_uri']), array($PHOTO, $PDF), 'second run still returns both paths');
  t_eq($sum['files_copied'], 2, 'second run counts both files as in place');

  $sum = $zero; dh_course_sync_files(array('a_photo' => 'private://photos/zzq/missing.jpg', 'a_uri' => '') + $r, 42, $stub + array('files' => 's3copy'), $sum);
  t_eq(array($sum['files_missing'], $sum['files_failed']), array(1, 0), 'a missing file is counted, not fatal, not a failure');

  // The copy fails: no path is stored, nothing counts as copied, the failure is counted.
  t_s3_reset();
  $sum = $zero; $out = dh_course_sync_files($r, 42, array('s3_head' => 't_s3_head', 's3_copy' => 't_s3_copy_fail', 'files' => 's3copy'), $sum);
  t_eq(array($out['s_photo'], $out['sc_uri']), array(NULL, NULL), 'failed copy: no path stored');
  t_eq(array($sum['files_copied'], $sum['bytes'], $sum['files_failed'], $sum['files_missing']), array(0, 0, 2, 0), 'failed copy: not copied, 2 failed, none missing');

  // The check itself fails (permission, network): counted as failed, never as missing, nothing copied.
  t_s3_reset();
  $sum = $zero; $out = dh_course_sync_files($r, 42, array('s3_head' => 't_s3_head_error', 's3_copy' => 't_s3_copy', 'files' => 's3copy'), $sum);
  t_eq(array($sum['files_failed'], $sum['files_missing'], $sum['files_copied']), array(2, 0, 0), 's3copy: a check that errors is failed, not missing');
  t_eq(array(count($GLOBALS['t_s3']['copies']), $out['s_photo'], $out['sc_uri']), array(0, NULL, NULL), 's3copy: nothing copied, no path');
  $sum = $zero; dh_course_sync_files($r, 42, array('s3_head' => 't_s3_head_error', 's3_copy' => 't_s3_copy', 'files' => 'count'), $sum);
  t_eq(array($sum['files_failed'], $sum['files_missing'], $sum['files_copied']), array(2, 0, 0), 'count: a check that errors is failed, not missing');

  // The source is fine but the check of the final file errors: do not copy blind, do not store a path.
  t_s3_reset();
  $sum = $zero; $out = dh_course_sync_files($r, 42, array('s3_head' => 't_s3_head_final_error', 's3_copy' => 't_s3_copy', 'files' => 's3copy'), $sum);
  t_eq(array($sum['files_failed'], $sum['files_copied'], count($GLOBALS['t_s3']['copies']), $out['s_photo'], $out['sc_uri']), array(2, 0, 0, NULL, NULL), 'the final file check errors: failed, no copy, no path');

  // 'none' touches nothing; an unknown mode is refused instead of falling into a copy.
  t_s3_reset();
  $sum = $zero; $out = dh_course_sync_files($r, 42, $stub + array('files' => 'none'), $sum);
  t_eq(array($GLOBALS['t_s3']['head_calls'], $out['s_photo'], $out['sc_uri']), array(0, NULL, NULL), 'none: no S3 call, no path');
  $none = array('a_photo' => '', 'a_uri' => '') + $r;   // no files, so even a wrong code path could not reach S3
  $thrown = NULL;
  try { $sum = $zero; dh_course_sync_files($none, 42, $stub + array('files' => 'bogus'), $sum); } catch (Exception $e) { $thrown = $e->getMessage(); }
  t_eq($thrown, 'Unknown files mode: bogus', 'an unknown files mode throws');
});

// s3_head_file() and s3_copy_file() with a stand-in client passed in: nothing reaches S3.
echo "S3 helpers (stand-in client)\n";
class T_S3_Err extends Exception
{
  public $st; public $code;
  function __construct($m, $st, $code) { parent::__construct($m); $this->st = $st; $this->code = $code; }
  function getStatusCode() { return $this->st; }
  function getAwsErrorCode() { return $this->code; }
}
class T_S3_Client
{
  public $err; public $calls = array();
  function __construct($err = NULL) { $this->err = $err; }
  function headObject($a) { $this->calls[] = $a; if ($this->err) throw $this->err; return array('ContentLength' => '1234'); }
  function copyObject($a) { $this->calls[] = $a; if ($this->err) throw $this->err; return array(); }
}
t_in_txn(function () {
  $logs = function () { return (int) db_query("select count(*) from watchdog where type = 'S3Photo'")->fetchField(); };
  $n = $logs(); $c = new T_S3_Client();
  $got = s3_head_file('bkt', 'photos/a.jpg', $c);
  t_ok($got === 1234, 'head: size of an existing object, as an integer (got ' . var_export($got, TRUE) . ')');
  t_eq(array($c->calls[0]['Bucket'], $c->calls[0]['Key']), array('bkt', 'photos/a.jpg'), 'head: asks for the right bucket and key');
  t_eq($logs() - $n, 0, 'head: success writes no log');

  $cases = array(
    array('404 NotFound', new T_S3_Err('x', 404, 'NotFound'), FALSE), array('404 without a code', new T_S3_Err('x', 404, NULL), FALSE),
    array('NoSuchKey without a status', new T_S3_Err('x', NULL, 'NoSuchKey'), FALSE),
    array('403 AccessDenied', new T_S3_Err('denied', 403, 'AccessDenied'), NULL), array('500', new T_S3_Err('server', 500, 'InternalError'), NULL),
    array('network error', new Exception('Could not resolve host'), NULL),
  );
  foreach ($cases as $k) {
    $n = $logs(); $got = s3_head_file('bkt', 'photos/a.jpg', new T_S3_Client($k[1]));
    t_ok($got === $k[2], 'head: ' . $k[0] . ' gives ' . var_export($k[2], TRUE) . ' (got ' . var_export($got, TRUE) . ')');
    t_eq($logs() - $n, $k[2] === NULL ? 1 : 0, 'head: ' . $k[0] . ($k[2] === NULL ? ' is logged' : ' is not logged'));
  }
  $w = db_query("select message, variables from watchdog where type = 'S3Photo' order by wid desc limit 1")->fetchObject();
  $msg = strtr($w->message, unserialize($w->variables));
  t_ok(strpos($msg, 'bkt') !== FALSE && strpos($msg, 'photos/a.jpg') !== FALSE && strpos($msg, 'Could not resolve host') !== FALSE, 'head: the log names the bucket, the key and the message');

  // No client can be built (only checked here where the S3 settings file does not exist, so nothing can reach S3).
  if (!is_readable('/dhamma/htpasswd/s3-env.ini')) {
    $n = $logs(); $got = @s3_head_file('bkt', 'photos/a.jpg');
    t_ok($got === NULL, 'head: no client gives NULL (got ' . var_export($got, TRUE) . ')'); t_eq($logs() - $n, 1, 'head: no client is logged');
    $cp = @s3_copy_file('bkt', 'photos/a.jpg', 'final/a.jpg');
    t_eq($cp['success'], FALSE, 'copy: no client is a failure');
  } else echo "  (skipped: no-client check, the S3 settings file exists on this machine)\n";

  $c = new T_S3_Client();
  $cp = s3_copy_file('bkt', "photos/a b/\u{fc}.jpg", 'final/photo-id/6/123/00000042.jpg', $c);
  t_eq($cp['success'], TRUE, 'copy: success');
  t_eq(array($c->calls[0]['Bucket'], $c->calls[0]['Key'], $c->calls[0]['CopySource'], $c->calls[0]['ACL']),
    array('bkt', 'final/photo-id/6/123/00000042.jpg', 'bkt/photos/a%20b/%C3%BC.jpg', 'private'), 'copy: source is bucket/key with the key URL-encoded, slashes kept, private');
  $c = new T_S3_Client(new T_S3_Err('copy denied', 403, 'AccessDenied'));
  $cp = s3_copy_file('bkt', 'photos/a.jpg', 'final/a.jpg', $c);
  t_eq($cp['success'], FALSE, 'copy: an error is a failure');
  t_ok(strpos($cp['message'], 'copy denied') !== FALSE && strpos($cp['message'], 'final/a.jpg') !== FALSE, 'copy: the message names the target and the error');
  t_eq(count($c->calls), 5, 'copy: tried 5 times');
});
t_done();
