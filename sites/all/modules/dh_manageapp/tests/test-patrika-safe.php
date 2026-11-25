<?php
// Patrika (newsletter) pages (hole 8, 24 Nov 2026): the search boxes, the PIN / state / city lookups and the edit
// page pass every value to the database as data (never pasted into the SQL); the payment / note Delete links carry a
// security token and must belong to the subscriber; note Delete checks the NOTE (it used to check the payments, so it
// mostly showed "Page not found"); the course-finalize import reads the student's PIN as data.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-patrika-safe.php
require __DIR__ . '/lib.php';

function t_patrika($over = array())
{
	static $n = 0;
	$n++;
	return (int) db_insert('dh_patrika')->fields(array_merge(array('p_student' => 0, 'p_f_name' => 'Zzqpat', 'p_l_name' => "Tester$n",
		'p_zip' => '999990', 'p_country' => 'IN', 'p_state' => 'MH', 'p_vray_id' => 990000000 + $n + mt_rand(0, 9999) * 10, 'p_created' => date('Y-m-d H:i:s'), 'p_updated' => date('Y-m-d H:i:s'), 'p_created_by' => 1, 'p_updated_by' => 1), $over))->execute();
}
function t_search($storage)
{
	$q = dh_patrika_search_query($storage);
	return $q ? db_query($q[0], $q[1])->fetchCol() : NULL;
}
function t_json($fn, $post)
{
	$saved = $_POST;
	$_POST = $post;
	ob_start();
	$fn();
	$out = ob_get_clean();
	$_POST = $saved;
	return json_decode($out, TRUE);
}

