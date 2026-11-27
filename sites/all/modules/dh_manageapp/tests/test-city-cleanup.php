<?php
// One-time city clean-up (owner, 27 Nov 2026; inc/city-cleanup.inc). Made-up cities in the made-up country ZZ and
// made-up records, rolled back. Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-city-cleanup.php
require __DIR__ . '/lib.php';
require_once __DIR__ . '/../inc/city-cleanup.inc';

function tcc_city($name, $state = 'Q1') {
  return (int) db_insert('dh_city')->fields(array('c_country' => 'ZZ', 'c_state' => $state, 'c_name' => $name))->execute();
}
function tcc_name($id) { return db_query("select c_name from dh_city where c_id = :i", array(':i' => $id))->fetchField(); }
function tcc_val($t, $col, $idc, $id) { return (int) db_query("select $col from $t where $idc = :i", array(':i' => $id))->fetchField(); }

t_eq(dh_city_clean_name('Alcarr&agrave;s'), 'Alcarràs', 'an HTML code becomes its letter');
t_eq(dh_city_clean_name("Connah\\&#039;s Quay"), "Connah's Quay", 'a backslash and a coded apostrophe');
t_eq(dh_city_clean_name("St-Andr&eacute; D\\'argenteuil"), "St-André D'argenteuil", 'a backslash before an apostrophe');
t_eq(dh_city_clean_name('L&amp;#039;Isle'), "L'Isle", 'a name coded twice');
t_eq(dh_city_clean_name("  Cox's   Bazar "), "Cox's Bazar", 'extra spaces');
t_eq(dh_city_clean_name("Gordon's Bay"), "Gordon's Bay", 'a clean name stays as it is');

