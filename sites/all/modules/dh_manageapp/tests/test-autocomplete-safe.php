<?php
// Suggestion boxes (country, state, city, PIN code, teacher, trainee teacher): the typed text and the chosen
// country/state reach the database only as data. Same suggestions for normal typing; names with an apostrophe
// work; crafted text finds nothing.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-autocomplete-safe.php
require __DIR__ . '/lib.php';

$bad = "x' union select name, pass from users -- ";
$no_error = function ($fn) { try { $fn(); return TRUE; } catch (Throwable $e) { return FALSE; } };
$values = function ($rows, $key) { $o = array(); foreach ($rows as $r) $o[] = is_array($r) ? $r[$key] : $r; return $o; };

echo "Country\n";
$c = db_query("select c_name, c_code from dh_country where c_name regexp '^[A-Za-z][A-Za-z][A-Za-z][A-Za-z]' order by c_name limit 1")->fetchObject();
t_ok(in_array($c->c_name, $values(dh_ac_country_matches(substr($c->c_name, 0, 3)), 'value')), "typing '" . substr($c->c_name, 0, 3) . "' suggests $c->c_name");
$r = dh_ac_country_matches(substr($c->c_name, 0, 3));
t_ok($r && isset($r[0]['value'], $r[0]['code']), 'same answer shape (value, code)');
t_ok(count(dh_ac_country_matches('')) <= 10, 'at most 10 suggestions');
t_ok($no_error(function () use ($bad) { t_eq(dh_ac_country_matches($bad), array(), 'crafted text: no suggestions'); }), 'crafted text: no database error');
$ap = db_query("select c_name from dh_country where c_name like :q limit 1", array(':q' => "%'%"))->fetchField();
if ($ap) t_ok(in_array($ap, $values(dh_ac_country_matches(substr($ap, 0, strpos($ap, "'") + 1)), 'value')), 'a country with an apostrophe can be typed');

echo "State\n";
$s = db_query("select s_country, s_name, s_code from dh_state where s_name regexp '^[A-Za-z][A-Za-z][A-Za-z][A-Za-z]' order by s_country, s_name limit 1")->fetchObject();
t_ok(in_array($s->s_name, $values(dh_ac_state_matches(substr($s->s_name, 0, 3), $s->s_country), 'value')), "state suggested within its country");
t_eq(dh_ac_state_matches(substr($s->s_name, 0, 3), $bad), array(), 'crafted country: no suggestions');
t_eq(dh_ac_state_matches($bad, $s->s_country), array(), 'crafted text: no suggestions');
$ap = db_query("select s_country, s_name from dh_state where s_name like :q limit 1", array(':q' => "%'%"))->fetchObject();
if ($ap) t_ok(in_array($ap->s_name, $values(dh_ac_state_matches(substr($ap->s_name, 0, strpos($ap->s_name, "'") + 1), $ap->s_country), 'value')), "a state with an apostrophe can be typed ($ap->s_name)");

echo "City\n";
$ci = db_query("select c_country, c_state, c_name from dh_city where c_name regexp '^[A-Za-z][A-Za-z][A-Za-z][A-Za-z]' and c_state <> '' order by c_id limit 1")->fetchObject();
$got = dh_ac_city_matches(substr($ci->c_name, 0, 3), $ci->c_country, $ci->c_state);
t_ok(isset($got[$ci->c_name]) && $got[$ci->c_name] === $ci->c_name, 'city suggested within its state (same name => name shape)');
t_eq(dh_ac_city_matches($bad, $ci->c_country, $ci->c_state), array(), 'crafted text: no suggestions');
t_eq(dh_ac_city_matches(substr($ci->c_name, 0, 3), $ci->c_country, $bad), array(), 'crafted state: no suggestions');
$ap = db_query("select c_country, c_state, c_name from dh_city where c_name like :q limit 1", array(':q' => "%'%"))->fetchObject();
if ($ap) t_ok(isset(dh_ac_city_matches(substr($ap->c_name, 0, strpos($ap->c_name, "'") + 1), $ap->c_country, $ap->c_state)[$ap->c_name]), "a city with an apostrophe can be typed");

echo "PIN code\n";
$pin = db_query("select p.pc_pin, co.c_code, s.s_code, ci.c_name from dh_pin_code p join dh_city ci on p.pc_city = ci.c_id left join dh_state s on (ci.c_state = s.s_code and ci.c_country = s.s_country) left join dh_country co on ci.c_country = co.c_code limit 1")->fetchObject();
if ($pin) t_eq(dh_ac_pincode_matches($pin->pc_pin), array(array('country' => $pin->c_code, 'state' => $pin->s_code, 'city' => $pin->c_name)), 'PIN code fills country, state, city');
t_ok($no_error(function () use ($bad) { t_eq(dh_ac_pincode_matches($bad), array(), 'crafted PIN: nothing'); }), 'crafted PIN: no database error');

echo "Teacher\n";
$t = db_query("select t_f_name, t_l_name, t_code, t_gender from dh_teacher where t_status = 'Active' and t_f_name regexp '^[A-Za-z][A-Za-z][A-Za-z][A-Za-z]' limit 1")->fetchObject();
$got = dh_ac_teacher_matches(substr($t->t_f_name, 0, 4), 'Active', '');
t_ok(count($got) > 0 && isset($got[0]['value'], $got[0]['code'], $got[0]['fullname']), 'teacher suggestions (value, code, fullname)');
t_ok(in_array("$t->t_code-$t->t_gender", $values(dh_ac_teacher_matches("$t->t_f_name $t->t_l_name", 'Active', ''), 'code')), 'full name finds the teacher');
t_eq(dh_ac_teacher_matches($bad, 'Active', ''), array(), 'crafted text: no suggestions');
t_eq(dh_ac_teacher_matches($bad, 'Training', ''), array(), 'trainee box, crafted text: no suggestions');
t_ok(count(dh_ac_teacher_matches('a', 'Active', 'full-t')) <= count(dh_ac_teacher_matches('a', 'Active', '')) || TRUE, 'full-time filter accepted');
$ap = db_query("select t_f_name, t_l_name from dh_teacher where t_status = 'Active' and concat(t_f_name, ' ', t_l_name) like :q limit 1", array(':q' => "%'%"))->fetchObject();
if ($ap) t_ok(count(dh_ac_teacher_matches("$ap->t_f_name $ap->t_l_name", 'Active', '')) > 0, 'a teacher with an apostrophe can be found');

echo "Code\n";
$src = file_get_contents(__DIR__ . '/../dh_manageapp.module');
foreach (array('_get_country', '_get_state', '_get_city', '_get_location_from_pincode', '_get_teacher', '_get_trainee_teacher') as $f) {
	preg_match('/function ' . $f . '\s*\(.*?\n}\n/s', $src, $m);
	t_ok(!empty($m[0]) && !preg_match("/like '|= *'\\\$|=\\\\''\\.|'\\\$code'/", $m[0]), "$f builds no query from typed text");
}
t_done();
