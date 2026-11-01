<?php
// Letter change log viewer (inc/letter-log.inc): readable lines and values, rows, and who sees it.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-log-viewer.php
require __DIR__ . '/lib.php';

t_eq(_dh_letter_log_lines('<p>Dear [Name],</p><p>Your course&nbsp;starts.</p>', TRUE), array('Dear [Name],', 'Your course starts.'), 'V1 HTML to lines');
t_eq(_dh_letter_log_lines("one\r\n\r\n  two  ", FALSE), array('one', 'two'), 'V1 plain text to lines');
t_eq(_dh_letter_log_diff('<p>A</p><p>B</p>', '<p>A</p><p>C</p>', TRUE), array('<span class="cl-del">B</span>', '<span class="cl-add">C</span>'), 'V2 only the changed lines');
list($o, $n) = _dh_letter_log_diff('<p>A</p>', '<p><b>A</b></p>', TRUE);
t_ok($o === $n && strpos($o, 'formatting') !== FALSE, 'V3 a formatting-only change says so');
list($o, $n) = _dh_letter_log_diff('<p>x</p>', '<p>&lt;b&gt;hi&lt;/b&gt;</p><script>alert(1)</script>', TRUE);
t_ok(strpos($n, '<script') === FALSE && strpos($n, '<b>') === FALSE && strpos($n, '&lt;b&gt;hi&lt;/b&gt;') !== FALSE, 'V4 stored HTML is shown as text');
t_eq(array(_dh_letter_log_value('coursetype', '0', array()), _dh_letter_log_value('inactive', '1', array()),
  _dh_letter_log_value('file', 'private://letters/zz.pdf', array()), _dh_letter_log_value('text', NULL, array())),
  array('All', 'Inactive', 'zz.pdf', '<span class="cl-empty">(empty)</span>'), 'V5 readable values');

// Run as a user: anonymous, or the first active user (not uid 1) who has / lacks a permission.
function tv_as($uid) { $GLOBALS['user'] = $uid ? user_load($uid) : drupal_anonymous_user(); drupal_static_reset('user_access'); }
function tv_uid($perm, $without = NULL) {
  $q = "select ur.uid from users_roles ur join role_permission p on p.rid = ur.rid join users u on u.uid = ur.uid
    where p.permission = :p and u.status = 1 and ur.uid > 1";
  $a = array(':p' => $perm);
  if ($without) {
    $q .= " and not exists (select 1 from users_roles ur2 join role_permission p2 on p2.rid = ur2.rid where ur2.uid = ur.uid and p2.permission = :w)";
    $a[':w'] = $without;
  }
  return (int) db_query($q . " order by ur.uid limit 1", $a)->fetchField();
}

