<?php
// AT Portal profile form: Country + State as lists (the State list holds the chosen country's states; the
// code is saved; State is required when the country has states). Made-up teacher, always rolled back.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-at-profile-state.php
require __DIR__ . '/lib.php';

$in_states = db_query("select s_code, s_name from dh_state where s_country = 'IN' order by s_name")->fetchAllKeyed();
$np_code = db_query("select s_code from dh_state where s_country = 'NP' and s_code not in (select s_code from dh_state where s_country = 'IN') limit 1")->fetchField();
$none = db_query("select c_code from dh_country c where not exists (select 1 from dh_state s where s.s_country = c.c_code) order by c_code limit 1")->fetchField();
$mh = array_search('Maharashtra', $in_states);

// S1-S3 the helpers
t_ok(count($in_states) > 30 && $mh !== FALSE, 'S0 test data: India has its states locally, Maharashtra among them');
t_eq(dh_atportal_states('IN'), $in_states, 'S1 the states of a country, code => name, in name order');
t_eq(array(dh_atportal_states($none), dh_atportal_states('0'), dh_atportal_states(''), dh_atportal_states("IN' or '1'='1")), array(array(), array(), array(), array()),
  "S2 no list for a country without states ($none), for Choose, for nothing, or for a value that is not a country code");
t_eq(array(dh_atportal_state_default('IN', $mh), dh_atportal_state_default('IN', strtolower($mh)), dh_atportal_state_default('IN', 'Maharashtra'),
    dh_atportal_state_default('IN', ' maharashtra '), dh_atportal_state_default('IN', 'Zzqland'), dh_atportal_state_default('IN', ''), dh_atportal_state_default('IN', $np_code)),
  array($mh, $mh, $mh, $mh, '', '', ''),
  'S3 chosen state for a saved value: the code, a typed name of the same state, or nothing (unknown text / another country\'s code)');

