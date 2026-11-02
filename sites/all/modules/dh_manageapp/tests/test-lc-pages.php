<?php
// Long-course letters on the Letters page, the edit page and the view page (inc/letters.inc, letter-protect.inc).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-pages.php
require __DIR__ . '/lib.php';

$mgr = (int) db_query("select ur.uid from users_roles ur join role_permission p on p.rid = ur.rid join users u on u.uid = ur.uid
  where p.permission = 'manage letters' and u.status = 1 and ur.uid > 1 order by ur.uid limit 1")->fetchField();
t_ok($mgr > 0, 'fixture: a local letter manager exists');
$GLOBALS['user'] = user_load($mgr); drupal_static_reset('user_access');

function tg_letter($over) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => 9900001, 'l_event' => 'Custom', 'l_course_type' => 0,
    'l_name' => 'Zzqtest Letter', 'l_subject' => 'Zzqtest subject', 'l_body' => '<p>Body</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 777130, 'l_updated' => $now, 'l_updated_by' => 777130), $over))->execute();
}
function tg_row($id) { return db_query("select * from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc(); }
function tg_input($html, $id) { return preg_match('/<(input|select)[^>]*id="' . $id . '"[^>]*>/', $html, $m) ? $m[0] : ''; }

t_in_txn(function () {
  $lc = tg_letter(array('l_name' => 'Recommending AT Approved'));
  $rv = tg_letter(array('l_name' => 'Zzqtest Review', 'l_event' => 'R-ATReview', 'l_body' => '<p>Open [ReviewLink]</p>'));
  $off = tg_letter(array('l_name' => 'Area AT Rejected', 'l_deleted' => 1));
  $ord = tg_letter(array('l_name' => 'Zzqtest <b>Ordinary</b>', 'l_event' => 'Received'));

  $_GET['q'] = 'letters/9900001';
  $html = dh_letters_listing();
  t_ok(strpos($html, 'lc-letter-notice') !== FALSE, 'L1 the Letters page has the LC notice box');
  t_ok(substr_count($html, 'lc-letter-badge') >= 3, 'L1 the three LC letters carry the label');
  t_ok(strpos($html, "letters/9900001/edit/$lc") !== FALSE, 'L2 an LC letter can still be edited');
  t_ok(strpos($html, "letters/9900001/copy/$lc") === FALSE && strpos($html, "letters/9900001/delete/$lc") === FALSE, 'L2 no Copy or Mark Inactive for an LC letter');
  t_ok(strpos($html, "letters/9900001/restore/$off") === FALSE, 'L2 no Mark Active for an inactive LC letter');
  t_ok(strpos($html, "letters/9900001/copy/$ord") !== FALSE && strpos($html, "letters/9900001/delete/$ord") !== FALSE, 'L3 an ordinary letter keeps Copy and Mark Inactive');
  t_ok(strpos($html, 'Zzqtest &lt;b&gt;Ordinary&lt;/b&gt;') !== FALSE, 'L3 letter names are shown as text');

  // L4: the Subject, Status and Course Type cells are shown as text, for active and inactive letters
  $tdid = (int) db_insert('dh_type_detail')->fields(array('td_type' => 'zzqtest', 'td_key' => 'Zzqtest <b>Kind</b>', 'td_val1' => 'Zzqtest <b>Kind</b>',
    'td_val2' => 0, 'td_val3' => '', 'td_val4' => '', 'td_active' => 1))->execute();
  tg_letter(array('l_name' => 'Zzqtest Subj Active', 'l_subject' => 'Zzqtest <i>Subj</i>', 'l_event' => 'Zzqtest <s>Ev</s>', 'l_course_type' => $tdid));
  tg_letter(array('l_name' => 'Zzqtest Subj Inactive', 'l_subject' => 'Zzqtest <u>Gone</u>', 'l_event' => 'Zzqtest <em>Old</em>', 'l_course_type' => $tdid, 'l_deleted' => 1));
  $_GET['q'] = 'letters/9900001';
  $html = dh_letters_listing();
  t_ok(strpos($html, 'Zzqtest &lt;i&gt;Subj&lt;/i&gt;') !== FALSE && strpos($html, 'Zzqtest <i>Subj</i>') === FALSE, 'L4 an active letter Subject is shown as text');
  t_ok(strpos($html, 'Zzqtest &lt;s&gt;Ev&lt;/s&gt;') !== FALSE && strpos($html, 'Zzqtest <s>Ev</s>') === FALSE, 'L4 an active letter Status is shown as text');
  t_ok(strpos($html, 'Zzqtest &lt;u&gt;Gone&lt;/u&gt;') !== FALSE && strpos($html, 'Zzqtest <u>Gone</u>') === FALSE, 'L4 an inactive letter Subject is shown as text');
  t_ok(strpos($html, 'Zzqtest &lt;em&gt;Old&lt;/em&gt;') !== FALSE && strpos($html, 'Zzqtest <em>Old</em>') === FALSE, 'L4 an inactive letter Status is shown as text');
  t_ok(substr_count($html, 'Zzqtest &lt;b&gt;Kind&lt;/b&gt;') >= 2 && strpos($html, 'Zzqtest <b>Kind</b>') === FALSE, 'L4 the Course Type is shown as text, active and inactive');

  $_GET['q'] = "letters/9900001/edit/$lc";
  $f = drupal_get_form('dh_letters_form', tg_row($lc)); $html = drupal_render($f);
  t_ok(strpos($html, 'lc-letter-notice') !== FALSE, 'E1 the LC edit page has the purpose box');
  t_ok(strpos(tg_input($html, 'edit-l-name'), 'disabled') !== FALSE && strpos(tg_input($html, 'edit-l-event'), 'disabled') !== FALSE
    && strpos(tg_input($html, 'edit-l-course-type'), 'disabled') !== FALSE, 'E1 name, status and course type are read-only');
  t_ok(strpos(tg_input($html, 'edit-l-subject'), 'disabled') === FALSE, 'E1 the subject is editable');

  $_GET['q'] = "letters/9900001/edit/$rv";
  $f = drupal_get_form('dh_letters_form', tg_row($rv)); $html = drupal_render($f);
  t_ok(strpos($html, '>[ReviewLink]</a>') !== FALSE, 'E2 a review letter offers the [ReviewLink] chip');
  t_ok(strpos($html, 'Keep [ReviewLink]') !== FALSE, 'E2 and says to keep it');

  $_GET['q'] = "letters/9900001/edit/$ord";
  $f = drupal_get_form('dh_letters_form', tg_row($ord)); $html = drupal_render($f);
  t_ok(strpos($html, 'lc-letter-notice') === FALSE && strpos(tg_input($html, 'edit-l-name'), 'disabled') === FALSE, 'E3 an ordinary letter is unchanged');

  $_GET['q'] = "letters/9900001/view/$lc";
  t_ok(strpos(dh_letters_view(tg_row($lc)), 'lc-letter-badge') !== FALSE, 'V1 the view page shows the label');
});
t_done();