t_in_txn(function () {
  $now = date('Y-m-d H:i:s');
  $id = (int) db_insert('dh_letter')->fields(array('l_center' => 9900001, 'l_event' => 'Received', 'l_course_type' => 0, 'l_name' => 'Zzqtest Letter',
    'l_subject' => 'Zzqtest subject', 'l_body' => '<p>Line one</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 777051, 'l_updated' => $now, 'l_updated_by' => 777051))->execute();
  // Spread the times so newest-first is clear (they are all written within the same second).
  db_query("update dh_letter_log set ll_tstamp = ll_tstamp - interval 2 minute where ll_area = 'letter' and ll_record = :r", array(':r' => $id));
  db_update('dh_letter')->fields(array('l_subject' => 'Zzqtest subject 2', 'l_updated_by' => 777052))->condition('l_id', $id)->execute();
  db_query("update dh_letter_log set ll_tstamp = ll_tstamp - interval 1 minute where ll_area = 'letter' and ll_record = :r and ll_field = 'l_subject' and ll_action = 'update'", array(':r' => $id));
  db_update('dh_letter')->fields(array('l_deleted' => 1, 'l_updated_by' => 777053))->condition('l_id', $id)->execute();

  $rows = dh_letter_log_rows(9900001);
  t_eq(count($rows), 3, 'V6 created, a subject change and made inactive: 3 rows');
  t_ok(strpos($rows[0]['what'], 'Made inactive') !== FALSE, 'V6 newest first: made inactive');
  t_eq(array($rows[1]['what'], $rows[1]['old'], $rows[1]['new'], $rows[1]['by']), array('Subject', 'Zzqtest subject', 'Zzqtest subject 2', 'user 777052'), 'V6 then the subject change');
  t_ok(strpos($rows[2]['what'], 'Created') !== FALSE && strpos($rows[2]['new'], 'Status: Received') !== FALSE && strpos($rows[2]['new'], 'Letter Body: ') !== FALSE, 'V6 then one Created row listing its fields');
  t_ok(strpos($rows[2]['item'], "letters/9900001/view/$id") !== FALSE, 'V6 a letter of this centre links to its page');
  t_eq(count(dh_letter_log_rows(9900001, $id)), 3, 'V7 the letter page shows that letter only');
  t_eq(count(dh_letter_log_rows(9900002)), 0, 'V7 nothing of it shows on another centre');

  db_insert('dh_letter_fields')->fields(array('lf_center' => 9900001, 'lf_name' => 'Zzqtest Field', 'lf_value' => 'v', 'lf_deleted' => 0, 'lf_updated_by' => 777054))->execute();
  $field = array_values(array_filter(dh_letter_log_rows(9900001), function ($r) { return strpos($r['item'], 'Merge field [Zzqtest Field]') !== FALSE; }));
  t_ok(count($field) == 1 && strpos($field[0]['what'], 'Added') !== FALSE, 'V8 a new merge field shows as one Added row');

  tv_as(0);
  t_eq(dh_letter_log_section(9900001), '', 'V9 no log for someone who cannot manage letters');
  tv_as(tv_uid('manage letters'));
  $html = dh_letter_log_section(9900001);
  t_ok(strpos($html, 'letter-change-log') !== FALSE && strpos($html, 'Change Log (4 changes)') !== FALSE, 'V10 a letter manager sees the section with 4 changes');
  t_ok(strpos($html, 'collapsed') !== FALSE, 'V10 closed at first');
  t_ok(strpos(dh_letter_log_section(9900001, $id), 'Letter or field') === FALSE, "V10 a letter's own log has no Letter column");
  tv_as(tv_uid('view letters', 'manage letters'));
  t_eq(dh_letter_log_section(9900001), '', 'V11 someone who can only read letters does not see it');
});

// V12: stored text is escaped exactly once wherever it shows (letter name, subject, course type, file, merge field, user name).
t_in_txn(function () {
  tv_as(tv_uid('manage letters'));
  $now = date('Y-m-d H:i:s');
  db_insert('users')->fields(array('uid' => 777061, 'name' => 'Zzqtest <b>who</b>', 'pass' => '', 'status' => 1, 'created' => time(), 'changed' => time()))->execute();
  $ct = (int) db_insert('dh_type_detail')->fields(array('td_type' => 'COURSE-TYPE', 'td_key' => 'Zzqtest', 'td_val1' => 'Zzqtest <i>ct</i>',
    'td_val2' => 0, 'td_val3' => '', 'td_val4' => '', 'td_active' => 1))->execute();
  $id = (int) db_insert('dh_letter')->fields(array('l_center' => 9900001, 'l_event' => 'Received', 'l_course_type' => 0, 'l_name' => 'Zzqtest <script>x</script>',
    'l_subject' => 'Zzqtest <b>bold</b>', 'l_body' => '', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 777062, 'l_updated' => $now, 'l_updated_by' => 777062))->execute();
  db_update('dh_letter')->fields(array('l_subject' => 'Zzqtest <i>new</i>', 'l_course_type' => $ct,
    'l_attachment' => 'private://letters/Zzqtest <u>a.pdf', 'l_updated_by' => 777061))->condition('l_id', $id)->execute();
  db_insert('dh_letter_fields')->fields(array('lf_center' => 9900001, 'lf_name' => 'Zzqtest <u>f</u>', 'lf_value' => 'v', 'lf_deleted' => 0, 'lf_updated_by' => 777061))->execute();

  $html = dh_letter_log_section(9900001);
  t_ok(strpos($html, '<script') === FALSE && strpos($html, '<b>') === FALSE && strpos($html, '<i>') === FALSE && strpos($html, '<u>') === FALSE,
    'V12 no stored tag reaches the page as a tag');
  t_ok(strpos($html, '&lt;script&gt;x&lt;/script&gt;') !== FALSE, 'V12 the letter name is escaped (link and created row)');
  t_ok(strpos($html, '&lt;i&gt;new&lt;/i&gt;') !== FALSE, 'V12 the new subject is escaped');
  t_ok(strpos($html, 'Merge field [Zzqtest &lt;u&gt;f&lt;/u&gt;]') !== FALSE, 'V12 the merge field label is escaped');
  t_ok(strpos($html, 'Zzqtest &lt;b&gt;who&lt;/b&gt;') !== FALSE, 'V12 the user name in By is escaped');
  t_ok(strpos($html, '&lt;i&gt;ct&lt;/i&gt;') !== FALSE, 'V12 the course type name is escaped');
  t_ok(strpos($html, 'Zzqtest &lt;u&gt;a.pdf') !== FALSE, 'V12 the attachment file name is escaped');
  t_ok(strpos($html, '&amp;lt;') === FALSE, 'V12 nothing is escaped twice');
});
t_done();