$keep_user = $GLOBALS['user']; $keep_q = $_GET['q'];
t_in_txn(function () use ($in_states, $np_code, $none, $mh) {
  global $user;
  $rid = db_query("select rid from role where name = 'AT Portal'")->fetchField();
  $base = db_query("select * from dh_teacher order by t_id desc limit 1")->fetchAssoc(); unset($base['t_id']);
  $mk = function ($code, $country, $state) use ($base) {
    return (int) db_insert('dh_teacher')->fields(array_merge($base, array('t_code' => $code, 't_gender' => 'M', 't_status' => 'Active', 't_f_name' => 'Zzqtest',
      't_l_name' => 'State', 't_country' => $country, 't_state' => $state, 't_email' => 'zzqtest@example.invalid', 't_email_alt' => '', 't_address' => 'Zzq street',
      't_mob_phone' => '', 't_mob_phone_alt' => '', 't_photo' => NULL)))->execute();
  };
  $form = function ($tid, $input = array()) {
    $_GET['q'] = "at-portal/address-book/edit/$tid";
    $edit = db_query("select * from dh_teacher where t_id = :t", array(':t' => $tid))->fetchAssoc();
    $fs = form_state_defaults(); $fs['build_info']['args'] = array($edit); $fs['input'] = $input;
    $f = drupal_retrieve_form('dh_atportal_profile', $fs);
    return $f['Address'];
  };
  $user = (object) array('uid' => 999999995, 'name' => 'zzq1.m', 'roles' => array(2 => 'authenticated user', $rid => 'AT Portal'));
  drupal_static_reset('user_access');
  t_ok(!user_access('add assistant teacher') && user_access('access at profile'), 'S4a acting as an ordinary AT');

  // S4 a saved code: list of the country's states, that state chosen, required
  $t1 = $mk('ZZQ1', 'IN', $mh);
  $a = $form($t1);
  t_eq(array($a['t_state']['#type'], $a['t_state']['#options'], $a['t_state']['#default_value'], $a['t_state']['#required'], $a['t_state']['#empty_option'], isset($a['t_state']['#description'])),
    array('select', $in_states, $mh, TRUE, '- Choose state -', FALSE), 'S4 saved code: a list of India\'s states, Maharashtra chosen, required, no hint');
  t_eq(key($a['t_country']['#options']), '0', 'S4b "Choose" is now the first country');

  // S5 a typed name of a real state (old saved names are at most 10 letters): that state is chosen
  $short = ''; foreach ($in_states as $c => $n) if (strlen($n) <= 10 && strlen($n) > 3) { $short = $c; break; }
  $t2 = $mk('ZZQ2', 'IN', strtolower($in_states[$short]));
  $a = $form($t2);
  t_eq(array($a['t_state']['#default_value'], isset($a['t_state']['#description'])), array($short, FALSE),
    'S5 typed name "' . strtolower($in_states[$short]) . '" from the old box: that state (' . $short . ') is chosen');

  // S6 text that matches no state: nothing chosen, a hint shows what was saved
  $t3 = $mk('ZZQ3', 'IN', 'Zzqland');
  $a = $form($t3);
  t_ok($a['t_state']['#default_value'] === '' && strpos($a['t_state']['#description'], 'Saved earlier') !== FALSE && strpos($a['t_state']['#description'], 'Zzqland') !== FALSE,
    'S6 unknown saved text: nothing chosen; hint "Saved earlier: Zzqland. Please choose your state from the list."');

  // S7 a country with no states listed, and "Choose": empty list, not required
  $t4 = $mk('ZZQ4', $none, '');
  $a = $form($t4);
  t_eq(array($a['t_state']['#options'], $a['t_state']['#required'], $a['t_state']['#empty_option']), array(array(), FALSE, 'No states listed for this country'),
    "S7 country $none (no states listed): empty, not required");
  $t5 = $mk('ZZQ5', '0', '');
  $a = $form($t5);
  t_eq(array($a['t_state']['#options'], $a['t_state']['#required'], $a['t_state']['#empty_option']), array(array(), FALSE, 'Choose the country first'),
    'S7b no country chosen: "Choose the country first", not required');

  // S8 the country was changed on the page: the list sent back is the new country's
  $np = db_query("select s_code, s_name from dh_state where s_country = 'NP' order by s_name")->fetchAllKeyed();
  $a = $form($t1, array('t_country' => 'NP'));
  t_eq(array($a['t_state']['#options'], $a['t_state']['#default_value'], $a['t_state']['#required']), array($np, '', TRUE),
    'S8 country changed to Nepal before saving: Nepal\'s states, none chosen yet, required');

  // S9 the server check (validate)
  $check = function ($country, $state) {
    form_clear_error(); drupal_static_reset('form_set_error');
    $fs = form_state_defaults(); $fs['input'] = array('t_country' => $country, 't_state' => $state);
    dh_atportal_profile_validate(array(), $fs);
    $e = form_get_errors();
    return isset($e['t_state']) ? strip_tags($e['t_state']) : '';
  };
  t_eq(array($check('IN', $mh), $check('IN', ''), $check($none, ''), $check('NP', $np_code)), array('', '', '', ''), 'S9 valid: a state of the country, or empty (required is checked by the field)');
  t_eq(array($check('IN', $np_code), $check('IN', 'Maharashtra'), $check('IN', str_repeat('X', 30))),
    array_fill(0, 3, 'State: please choose your state from the list.'), 'S9b refused with a message (no crash): another country\'s state, a typed name, a long value');
  form_clear_error();

  // S10 the JSON list for the page
  ob_start(); dh_atportal_states_json('IN'); $json = json_decode(ob_get_clean(), TRUE);
  t_ok(is_array($json) && count($json) === count($in_states) && $json[0] === array('id' => key($in_states), 'text' => reset($in_states)), 'S10 at-portal/states/IN gives India\'s states as {id: code, text: name}');
  ob_start(); dh_atportal_states_json("IN' union select 1,2 -- "); $bad = ob_get_clean();
  t_eq($bad, '[]', 'S10b a value that is not a country code gives an empty list');

  // S11 a real save through Drupal's form handling, as the AT
  $submit = function ($tid, $state) {
    $_GET['q'] = "at-portal/address-book/edit/$tid";
    form_clear_error(); drupal_static_reset('form_set_error'); drupal_get_messages();
    $edit = db_query("select * from dh_teacher where t_id = :t", array(':t' => $tid))->fetchAssoc();
    $fs = array('values' => array('t_country' => 'IN', 't_state' => $state, 't_address' => 'Zzq street 2', 'op' => 'Submit'));
    $fs['build_info']['args'] = array($edit);
    drupal_form_submit('dh_atportal_profile', $fs, $edit);
    $e = form_get_errors();
    return array(db_query("select t_state from dh_teacher where t_id = :t", array(':t' => $tid))->fetchField(), $e ? implode(' | ', array_map('strip_tags', $e)) : '');
  };
  list($saved, $err) = $submit($t3, '');
  t_ok($saved === 'Zzqland' && strpos($err, 'State field is required') !== FALSE, 'S11 save without a state (India): refused "State field is required", nothing changed' . ($err ? '' : ' — no error'));
  list($saved, $err) = $submit($t3, $mh);
  t_ok($saved === $mh && $err === '', 'S11b save with Maharashtra chosen: saved as its code' . ($err ? " — $err" : ''));
});
$GLOBALS['user'] = $keep_user; $_GET['q'] = $keep_q;
t_ok((int) db_query("select count(*) from dh_teacher where t_code like 'ZZQ%' and t_f_name = 'Zzqtest'")->fetchField() === 0, 'S12 made-up teachers rolled back');

$items = dh_atportal_menu();
t_eq(isset($items['at-portal/states/%']) ? array($items['at-portal/states/%']['page callback'], $items['at-portal/states/%']['access arguments']) : NULL,
  array('dh_atportal_states_json', array('access at profile')), 'S13 route at-portal/states/% for everyone who can use the profile form');
t_done();
