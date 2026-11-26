<?php
// The automatic LC letters have their own status, LC Automatic (owner, 26 Nov 2026; inc/letter-protect.inc,
// inc/lc-auto-status.inc). It is not an applicant status: no status list shows it. Because no application has it,
// these letters are never offered as a Custom letter (search page popup, bulk email) and a send by status never takes
// them; DIPI sends them by name. Only they may have it and nobody can change it. Bulk email lists active Custom letters
// only, and a bulk mail whose letter cannot be sent any more stops on the send day. Made-up data, rolled back.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-custom-lists.php
require __DIR__ . '/lib.php';
require_once __DIR__ . '/../inc/lc-auto-status.inc';   // the one-time move (the site does not load it)

$mgr = (int) db_query("select ur.uid from users_roles ur join role_permission p on p.rid = ur.rid join users u on u.uid = ur.uid
  where p.permission = 'manage letters' and u.status = 1 and ur.uid > 1 order by ur.uid limit 1")->fetchField();
$GLOBALS['user'] = user_load($mgr); drupal_static_reset('user_access');
$AUTO = DH_LETTER_LC_AUTO_STATUS;

function tl_letter($centre, $name, $event = 'Custom', $over = array()) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => $centre, 'l_event' => $event, 'l_course_type' => 0,
    'l_name' => $name, 'l_subject' => "$name subject", 'l_body' => '<p>Body</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 0, 'l_updated' => $now, 'l_updated_by' => 0), $over))->execute();
}
function tl_names($list) { $out = array(); foreach ($list as $x) $out[] = $x['status']; return $out; }
function tl_ev($id) { return db_query("select l_event from dh_letter where l_id = :l", array(':l' => $id))->fetchField(); }
// The select box with that id (Drupal adds --2 when a second form of the same kind is built in one run).
function tl_input($html, $id) { return preg_match('/<select[^>]*id="' . $id . '(--\d+)?"[^>]*>.*?<\/select>/s', $html, $m) ? $m[0] : ''; }

echo "LC Automatic is a letter category, not an applicant status\n";
t_eq($AUTO, 'LC Automatic', 'the status is "LC Automatic"');
t_eq(count(dh_letter_lc_names()), 8, 'it is for the eight LC letters DIPI finds by name (not the two review letters)');
t_eq((int) db_query("select count(*) from dh_type_detail where td_key = :s or td_val1 = :s", array(':s' => $AUTO))->fetchField(), 0,
  'it is not in the status table, so no applicant status list (search filter, status choice, LC statuses) shows it');
$_GET['q'] = 'letters/9900001/add';
$f = drupal_get_form('dh_letters_form'); $html = drupal_render($f);
t_ok(tl_input($html, 'edit-l-event') !== '' && strpos(tl_input($html, 'edit-l-event'), $AUTO) === FALSE, 'a new letter cannot choose it');

t_in_txn(function () use ($AUTO) {
  echo "The one-time move (deploy/2026-11-26-lc-automatic-status.php)\n";
  $c = t_make_centre();
  $a1 = tl_letter($c, 'Area AT Approved');
  $a2 = tl_letter($c, 'LC Admin Notification', 'Custom', array('l_deleted' => 1));
  $a3 = tl_letter($c, 'Recommending AT Rejected', 'Rejected');
  $a4 = tl_letter($c, 'recommending at changed  ');
  $rv = tl_letter($c, 'Area AT Approved', 'R-ATReview');               // a review status decides: this is the review letter
  $real = tl_letter($c, 'Welcome back');
  $mine = function () use ($c) { return array_values(array_filter(dh_lc_auto_targets(), function ($r) use ($c) { return (int) $r['l_center'] === $c; })); };
  t_eq(array_map(function ($r) { return (int) $r['l_id']; }, $mine()), array($a1, $a2, $a3, $a4), 'finds the LC letters by name: Custom, inactive, Rejected, other capitals/end spaces');
  $backup = dh_lc_auto_targets();
  $n = dh_lc_auto_apply($backup, dh_system_uid());
  t_ok($n >= 4, "moved $n letters");
  t_eq(array(tl_ev($a1), tl_ev($a2), tl_ev($a3), tl_ev($a4)), array($AUTO, $AUTO, $AUTO, $AUTO), 'all four are LC Automatic');
  t_eq(array(tl_ev($rv), tl_ev($real)), array('R-ATReview', 'Custom'), 'the review letter and the ordinary letter are untouched');
  t_eq(count($mine()), 0, 'a second run finds nothing');
  $log = db_query("select ll_old, ll_new, ll_user from dh_letter_log where ll_record = :l and ll_field = 'l_event' order by ll_id desc limit 1", array(':l' => $a3))->fetchAssoc();
  t_eq($log, array('ll_old' => 'Rejected', 'll_new' => $AUTO, 'll_user' => (string) dh_system_uid()), 'the Letter Change Log shows Rejected -> LC Automatic by System');
  dh_lc_auto_undo($backup, dh_system_uid());
  t_eq(array(tl_ev($a1), tl_ev($a2), tl_ev($a3), tl_ev($a4)), array('Custom', 'Custom', 'Rejected', 'Custom'), 'the undo puts the old statuses back');
});

