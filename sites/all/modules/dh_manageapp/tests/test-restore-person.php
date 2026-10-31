<?php
require __DIR__ . '/lib.php';
echo "Restore: building one person from old rows\n";
// A made-up old row (only the keys dh_restore_person reads).
function t_rs($over = array()) {
  return array_merge(array('rs_f_name' => ' Zzqram ', 'rs_m_name' => '', 'rs_l_name' => 'Zzqkumar', 'rs_gender' => 'm', 'rs_dob' => '1970-05-06',
    'rs_address' => 'Zzq street 1', 'rs_zip' => '411001', 'rs_country_name' => '', 'rs_state_name' => '', 'rs_city_name' => '',
    'rs_email' => 'zzq.ram@example.com', 'rs_phone_mobile' => '[91]9876500001', 'rs_phone_home' => '', 'rs_phone_office' => '', 'rs_phone_fax' => '',
    'rs_education' => 'BCom', 'rs_occupation' => 'Zzq work', 'rs_company' => '', 'rs_post' => 'Clerk', 'rs_department' => '',
    'rs_mother_tongue' => 'Marathi', 'rs_other_langs' => 'Hindi, English', 'rs_health_physical' => 'BLOOD PRESSURE', 'rs_health_mental' => 'NO',
    'rs_medication' => NULL, 'rs_intoxicants' => '-', 'rs_other_technique' => 'N/A', 'rs_teach_others' => ''), $over);
}

foreach (array(array(NULL, NULL), array('', NULL), array('  ', NULL), array('NO', 0), array(' na ', 0), array('-', 0), array('N/A', 0), array('nil', 0),
  array('None', 0), array('BLOOD PRESSURE', 1), array('yes', 1)) as $c)
  t_eq(dh_restore_flag($c[0]), $c[1], 'flag: ' . var_export($c[0], TRUE));

foreach (array(array('[91]9876500001', '9876500001'), array('9876500001', '9876500001'), array('[91] 98765-00001', '9876500001'),
  array('919876500001', '9876500001'), array('09876500001', '9876500001'), array('[44]7700900123', '447700900123'), array('', ''),
  array(NULL, ''), array('[91]', '')) as $c)
  t_eq(dh_restore_mobile($c[0]), $c[1], 'mobile: ' . var_export($c[0], TRUE));

// Places: a country, state and city whose names are unique at their level, so the expected codes are certain.
$p = db_query("select co.c_name cn, co.c_code cc, st.s_name sn, st.s_code sc, ci.c_name ctn, ci.c_id cid
  from dh_city ci join dh_state st on st.s_code = ci.c_state and st.s_country = ci.c_country join dh_country co on co.c_code = ci.c_country
  where (select count(*) from dh_country x where x.c_name = co.c_name) = 1
    and (select count(*) from dh_state x where x.s_country = co.c_code and x.s_name = st.s_name) = 1
    and (select count(*) from dh_city x where x.c_country = co.c_code and x.c_state = st.s_code and x.c_name = ci.c_name) = 1
  order by ci.c_id limit 1")->fetchAssoc();
t_ok((bool) $p, 'places: local data has a country, state and city to test with');
t_eq(dh_restore_place(strtolower($p['cn']), $p['sn'], $p['ctn']), array('country' => $p['cc'], 'state' => $p['sc'], 'city' => (int) $p['cid']), 'place: all three match, any case');
t_eq(dh_restore_place($p['cn'], $p['sn'], 'Zzq no such city'), array('country' => $p['cc'], 'state' => $p['sc'], 'city' => NULL), 'place: an unknown city stays empty');
t_eq(dh_restore_place('Zzq no such country', $p['sn'], $p['ctn']), array('country' => '', 'state' => '', 'city' => NULL), 'place: an unknown country leaves all empty');

t_ok(dh_restore_same_person(array(t_rs(), t_rs(array('rs_f_name' => 'zzqram', 'rs_l_name' => 'ZZQKUMAR ', 'rs_gender' => 'M')))), 'same person: case and spaces do not matter');
t_ok(!dh_restore_same_person(array(t_rs(), t_rs(array('rs_l_name' => 'Zzqother')))), 'same person: another last name is another person');
t_ok(!dh_restore_same_person(array(t_rs(), t_rs(array('rs_gender' => 'F')))), 'same person: another gender is another person');
t_ok(!dh_restore_same_person(array()), 'same person: no rows is not a person');

$a = dh_restore_person(t_rs(array('rs_country_name' => $p['cn'], 'rs_state_name' => $p['sn'], 'rs_city_name' => $p['ctn'])));
t_eq(array($a['a_f_name'], $a['a_l_name'], $a['a_gender'], $a['a_dob']), array('Zzqram', 'Zzqkumar', 'M', '1970-05-06'), 'person: trimmed names, upper-case gender, date of birth');
t_eq(array($a['a_country'], $a['a_state'], $a['a_city']), array($p['cc'], $p['sc'], (int) $p['cid']), 'person: DIPI place codes');
t_eq(array($a['a_phone_mobile'], $a['a_email'], $a['a_designation']), array('9876500001', 'zzq.ram@example.com', 'Clerk'), 'person: mobile in DIPI form, email, post as designation');
t_eq(array($a['a_lang_1'], $a['a_langs']), array('Marathi', 'Hindi, English'), 'person: mother tongue, other languages');
t_eq(array($a['a_problem_physical'], $a['a_problem_mental'], $a['a_medication'], $a['a_addiction_current'], $a['a_other_technique'], $a['a_teach_others']),
  array(1, 0, NULL, 0, 0, NULL), 'person: health and practice boxes');
t_eq($a['a_old'], 1, 'person: a_old is 1, so the first-course box of a current student is never touched');
t_eq(dh_restore_person(t_rs(array('rs_dob' => NULL)))['a_dob'], '', 'person: no date of birth is empty');
t_done();