t_in_txn(function () {
  $sys = dh_system_uid();
  // one city five times, written five ways; the oldest is all small letters
  $c1 = tcc_city("cox's bazar"); $c2 = tcc_city("Cox's Bazar"); $c3 = tcc_city("COX'S BAZAR"); $c4 = tcc_city("Cox's Bazar "); $c5 = tcc_city("Cox's bazar");
  $other_state = tcc_city("Cox's Bazar", 'Q2');
  $nospace = tcc_city("Cox'sBazar");
  $alone = tcc_city("Gordon's Bay");
  $ent = tcc_city('Alcarr&agrave;s');
  $m_clean = tcc_city('Montréal'); $m_ent = tcc_city('Montr&eacute;al');
  $q = array(tcc_city("Connah\\&#039;s Quay"), tcc_city("Connah\\&#039;s Quay"), tcc_city("Connah\\&#039;s Quay"));
  $sp = tcc_city('Sao Paulo'); $sp_ent = tcc_city('S&atilde;o Paulo');

  $plan = dh_city_cleanup_plan('ZZ');
  $by = array(); foreach ($plan as $g) $by[$g['keep']] = $g;
  t_eq(array_keys($by), array($c1, $ent, $m_clean, $q[0], $sp), 'five cities to fix, each kept as its oldest row');
  t_eq($by[$c1]['drop'], array($c2, $c3, $c4, $c5), 'all copies of one city, whatever the capitals or end spaces');
  t_eq($by[$c1]['name'], "Cox's Bazar", 'the name most rows use, not all small or all capital letters');
  t_ok(!isset($by[$other_state]) && !in_array($other_state, $by[$c1]['drop']), 'the same name in another state is another city');
  t_ok(!isset($by[$nospace]) && !in_array($nospace, $by[$c1]['drop']), 'a different spelling (no space) is left as it is');
  t_ok(!isset($by[$alone]), 'a city with an apostrophe and no copies is left as it is');
  t_eq(array($by[$ent]['drop'], $by[$ent]['name']), array(array(), 'Alcarràs'), 'a coded name alone: only the name is fixed');
  t_eq(array($by[$m_clean]['drop'], $by[$m_clean]['name']), array(array($m_ent), 'Montréal'), 'a coded name joins the city that already has the clean name');
  t_eq(array($by[$q[0]]['drop'], $by[$q[0]]['name']), array(array($q[1], $q[2]), "Connah's Quay"), 'copies that are all coded: one row, clean name');
  t_eq(array($by[$sp]['drop'], $by[$sp]['name']), array(array($sp_ent), 'São Paulo'), 'the database finds Sao Paulo for São Paulo, so they are one city (tie: the name with accents)');

  // records that point at the copies
  $course = t_make_course();
  $app1 = t_make_applicant($course, array('a_city' => $c3));
  $app2 = t_make_applicant($course, array('a_city' => $q[2]));
  $app3 = t_make_applicant($course, array('a_city' => $c1));
  $stu = t_make_student(array('s_city' => $c4));
  $pat = (int) db_insert('dh_patrika')->fields(array('p_student' => $stu, 'p_city' => $m_ent, 'p_updated' => '2020-01-01 00:00:00', 'p_updated_by' => 0, 'p_created' => '2020-01-01 00:00:00'))->execute();
  $pin = (int) db_insert('dh_pin_code')->fields(array('pc_pin' => 'ZZ999', 'pc_city' => $c2, 'pc_state_str' => 'Zz', 'pc_city_str' => 'Zz'))->execute();
  $cen = t_make_centre(); db_update('dh_center')->fields(array('c_city' => $c5))->condition('c_id', $cen)->execute();
  $ref = (int) db_insert('dh_referral')->fields(array('r_student' => $stu, 'r_city' => $sp_ent, 'r_reason' => 'Zzqtest', 'r_start' => '2020-01-01', 'r_end' => '2020-01-02', 'r_created_by' => 0))->execute();
  $la = (int) db_insert('dh_lc_admin')->fields(array('la_user' => 0, 'la_city' => $q[1], 'la_created_by' => 0, 'la_updated_by' => 0))->execute();

  $snap = dh_city_cleanup_snapshot($plan);
  $n = array(); foreach ($snap['refs'] as $t => $list) $n[$t] = count($list);
  t_eq(array_intersect_key($n, array_flip(array('dh_applicant', 'dh_student', 'dh_patrika', 'dh_pin_code', 'dh_center', 'dh_referral', 'dh_lc_admin'))),
    array('dh_applicant' => 2, 'dh_student' => 1, 'dh_patrika' => 1, 'dh_pin_code' => 1, 'dh_center' => 1, 'dh_referral' => 1, 'dh_lc_admin' => 1), 'the backup holds every record that points at a copy');
  t_eq(count($snap['cities']), 13, 'and the 5 kept and 8 removed city rows');
  $backup = json_decode(json_encode(array('plan' => $plan, 'snap' => $snap)), TRUE);   // as the script writes and reads it

  $done = dh_city_cleanup_apply($plan, $sys);
  t_eq(array($done['deleted'], $done['renamed']), array(8, 4), '8 copies removed, 4 names fixed');
  t_eq(array(tcc_val('dh_applicant', 'a_city', 'a_id', $app1), tcc_val('dh_applicant', 'a_city', 'a_id', $app2), tcc_val('dh_applicant', 'a_city', 'a_id', $app3)), array($c1, $q[0], $c1), 'applications point at the kept city');
  t_eq(array(tcc_val('dh_student', 's_city', 's_id', $stu), tcc_val('dh_patrika', 'p_city', 'p_id', $pat), tcc_val('dh_pin_code', 'pc_city', 'pc_id', $pin),
    tcc_val('dh_center', 'c_city', 'c_id', $cen), tcc_val('dh_referral', 'r_city', 'r_id', $ref), tcc_val('dh_lc_admin', 'la_city', 'la_id', $la)),
    array($c1, $m_clean, $c1, $c1, $sp, $q[0]), 'student, Patrika, PIN code, centre, referral and LC admin rows too');
  t_eq(tcc_val('dh_patrika', 'p_updated_by', 'p_id', $pat), $sys, 'the Patrika row says System changed it');
  t_eq((int) db_query("select pl_user from dh_patrika_log where pl_patrika = :p order by pl_id desc limit 1", array(':p' => $pat))->fetchField(), $sys, 'and the Patrika log gives System');
  $drops = array_merge($by[$c1]['drop'], $by[$m_clean]['drop'], $by[$q[0]]['drop'], $by[$sp]['drop']);
  t_eq((int) db_query("select count(*) from dh_city where c_id in (:i)", array(':i' => $drops))->fetchField(), 0, 'the copies are gone');
  t_eq(array(tcc_name($c1), tcc_name($ent), tcc_name($q[0]), tcc_name($m_clean), tcc_name($sp)), array("Cox's Bazar", 'Alcarràs', "Connah's Quay", 'Montréal', 'São Paulo'), 'the kept cities have clean names');
  $find = function ($n) { return (int) db_query("select c_id from dh_city where c_country = 'ZZ' and c_state = 'Q1' and c_name = :n limit 1", array(':n' => $n))->fetchField(); };
  t_eq(array($find("COX'S BAZAR"), $find("Connah's Quay"), $find('São Paulo')), array($c1, $q[0], $sp), 'a new application finds the one kept city');
  t_eq(array(tcc_name($other_state), tcc_name($nospace), tcc_name($alone)), array("Cox's Bazar", "Cox'sBazar", "Gordon's Bay"), 'other cities are untouched');
  t_eq(dh_city_cleanup_plan('ZZ'), array(), 'a second run finds nothing');

  // undo: a record and a name someone changed since stay as they are
  db_update('dh_applicant')->fields(array('a_city' => $alone))->condition('a_id', $app2)->execute();
  db_update('dh_city')->fields(array('c_name' => 'Alcarras Changed'))->condition('c_id', $ent)->execute();
  $r = dh_city_cleanup_undo($backup);
  t_eq($r, array('cities_back' => 8, 'names_back' => 3, 'refs_back' => 7, 'kept' => 2), 'undo: 8 cities, 3 names and 7 records back; 2 changed since are kept');
  t_eq(array(tcc_name($c1), tcc_name($c3), tcc_name($q[1]), tcc_name($ent), tcc_name($sp)), array("cox's bazar", "COX'S BAZAR", "Connah\\&#039;s Quay", 'Alcarras Changed', 'Sao Paulo'), 'old names back with their own ids');
  t_eq(array(tcc_val('dh_applicant', 'a_city', 'a_id', $app1), tcc_val('dh_applicant', 'a_city', 'a_id', $app2), tcc_val('dh_student', 's_city', 's_id', $stu),
    tcc_val('dh_patrika', 'p_city', 'p_id', $pat), tcc_val('dh_pin_code', 'pc_city', 'pc_id', $pin), tcc_val('dh_center', 'c_city', 'c_id', $cen),
    tcc_val('dh_referral', 'r_city', 'r_id', $ref), tcc_val('dh_lc_admin', 'la_city', 'la_id', $la)),
    array($c3, $alone, $c4, $m_ent, $c2, $c5, $sp_ent, $q[1]), 'records point at their old city again, except the one changed since');
  t_eq(db_query("select p_updated from dh_patrika where p_id = :p", array(':p' => $pat))->fetchField(), '2020-01-01 00:00:00', 'the Patrika row gets its old updated time back');
  t_eq(count(dh_city_cleanup_plan('ZZ')), 4, 'after the undo the four cities with copies need fixing again');
});
t_done();
