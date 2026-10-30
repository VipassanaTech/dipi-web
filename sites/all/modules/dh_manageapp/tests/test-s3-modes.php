<?php
require __DIR__ . '/lib.php';
echo "Files: count and copy inside S3 (stand-ins, no real S3)\n";
$GLOBALS['t_s3'] = array('objects' => array('applications/zzq/a.pdf' => 1000, 'photos/zzq/a.jpg' => 200), 'copies' => array());
function t_s3_head($bucket, $key) { return isset($GLOBALS['t_s3']['objects'][$key]) ? $GLOBALS['t_s3']['objects'][$key] : FALSE; }
function t_s3_copy($bucket, $from, $to) { $GLOBALS['t_s3']['copies'][] = $to; $GLOBALS['t_s3']['objects'][$to] = $GLOBALS['t_s3']['objects'][$from]; return array('success' => TRUE, 'message' => ''); }
$r = array('a_photo' => 'private://photos/zzq/a.jpg', 'a_uri' => 'private:///applications/zzq/a.pdf', 'a_center' => 6, 'a_course' => 123);
$stub = array('s3_head' => 't_s3_head', 's3_copy' => 't_s3_copy');
$zero = array('files_copied' => 0, 'files_missing' => 0, 'bytes' => 0);
$sum = $zero; dh_course_sync_files($r, 42, $stub + array('files' => 'count'), $sum);
t_eq(array($sum['files_copied'], $sum['bytes'], count($GLOBALS['t_s3']['copies'])), array(2, 1200, 0), 'count: 2 files, 1200 bytes, nothing copied');
$sum = $zero; $out = dh_course_sync_files($r, 42, $stub + array('files' => 's3copy'), $sum);
t_eq(count($GLOBALS['t_s3']['copies']), 2, 's3copy: 2 copies made');
t_eq($out['sc_uri'], 'private://final/applications/6/123/00000042-000123.pdf', 'PDF path as finalize stores it');
t_eq($out['s_photo'], 'private://final/photo-id/6/123/00000042.jpg', 'photo path as finalize stores it');
$sum = $zero; dh_course_sync_files($r, 42, $stub + array('files' => 's3copy'), $sum);
t_eq(count($GLOBALS['t_s3']['copies']), 2, 'second run copies nothing (already there)');
$sum = $zero; dh_course_sync_files(array('a_photo' => 'private://photos/zzq/missing.jpg', 'a_uri' => '') + $r, 42, $stub + array('files' => 's3copy'), $sum);
t_eq($sum['files_missing'], 1, 'a missing file is counted, not fatal');
t_done();