t_in_txn(function () use ($AUTO) {
  $centre = t_make_centre();
  $other = t_make_centre();
  $lc_c = tl_letter($centre, 'Area AT Approved', $AUTO);
  $lc_n = tl_letter($centre, 'LC Admin Notification', $AUTO);
  $real = tl_letter($centre, 'Welcome back');
  $real2 = tl_letter($centre, 'Course timings');
  $off = tl_letter($centre, 'Old notice', 'Custom', array('l_deleted' => 1));
  tl_letter($centre, 'Area AT Rejected', $AUTO);
  tl_letter($centre, 'Rejected Common', 'Rejected');
  tl_letter($centre, 'Recommending Review', 'R-ATReview', array('l_body' => '<p>Open [ReviewLink]</p>'));
  tl_letter($other, 'Elsewhere letter');

  echo "After the move: the status popup on the search page\n";
  $pop = dh_search_status_letters($centre);
  t_eq(tl_names($pop['Custom']), array('Course timings', 'Welcome back'), 'Custom offers the real Custom letters only');
  t_eq(tl_names($pop['Rejected']), array('Rejected Common'), 'Rejected offers the real Rejected letter only');
  t_eq(tl_names($pop['R-ATReview']), array('Recommending Review'), 'the review letter stays under its review status');
  $all = array(); foreach ($pop as $ev => $l) $all = array_merge($all, tl_names($l));
  t_ok(!in_array('Old notice', $all) && !in_array('Elsewhere letter', $all), 'no inactive letter, no letter of another centre');

  echo "Bulk email list\n";
  $bm = dh_bulk_mail_letter_options($centre);
  t_eq($bm, array('' => 'Choose', $real2 => 'Course timings', $real => 'Welcome back'), 'Choose, then the active Custom letters by name');
  t_eq(dh_bulk_mail_letter_options($other + 1000000), array('' => 'Choose'), 'a centre with no letters: just Choose');

  echo "Bulk email: a letter that cannot be sent any more\n";
  $foreign = (int) db_query("select l_id from dh_letter where l_center = :c", array(':c' => $other))->fetchField();
  t_eq(dh_bulk_mail_letter_problem($centre, $real), '', 'an active Custom letter: fine');
  t_eq(dh_bulk_mail_letter_problem($centre, $off), 'The letter "Old notice" is not active any more.', 'inactive: "is not active any more"');
  t_eq(dh_bulk_mail_letter_problem($centre, $lc_c), 'The letter "Area AT Approved" is not a Custom letter any more.', 'an LC Automatic letter: not a Custom letter');
  t_eq(dh_bulk_mail_letter_problem($centre, $foreign), 'The letter of this bulk mail does not exist any more.', 'another centre\'s letter = does not exist here');
  t_eq(dh_bulk_mail_letter_problem($centre, 999999999), 'The letter of this bulk mail does not exist any more.', 'a missing letter');
  $same = TRUE;
  foreach (db_query("select l_id from dh_letter where l_center = :c", array(':c' => $centre))->fetchCol() as $lid)
    if (isset($bm[$lid]) !== (dh_bulk_mail_letter_problem($centre, $lid) === '')) $same = FALSE;
  t_ok($same, 'the list offers exactly the letters the send day accepts');

  $mk = function ($letter, $processed = 0) use ($centre) {
    $now = date('Y-m-d H:i:s');
    return (int) db_insert('dh_bulk_mail')->fields(array('bm_name' => 'Zzqtest bm', 'bm_center' => $centre, 'bm_count' => 0, 'bm_query' => 'select 0 as id from dual where 0',
      'bm_criteria' => '{}', 'bm_letter' => $letter, 'bm_schedule_date' => date('Y-m-d 00:00:00'), 'bm_processed' => $processed, 'bm_deleted' => 0,
      'bm_created' => $now, 'bm_created_by' => 0, 'bm_updated' => $now, 'bm_updated_by' => 0))->execute();
  };
  $proc = function ($b) { return (int) db_query("select bm_processed from dh_bulk_mail where bm_id = :b", array(':b' => $b))->fetchField(); };
  $bad = $mk($off);
  ob_start(); $stopped = dh_bulk_mail_stop_if_letter_bad(array('bm_id' => $bad, 'bm_center' => $centre, 'bm_letter' => $off), 15, date('Y-m-d H:i:s')); $said = ob_get_clean();
  t_ok($stopped && $proc($bad) == DH_BULK_MAIL_LETTER_FAILED, 'send day, inactive letter: stopped, the bulk mail is Failed (5), nothing queued');
  t_ok(strpos($said, 'The letter "Old notice" is not active any more.') !== FALSE, 'the job prints the reason in plain text');
  $w = db_query("select variables from watchdog where type = 'BulkMail' order by wid desc limit 1")->fetchField();
  t_ok($w && strpos(implode(' ', unserialize($w)), (string) $bad) !== FALSE, 'and logs it (watchdog BulkMail)');
  $good = $mk($real);
  ob_start(); $stopped = dh_bulk_mail_stop_if_letter_bad(array('bm_id' => $good, 'bm_center' => $centre, 'bm_letter' => $real), 15, date('Y-m-d H:i:s')); ob_end_clean();
  t_ok(!$stopped && $proc($good) == 0, 'send day, active Custom letter: goes ahead as before');
  $lcbm = $mk($lc_n);
  ob_start(); $stopped = dh_bulk_mail_stop_if_letter_bad(array('bm_id' => $lcbm, 'bm_center' => $centre, 'bm_letter' => $lc_n), 15, date('Y-m-d H:i:s')); ob_end_clean();
  t_ok($stopped && $proc($lcbm) == DH_BULK_MAIL_LETTER_FAILED, 'send day, an LC Automatic letter (an old bulk mail): stopped too');

  $_GET['q'] = "bulk-mail/$centre/$bad/edit";
  dh_create_edit_bulk_mail_schedule('Zzqtest bm', 'a_id = 0', $real, date('Y-m-d'), array(), $bad);
  t_ok($proc($bad) == 0 && (int) db_query("select bm_letter from dh_bulk_mail where bm_id = :b", array(':b' => $bad))->fetchField() == $real,
    'a failed bulk mail saved again with a new letter is Pending again');
  $_GET['q'] = "bulk-mail/$centre/$good/edit";
  db_update('dh_bulk_mail')->fields(array('bm_processed' => 3))->condition('bm_id', $good)->execute();
  dh_create_edit_bulk_mail_schedule('Zzqtest bm', 'a_id = 0', $real, date('Y-m-d'), array(), $good);
  t_ok($proc($good) == 3, 'a muted bulk mail stays muted when edited (as before)');

  $_GET['q'] = "centre/$centre/bulk-mail-schedule";
  drupal_static_reset('drupal_add_js');
  dh_show_bulk_mail_schedule();
  $js = ''; foreach (drupal_add_js() as $k => $v) if (is_array($v) && isset($v['type']) && $v['type'] == 'inline') $js .= $v['data'];
  $jsu = str_replace('\\/', '/', $js);   // json_encode writes / as \/
  t_ok(strpos($jsu, 'Failed! Not sent.') !== FALSE && strpos($jsu, 'is not a Custom letter any more') !== FALSE, 'the schedule list shows Failed with the reason');
  t_ok(strpos($jsu, "bulk-mail/$centre/$lcbm/edit") !== FALSE && strpos($jsu, "bulk-mail/$centre/$lcbm/delete") !== FALSE && strpos($jsu, "bulk-mail/$centre/$lcbm/mute") === FALSE,
    'a failed bulk mail offers Edit and Delete (no Mute)');

  echo "A send by status never takes an LC Automatic letter\n";
  $course = t_make_course(array('c_center' => $centre));
  $app = t_make_applicant($course, array('a_email' => 'zzqtest@example.invalid'));   // dh_get_letter only builds the letter, it sends nothing
  $g = dh_get_letter('applicant', $app, 'Custom');
  t_eq($g['letter_name'], 'Course timings', 'Custom by status: the first real Custom letter');
  $g = dh_get_letter('applicant', $app, 'Rejected');
  t_eq($g['letter_name'], 'Rejected Common', 'Rejected by status: the real Rejected letter');
  $g = dh_get_letter('applicant', $app, 'Custom', $lc_n);
  t_eq($g['letter_name'], 'LC Admin Notification', 'an LC letter asked for by id (how the LC process sends it) still goes');
  t_eq(dh_letter_by_name($centre, 'Area AT Approved'), $lc_c, 'and the LC process still finds it by name');
  db_update('dh_letter')->fields(array('l_deleted' => 1))->condition('l_id', array($real, $real2), 'IN')->execute();
  $g = dh_get_letter('applicant', $app, 'Custom');
  t_eq($g['result'], FALSE, 'only LC Automatic letters left: no letter by status');
  $w = db_query("select variables from watchdog where type = 'LETTER' order by wid desc limit 1")->fetchField();
  $v = $w ? unserialize($w) : array();
  t_ok(isset($v['@a']) && strpos($v['@a'], '"Custom"') !== FALSE && strpos($v['@a'], (string) $centre) !== FALSE,
    'the "no letter" record still names the centre and status');

  echo "Nobody can change the status (locked)\n";
  list($f, $err) = dh_letter_protect($centre, 'edit', $lc_c, array('l_name' => 'Area AT Approved', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_subject' => 'x', 'l_body' => 'y'));
  t_ok($err === '' && $f['l_event'] === $AUTO, 'saving an LC letter with another status keeps LC Automatic');
  list(, $err) = dh_letter_protect($centre, 'add', 0, array('l_name' => 'My own letter', 'l_event' => $AUTO, 'l_course_type' => 0, 'l_subject' => 'x', 'l_body' => 'y'));
  t_ok(strpos($err, 'only for the automatic long-course letters') !== FALSE, 'a new letter cannot take LC Automatic');
  list(, $err) = dh_letter_protect($centre, 'edit', $real, array('l_name' => 'Welcome back', 'l_event' => ' lc automatic ', 'l_course_type' => 0, 'l_subject' => 'x', 'l_body' => 'y'));
  t_ok(strpos($err, 'only for the automatic long-course letters') !== FALSE, 'nor an existing one (capitals and spaces do not count)');
  list(, $err) = dh_letter_protect($centre, 'edit', $real, array('l_name' => 'Welcome back', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_subject' => 'x', 'l_body' => 'y'));
  t_eq($err, '', 'an ordinary letter saves as before');
  $_GET['q'] = "letters/$centre/edit/$lc_c";
  $row = db_query("select * from dh_letter where l_id = :l", array(':l' => $lc_c))->fetchAssoc();
  $f = drupal_get_form('dh_letters_form', $row); $html = drupal_render($f);
  $sel = tl_input($html, 'edit-l-event');
  t_ok(strpos($sel, 'disabled') !== FALSE && preg_match('/<option value="LC Automatic" selected="selected">LC Automatic<\/option>/', $sel) === 1,
    'the LC letter edit page shows LC Automatic, read-only');
  $_GET['q'] = '';
});

echo "Labels and the popup messages\n";
t_ok(strpos(dh_letter_lc_badge(), 'Automatic · LC review') !== FALSE, 'the label reads "Automatic · LC review"');
t_ok(strpos(dh_letter_lc_notice_list(), 'their status is LC Automatic') !== FALSE, 'the Letters page box explains the status');
$src = file_get_contents(__DIR__ . '/../inc/search.inc');
t_ok(preg_match('/if \(tmp == "\'\.\$system_statues\[\'STATUS-CUSTOM\'\]\.\'" && rd\.centreid == lettersCentre && !rd\.transferred\)\s*\{[^}]*?cavail[\s\S]{0,200}?\$\.alert\("There is no Custom letter for this course type\. Add one on the Letters page first\."\);\s*s\.val\(""\);\s*return;/', $src) === 1,
  'Custom with no Custom letter for the course type: the popup says so and resets the status');
t_ok(preg_match('/if \(s == "\'\.\$system_statues\[\'STATUS-CUSTOM\'\]\.\'" && !letter && !\$\("\.sub-letter-"\+id\)\.text\(\) && rdc\.centreid == lettersCentre && !rdc\.transferred\)\s*\{\s*alert\("Please choose the letter to send[^"]*"\);\s*return;/', $src) === 1,
  'Update with Custom and the letter list closed without a choice: asks for the letter, sends nothing');
t_ok(strpos($src, "l_event='custom'") === FALSE, 'the old bulk email query (inactive letters, centre in the SQL) is gone');
$cron = file_get_contents(__DIR__ . '/../../../../../cron-bulk-mail.php');
t_ok(preg_match('/\$bm_updated = date\([^;]*;\s*(\/\/[^\n]*\n\s*)*if \(dh_bulk_mail_stop_if_letter_bad\(\$row, \$bm_updated_by, \$bm_updated\)\)\s*continue;[\s\S]*dh_bulk_mail_enqueue\(/', $cron) === 1,
  'cron-bulk-mail.php checks the letter before the monthly limit and before anything is queued');
foreach (array('inc/search.inc', 'inc/letters.inc', 'inc/letter-coverage.inc', 'inc/bulk-mail.inc') as $file)
  t_ok(strpos(file_get_contents(__DIR__ . '/../' . $file), 'dh_letter_lc_names') === FALSE, "$file has no letter-name check (the status does the work)");
t_done();