t_in_txn(function () {
	$p1 = t_patrika(array('p_f_name' => 'Zzqpatone', 'p_l_name' => 'Zzqalpha', 'p_zip' => '999991'));
	$p2 = t_patrika(array('p_f_name' => 'Zzqpattwo', 'p_l_name' => 'Zzqbeta', 'p_zip' => '999992'));
	$v1 = db_query("select p_vray_id from dh_patrika where p_id = :p", array(':p' => $p1))->fetchField();
	$pay = (int) db_insert('dh_patrika_payment')->fields(array('pp_patrika' => $p1, 'pp_amt' => 100, 'pp_receipt_no' => 'ZZQ-R-77', 'pp_receipt_date' => '2026-11-20', 'pp_type' => 'Cash', 'pp_user' => 1, 'pp_deleted' => 0))->execute();
	$note = (int) db_insert('dh_patrika_note')->fields(array('pn_patrika' => $p1, 'pn_note' => 'zzq test note', 'pn_created_by' => 1, 'pn_tstamp' => date('Y-m-d H:i:s'), 'pn_deleted' => 0))->execute();

	echo "Search\n";
	t_eq(dh_patrika_search_query(array()), NULL, 'all boxes empty: no search');
	t_eq(dh_patrika_search_query(array('student_f_name' => '   ')), NULL, 'only spaces in a name: no search (as before)');
	t_eq(t_search(array('student_f_name' => 'Zzqpaton')), array((string) $p1), 'first name: starts with');
	t_eq(t_search(array('student_l_name' => ' Zzqbet ')), array((string) $p2), 'last name: starts with, spaces trimmed');
	t_eq(t_search(array('patrika_id' => (string) $v1)), array((string) $p1), 'Patrika ID: exact');
	t_eq(t_search(array('pincode' => '999992')), array((string) $p2), 'PIN: exact');
	t_eq(t_search(array('receipt_number' => 'ZZQ-R-77')), array((string) $p1), 'receipt number: exact');
	t_eq(t_search(array('receipt_date' => '2026-11-20', 'student_f_name' => 'Zzqpat')), array((string) $p1), 'receipt date + name together');
	$ok = TRUE;
	try {
		foreach (array('student_f_name', 'student_l_name', 'patrika_id', 'pincode', 'receipt_number', 'receipt_date') as $box) {
			$r = t_search(array($box => "x' or '1'='1"));
			t_eq($r, array(), "database text in $box: only data (finds nothing)");
		}
		t_eq(t_search(array('student_f_name' => "Zzqpat%' union select 1 -- ")), array(), 'database text (union): only data');
		t_eq(dh_patrika_search_query(array('student_f_name' => array('Zzqpat'))), NULL, 'not text: ignored');
	}
	catch (Throwable $e) {
		$ok = FALSE;
	}
	t_ok($ok, 'no database error from any box');

	echo "Lookups (PIN, state, city)\n";
	$states = t_json('dh_patrika_get_state', array('Country' => 'IN'));
	t_ok(is_array($states) && count($states) > 10 && isset($states[0]['s_code']), 'states of a country');
	t_eq(t_json('dh_patrika_get_state', array('Country' => "IN' or '1'='1")), array(), 'state lookup, database text: nothing');
	$st = db_query("select c_state from dh_city where c_country = 'IN' and c_state <> '' group by c_state order by count(*) desc limit 1")->fetchField();
	$cities = t_json('dh_patrika_get_city', array('State' => $st));
	t_ok(is_array($cities) && count($cities) > 0 && isset($cities[0]['c_id']), 'cities of a state');
	t_eq(t_json('dh_patrika_get_city', array('State' => "x' or '1'='1")), array(), 'city lookup, database text: nothing');
	$pin = db_query("select pc_pin from dh_pin_code pc join dh_city c on pc.pc_city = c.c_id where c.c_country = 'IN' limit 1")->fetchField();
	$z = t_json('getDetailsByZipcode', array('PinCode' => $pin));
	t_ok($z['count'] == 1 && $z['StateCount'] > 0 && $z['CityCount'] > 0, 'PIN lookup: place, states and cities');
	$unknown = t_json('getDetailsByZipcode', array('PinCode' => '000000'));
	$z = t_json('getDetailsByZipcode', array('PinCode' => "x' or '1'='1"));
	t_eq($z, $unknown, 'PIN lookup, database text: same answer as an unknown PIN (only data)');
	t_eq($z['count'], 0, 'PIN lookup, database text: no place found');
	$z = t_json('getDetailsByZipcode', array());
	t_eq($z['count'], 0, 'PIN lookup, nothing sent: nothing, no error');

	echo "Delete links\n";
	$url = dh_patrika_delete_link('payment', $p1, $pay);
	t_ok(preg_match('#patrika/payment/delete/' . $p1 . '/' . $pay . '\?token=[\w-]+#', $url) === 1, 'payment Delete link carries a token');
	parse_str(parse_url($url, PHP_URL_QUERY), $qs);
	t_ok(!dh_patrika_delete_item('payment', $p1, $pay, 'wrong-token'), 'wrong token: not deleted');
	t_ok(!dh_patrika_delete_item('payment', $p2, $pay, drupal_get_token("patrika-payment-delete-$p2-$pay")), 'payment of another subscriber: not deleted');
	t_eq(db_query("select pp_deleted from dh_patrika_payment where pp_id = :i", array(':i' => $pay))->fetchField(), '0', '(still not deleted)');
	t_ok(dh_patrika_delete_item('payment', $p1, $pay, $qs['token']), 'right token: deleted');
	t_eq(db_query("select pp_deleted from dh_patrika_payment where pp_id = :i", array(':i' => $pay))->fetchField(), '1', 'payment marked deleted (not erased)');
	$url = dh_patrika_delete_link('note', $p1, $note);
	parse_str(parse_url($url, PHP_URL_QUERY), $qs);
	t_ok(strpos($url, "patrika/note/delete/$p1/$note?token=") !== FALSE, 'note Delete link carries a token');
	t_ok(!dh_patrika_delete_item('note', $p1, $note, ''), 'note, no token: not deleted');
	t_ok(dh_patrika_delete_item('note', $p1, $note, $qs['token']), 'note, right token: deleted');
	t_eq(db_query("select pn_deleted from dh_patrika_note where pn_id = :i", array(':i' => $note))->fetchField(), '1', 'note marked deleted (not erased)');
	t_eq(patrika_note_id_load((string) $note), (string) $note, 'note address check: a real note number');
	t_ok(!patrika_note_id_load("$note or 1=1") && !patrika_note_id_load('999999999999'), 'note address check: text or unknown number refused');
});

echo "Code\n";
$src = file_get_contents(__DIR__ . '/../../dh_patrika/dh_patrika.module');
t_ok(!preg_match('/db_query\([^;]*"\s*\.\s*arg\(/', $src), 'no address part pasted into a query');
t_ok(strpos($src, "s_country='\".\$CountryCode") === FALSE && strpos($src, "c_state='\".\$State") === FALSE && strpos($src, "pc_pin = '\".\$PinCode") === FALSE, 'lookups no longer paste the posted value');
t_ok(strpos($src, "like '\$f_name%'") === FALSE && strpos($src, "p_zip = '\$pincode'") === FALSE, 'search no longer pastes the boxes');
t_ok(strpos($src, "pc_pin='\".\$r['p_zip']") === FALSE && strpos($src, "c_id='.\$course") === FALSE, 'course import reads the PIN and course as data');
t_ok(strpos($src, "'patrika/note/delete/%patrika_id/%patrika_note_id'") !== FALSE, 'note Delete route checks the note');
t_done();
