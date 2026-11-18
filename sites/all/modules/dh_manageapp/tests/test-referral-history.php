<?php
// Referral history: escaped output, access only for people who can edit the referral, r-only toggle logged.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-history.php
require __DIR__ . '/lib.php';

$items = dh_manageapp_menu();
t_eq(array($items['referral/get-activity/%']['access callback'], $items['referral/get-activity/%']['access arguments']),
  array('dh_referral_can_edit', array(2)), 'R1 the history address checks can-edit for that referral');

t_in_txn(function () {
  $r = (int) db_insert('dh_referral')->fields(array('r_student' => 0, 'r_center' => 0, 'r_reason' => 'zzq', 'r_listed_by' => 'zzq',
    'r_start' => date('Y-m-d'), 'r_end' => date('Y-m-d', strtotime('+30 days')), 'r_created_by' => 0, 'r_updated_by' => 0, 'r_moved' => date('Y-m-d H:i:s')))->execute();
  add_referral_activity($r, 'Edited', "First Name: 'A' -> '<script>x</script>'");
  add_referral_activity($r, 'Created', 'Referral created');
  $html = dh_referral_history_html($r);
  t_ok(strpos($html, '<script>') === FALSE && strpos($html, '&lt;script&gt;') !== FALSE, 'H1 log text is escaped');
  t_ok(strpos($html, 'History') !== FALSE && strpos($html, 'Referral created') < strpos($html, 'First Name'), 'H2 headed History, newest first');
  t_eq(dh_referral_history_html(999999999), '<h4>History</h4><p>' . t('No history yet.') . '</p>', 'H3 nothing logged: a plain note');

  t_eq(dh_referral_set_readonly($r, 1), 'Added readonly flag to referral.', 'O1 r-only on: message');
  t_eq(db_query('select r_readonly from dh_referral where r_id = :r', array(':r' => $r))->fetchField(), 1, 'O2 r-only stored');
  dh_referral_set_readonly($r, 0);
  t_eq(db_query("select rl_msg from dh_referral_log where rl_referral = :r and rl_event = 'Edited' order by rl_id desc limit 2", array(':r' => $r))->fetchCol(),
    array('Read-only (r-only): Yes -> No', 'Read-only (r-only): No -> Yes'), 'O3 both toggles logged');
  t_eq(dh_referral_set_readonly($r, 0), 'Removed readonly flag from referral.', 'O4 setting the same value again: message, no new line');
  t_eq((int) db_query("select count(*) from dh_referral_log where rl_referral = :r and rl_msg like 'Read-only%'", array(':r' => $r))->fetchField(), 2, 'O5 still two lines');
});
t_done();
