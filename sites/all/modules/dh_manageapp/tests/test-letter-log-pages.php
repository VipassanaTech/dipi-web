<?php
// The letter Change Log on the Letters page, the letter view page and the letter edit page:
// shown to letter managers only. Made-up letter of fake centre 9900001, rolled back.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-log-pages.php
require __DIR__ . '/lib.php';

function tp_as($uid) { $GLOBALS['user'] = user_load($uid); drupal_static_reset('user_access'); }
function tp_uid($perm, $without = NULL) {
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
    'l_created' => $now, 'l_created_by' => 777061, 'l_updated' => $now, 'l_updated_by' => 777061))->execute();
  db_update('dh_letter')->fields(array('l_subject' => 'Zzqtest subject 2', 'l_updated_by' => 777062))->condition('l_id', $id)->execute();
  $row = db_query("select * from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc();
  $logged = 'class="cl-val">Zzqtest subject 2';   // the new subject inside the log table, not the letters table

  tp_as(tp_uid('manage letters'));
  $_GET['q'] = 'letters/9900001';
  $html = dh_letters_listing();
  t_ok(strpos($html, 'letter-change-log') !== FALSE && strpos($html, $logged) !== FALSE, 'P1 the Letters page shows the Change Log to a letter manager');
  t_ok(strpos($html, 'letter-change-log') > strpos($html, 'table-letters-d'), 'P1 below the Inactive Letters table');
  $_GET['q'] = "letters/9900001/view/$id";
  t_ok(strpos(dh_letters_view($row), $logged) !== FALSE, 'P2 the letter view page shows its log');
  $_GET['q'] = "letters/9900001/edit/$id";
  $form = drupal_get_form('dh_letters_form', $row);
  $html = drupal_render($form);
  t_ok(strpos($html, 'letter-change-log') !== FALSE && strpos($html, $logged) !== FALSE, 'P3 the letter edit page shows its log');
  t_ok(strpos($html, 'letter-change-log') > strpos($html, 'edit-sub'), 'P3 below the save button');

  tp_as(tp_uid('view letters', 'manage letters'));
  $_GET['q'] = 'letters/9900001';
  t_ok(strpos(dh_letters_listing(), 'letter-change-log') === FALSE, 'P4 someone who can only read letters sees the page without the log');
  $_GET['q'] = "letters/9900001/view/$id";
  t_ok(strpos(dh_letters_view($row), 'letter-change-log') === FALSE, 'P5 nor on the view page');
});
t_done();
