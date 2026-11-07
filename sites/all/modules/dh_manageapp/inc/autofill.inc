<?php
/**
 * Form autofill API for schedule.vridhamma.org. The schedule site (vri-3) calls three JSON
 * endpoints server to server: autofill/email/start, autofill/email/verify and autofill/fetch.
 * DIPI emails a one-time code, groups the email's applications into people, and returns the
 * listed fields of one person's latest application. Design: private Claude Doc
 * "Form autofill - DIPI API design" (6 Nov 2026).
 */

/** Target forms (schedule webform ids) => 'adult' or 'child'. */
function dh_autofill_forms()
{
	return array(
		'application_form' => 'adult', 'stp_application_form' => 'adult', 'long_course_application_form' => 'adult',
		'vi_application_form' => 'adult', 'one_day_application_form' => 'adult',
		'global_pagoda_application_form' => 'adult', 'vi_one_day_application_form' => 'adult',
		'children_application_form' => 'child', 'teenager_form' => 'child',
	);
}

/**
 * Course types (dh_type_detail COURSE-TYPE td_key) => what an application of that type can do:
 * 'adult' fills adult forms, 'child' fills children and teenager forms, 'count' only counts when
 * deciding one person or several. A type not listed here counts as 'count'.
 */
function dh_autofill_course_roles()
{
	return array(
		'10-Day' => 'adult', '10-DayExecutive' => 'adult', '10-DaySpecial' => 'adult', '2-DayOSC' => 'adult',
		'3-DayOSC' => 'adult', '20-DayOSC' => 'adult', '30-DayOSC' => 'adult', '45-DayOSC' => 'adult',
		'60-DayOSC' => 'adult', 'Satipatthana' => 'adult', 'TSC' => 'adult',
		'7-DayTeen' => 'child', 'Child-Teen' => 'child',
		'1-DayOSC' => 'count', 'Other' => 'count', 'ServicePeriod' => 'count', 'OSProgram' => 'count',
	);
}

/** A name part for grouping: lowercase, dots removed, trimmed, runs of spaces collapsed. */
function dh_autofill_norm($s)
{
	$s = mb_strtolower(str_replace('.', '', (string) $s), 'UTF-8');
	return trim(preg_replace('/\s+/u', ' ', $s));
}

/** "One person" = normalized first name + last name + gender. */
function dh_autofill_person_key($r)
{
	return dh_autofill_norm($r['a_f_name']).'|'.dh_autofill_norm($r['a_l_name']).'|'.strtoupper(trim((string) $r['a_gender']));
}

/**
 * The counted applications on an email: a_id, names, gender, a_created, a_center and the course
 * type key. Leaves out status Duplicate and Errors, and the test centre 302 unless the email is
 * the test email (variable autofill_test_email). a_email's collation (utf8mb4_general_ci)
 * already ignores case and trailing spaces; the a_email index makes this a quick lookup.
 */
function dh_autofill_applications($email)
{
	$email = trim((string) $email);
	if ($email === '')
		return array();
	$test = trim((string) variable_get('autofill_test_email', ''));
	$rows = db_query("select a.a_id, a.a_email, a.a_f_name, a.a_l_name, a.a_gender, a.a_created, a.a_center, td.td_key
		from dh_applicant a
		left join dh_course c on c.c_id = a.a_course
		left join dh_type_detail td on td.td_id = c.c_course_type
		where a.a_email = :e and ifnull(a.a_status, '') not in ('Duplicate', 'Errors')", array(':e' => $email))->fetchAll(PDO::FETCH_ASSOC);
	$out = array();
	$want = dh_autofill_email_norm($email);
	foreach ($rows as $r)
	{
		// The collation ignores accents ('andre' = 'andré'); only the exact spelling counts.
		if (dh_autofill_email_norm($r['a_email']) !== $want)
			continue;
		if ((int) $r['a_center'] != 302 || ($test !== '' && strcasecmp($test, $email) == 0))
		{
			unset($r['a_email']);
			$out[] = $r;
		}
	}
	return $out;
}

/**
 * One person or several, for a target group ('adult' or 'child'). Adult forms count every
 * application; children's forms count only Children and Teenager applications. The source is
 * the person's latest application (a_created, then a_id) whose course type can fill the group.
 * Returns array('result' => 'none'|'single'|'multiple'), plus 'source' (the row) for 'single'.
 */
function dh_autofill_match($apps, $group)
{
	$roles = dh_autofill_course_roles();
	$people = array();
	$best = NULL;
	foreach ($apps as $r)
	{
		$role = isset($roles[(string) $r['td_key']]) ? $roles[(string) $r['td_key']] : 'count';
		if ($group == 'child' && $role != 'child')
			continue;
		$people[dh_autofill_person_key($r)] = TRUE;
		if ($role == $group && ($best === NULL || $r['a_created'] > $best['a_created']
			|| ($r['a_created'] == $best['a_created'] && (int) $r['a_id'] > (int) $best['a_id'])))
			$best = $r;
	}
	if (count($people) > 1)
		return array('result' => 'multiple');
	if (count($people) == 0 || $best === NULL)
		return array('result' => 'none');
	return array('result' => 'single', 'source' => $best);
}

/** "N*** B*****": the first letter of every word of the first and last name, the rest '*'. */
function dh_autofill_mask_name($first, $last)
{
	$out = array();
	foreach (preg_split('/\s+/u', trim($first.' '.$last), -1, PREG_SPLIT_NO_EMPTY) as $w)
		$out[] = mb_substr($w, 0, 1, 'UTF-8').str_repeat('*', mb_strlen($w, 'UTF-8') - 1);
	return implode(' ', $out);
}

/** A stored mobile as E.164 ('+' country code + number), or '' when it is not a valid number. */
function dh_autofill_e164($cc, $num)
{
	$c = preg_replace('/\D/', '', (string) $cc);
	$n = ltrim(preg_replace('/\D/', '', (string) $num), '0');
	if ($c === '' || (int) $c == 0 || $n === '')
		return '';
	$util = \libphonenumber\PhoneNumberUtil::getInstance();
	try
	{
		$p = $util->parse('+'.$c.$n, NULL);
		return $util->isValidNumber($p) ? $util->format($p, \libphonenumber\PhoneNumberFormat::E164) : '';
	}
	catch (\libphonenumber\NumberParseException $e)
	{
		return '';
	}
}

/**
 * Another stored phone (home, office, emergency) as E.164, or '' when it is not a valid number.
 * A number that starts with + (after any brackets or spaces) or 00 is read as international. Any other number is read as a
 * number of the country of the applicant's mobile ($cc), so '022 2345 6789' with 91 is
 * +912223456789. No country code anywhere gives ''.
 */
function dh_autofill_phone($cc, $num)
{
	$num = trim((string) $num);
	// Text, an extension or two numbers in one field: left out, so digits are never glued together.
	if (preg_match('/[\p{L}\/,;]/u', $num))
		return '';
	$n = preg_replace('/\D/', '', $num);
	if ($n === '')
		return '';
	$util = \libphonenumber\PhoneNumberUtil::getInstance();
	try
	{
		if (preg_match('/^\W*\+/', $num) || substr($n, 0, 2) === '00')
			$p = $util->parse('+'.preg_replace('/^00/', '', $n), NULL);
		else
		{
			$c = (int) preg_replace('/\D/', '', (string) $cc);
			$region = $c ? $util->getRegionCodeForCountryCode($c) : 'ZZ';
			if ($region === 'ZZ')
				return '';
			$p = $util->parse($n, $region);
		}
		return $util->isValidNumber($p) ? $util->format($p, \libphonenumber\PhoneNumberFormat::E164) : '';
	}
	catch (\libphonenumber\NumberParseException $e)
	{
		return '';
	}
}

/** 'YYYY-MM-DD' for a real date from its parts (year 1900 or later), else ''. */
function dh_autofill_ymd($y, $m, $d)
{
	if (!is_numeric($y) || !is_numeric($m) || !is_numeric($d))
		return '';
	$y = (int) $y;
	$m = (int) $m;
	$d = (int) $d;
	return ($y >= 1900 && checkdate($m, $d, $y)) ? sprintf('%04d-%02d-%02d', $y, $m, $d) : '';
}

/** A stored DATE / DATETIME / TIMESTAMP as 'YYYY-MM-DD', or '' when it is not a real date. */
function dh_autofill_date($v)
{
	if (!preg_match('/^(\d{4})-(\d{2})-(\d{2})/', (string) $v, $m))
		return '';
	return dh_autofill_ymd($m[1], $m[2], $m[3]);
}

/**
 * The fields fetch returns for one application and target form. Only keys on these lists ever
 * leave DIPI; a column not listed here is never returned. Empty values are left out.
 */
function dh_autofill_fields($a_id, $form)
{
	$forms = dh_autofill_forms();
	if (!is_string($form) || !array_key_exists($form, $forms))
		return array();
	$group = $forms[$form];
	$a = db_query("select * from dh_applicant where a_id = :id", array(':id' => $a_id))->fetchAssoc();
	if (!$a)
		return array();
	$ae = db_query("select * from dh_applicant_extra where ae_applicant = :id", array(':id' => $a_id))->fetchAssoc();
	$ac = db_query("select * from dh_applicant_course where ac_applicant = :id", array(':id' => $a_id))->fetchAssoc();
	$al = db_query("select * from dh_applicant_lc where al_applicant = :id", array(':id' => $a_id))->fetchAssoc();
	$ae = $ae ? $ae : array();
	$ac = $ac ? $ac : array();
	$al = $al ? $al : array();

	$f = array();
	foreach (array('a_old', 'a_f_name', 'a_m_name', 'a_l_name', 'a_address', 'a_zip',
		'a_emergency_name', 'a_emergency_relation', 'a_lang_1_level', 'a_lang_2_level', 'a_lang_discourse',
		'a_education', 'a_occupation', 'a_company', 'a_department', 'a_designation') as $k)
		$f[$k] = $a[$k];
	$f['a_dob'] = dh_autofill_date($a['a_dob']);
	$f['a_phone_mobile'] = dh_autofill_e164($a['a_mob_country'], $a['a_phone_mobile']);
	// The other phones have no country code of their own; the mobile's is used. Only valid numbers.
	foreach (array('a_phone_home', 'a_phone_office', 'a_emergency_num') as $k)
		$f[$k] = dh_autofill_phone($a['a_mob_country'], $a[$k]);
	// DIPI stores M / F; the forms want Male / Female. Anything else is left out.
	$g = strtoupper(trim((string) $a['a_gender']));
	$f['a_gender'] = ($g === 'M' || $g === 'MALE') ? 'Male' : (($g === 'F' || $g === 'FEMALE') ? 'Female' : '');

	// ID: the first one set, Aadhaar > Passport > PAN > Voter ID; the full number (owner decision, 6 Nov).
	// The VI forms offer only Passport and Voter ID, so they get the first of those that is set.
	$id_types = array('a_aadhar' => 'Aadhar', 'a_passport' => 'Passport', 'a_pancard' => 'Pancard', 'a_voter_id' => 'Voter ID');
	$usual = '';
	foreach ($id_types as $col => $type)
		if (trim((string) $a[$col]) !== '')
		{
			$usual = $type;
			break;
		}
	if (in_array($form, array('vi_application_form', 'vi_one_day_application_form'), TRUE))
		$id_types = array('a_passport' => 'Passport', 'a_voter_id' => 'Voter ID');
	foreach ($id_types as $col => $type)
		if (trim((string) $a[$col]) !== '')
		{
			$f['photo_id_type'] = $type;
			$f['photo_id_val'] = trim($a[$col]);
			break;
		}
	// The stored issuer and issue date belong to the usual ID; keep them only with that ID.
	$id_issue_ok = isset($f['photo_id_type']) && $f['photo_id_type'] === $usual;

	// DIPI stores a city id; the form wants the name. Country and state go as codes and names.
	if ((int) $a['a_city'] > 0)
		$f['a_city'] = db_query("select c_name from dh_city where c_id = :id", array(':id' => $a['a_city']))->fetchField();
	if (empty($f['a_city']) && trim((string) $a['a_city_str']) !== '')
		$f['a_city'] = trim($a['a_city_str']);
	if (trim((string) $a['a_country']) !== '')
	{
		$cn = db_query("select c_name from dh_country where c_code = :c", array(':c' => $a['a_country']))->fetchField();
		$sn = (trim((string) $a['a_state']) === '') ? '' : db_query("select s_name from dh_state where s_code = :s and s_country = :c",
			array(':s' => $a['a_state'], ':c' => $a['a_country']))->fetchField();
		if ($cn)
			$f['country'] = array('country_code' => $a['a_country'], 'country_name' => $cn,
				'state_code' => $sn ? $a['a_state'] : '', 'state_name' => $sn ? $sn : '');
	}

	foreach (array('ae_nationality', 'ae_id_issued_by', 'ae_seva_details') as $k)
		$f[$k] = isset($ae[$k]) ? $ae[$k] : '';
	$f['ae_id_issued'] = isset($ae['ae_id_issued']) ? dh_autofill_date($ae['ae_id_issued']) : '';
	if (!$id_issue_ok)
		unset($f['ae_id_issued_by'], $f['ae_id_issued']);
	if ($group == 'child')
		foreach (array('ae_father', 'ae_mother', 'ae_parent_course', 'ae_ch_course', 'ae_ch_course_detail', 'ae_ch_practice', 'ae_ch_practice_detail') as $k)
			$f[$k] = isset($ae[$k]) ? $ae[$k] : '';

	foreach (array('ac_teen', 'ac_10d', 'ac_stp', 'ac_spl', 'ac_20d', 'ac_30d', 'ac_45d', 'ac_60d', 'ac_tsc', 'ac_service',
		'ac_practice_details',
		'ac_first_location_str', 'ac_first_teacher_str', 'ac_last_location_str', 'ac_last_teacher_str') as $k)
		$f[$k] = isset($ac[$k]) ? $ac[$k] : '';
	if ($ac)
	{
		$f['date_first_course'] = dh_autofill_ymd($ac['ac_first_year'], $ac['ac_first_month'], $ac['ac_first_day']);
		$f['date_last_course'] = dh_autofill_ymd($ac['ac_last_year'], $ac['ac_last_month'], $ac['ac_last_day']);
	}

	if ($form == 'long_course_application_form')
		foreach (array('al_recent_sat_type', 'al_last_lc_type', 'al_last_lc_date', 'al_last_lc_location', 'al_last_lc_teacher') as $k)
			$f[$k] = isset($al[$k]) ? $al[$k] : '';

	// Stored values often carry stray spaces ('Student '): trim them; empty values are left out.
	foreach ($f as $k => $v)
	{
		if (is_string($v))
			$f[$k] = $v = trim($v);
		if ($v === NULL || $v === FALSE || $v === '')
			unset($f[$k]);
	}
	return $f;
}

/** HMAC with this site's own secret (set by the installer), for emails and codes at rest. */
function dh_autofill_hmac($v)
{
	$secret = (string) variable_get('autofill_hash_secret', '');
	if (strlen($secret) < 32)
		throw new Exception('autofill_hash_secret is not set');
	return hash_hmac('sha256', (string) $v, $secret);
}

/** An email as compared and hashed: trimmed and lowercased. */
function dh_autofill_email_norm($e)
{
	return mb_strtolower(trim((string) $e), 'UTF-8');
}

/** One audit row. Never a code, token, email, name or field value. */
function dh_autofill_log($endpoint, $email_hash, $form, $result, $ip, $request_id = '')
{
	db_insert('dh_autofill_log')->fields(array(
		'afl_tstamp' => REQUEST_TIME, 'afl_endpoint' => $endpoint, 'afl_email_hash' => (string) $email_hash,
		'afl_form' => substr((string) $form, 0, 64), 'afl_result' => $result,
		'afl_client_ip' => substr((string) $ip, 0, 45), 'afl_request_id' => substr((string) $request_id, 0, 64),
	))->execute();
}

/** Codes and tokens go after 24 hours, the audit log after 180 days. */
function dh_autofill_cleanup()
{
	db_delete('dh_autofill_code')->condition('afc_created', REQUEST_TIME - 86400, '<')->execute();
	db_delete('dh_autofill_token')->condition('aft_created', REQUEST_TIME - 86400, '<')->execute();
	db_delete('dh_autofill_log')->condition('afl_tstamp', REQUEST_TIME - 180 * 86400, '<')->execute();
}

/** The code email. The code leads the subject, so mail apps show it at once and never group two codes. */
function dh_autofill_code_email($code)
{
	return array(
		'subject' => $code.' is your code for your Vipassana application',
		'text' => 'Your code to fill your Vipassana application is '.$code.'. It expires in 10 minutes. If you did not ask for this, ignore this email.',
	);
}

/**
 * Email the code through Mailgun, waiting at most 3 seconds. TRUE when Mailgun accepted it.
 * In test mode (mode_test) DIPI sends to the test address instead, as it does for letters.
 * autofill_mail_off (local tests only, never on prod) skips sending. The email carries no
 * application id, so the bounce webhook (Email Error tag) leaves it alone.
 */
function dh_autofill_send_code($email, $code)
{
	// autofill_mail_off is for local tests only: ignored on a vri server (the same check tests/lib.php uses).
	if (variable_get('autofill_mail_off', 0) && stripos((string) db_query('select @@hostname')->fetchField(), 'vri') === FALSE)
		return TRUE;
	$from = 'Vipassana Applications <donotreply@vridhamma.org>';
	$to = variable_get('mode_test', '0') ? variable_get('mode_test_emails', '') : $email;
	try
	{
		$conf = new \Mailgun\HttpClientConfigurator();
		$conf->setApiKey(variable_get('mailgun_key', ''));
		$conf->setHttpClient(new \Http\Client\Curl\Client(NULL, NULL, array(CURLOPT_TIMEOUT => 3, CURLOPT_CONNECTTIMEOUT => 2)));
		$mg = \Mailgun\Mailgun::configure($conf);
		$m = dh_autofill_code_email($code);
		$mg->messages()->send(get_mailgun_domain($from), array(
			'from' => $from,
			'to' => $to,
			'subject' => $m['subject'],
			'text' => $m['text'],
			'o:tag' => 'autofill-code',
			'o:tracking' => 'no',
		));
		return TRUE;
	}
	catch (Throwable $e)
	{
		watchdog('autofill', 'Code email failed: @c', array('@c' => get_class($e)), WATCHDOG_WARNING);
		return FALSE;
	}
}

/**
 * email/start: store a code and email it. The same answer for every email, known or not, and
 * no application lookup here, so neither the answer nor its timing tells whether DIPI knows the
 * email. Over a limit: array('status' => 'limited', 'retry_after' => seconds), and no email.
 * $mail_fn($email, $code) sends the code (tests pass their own).
 */
function dh_autofill_email_start($in, $mail_fn = 'dh_autofill_send_code')
{
	$email = dh_autofill_email_norm(isset($in['email']) ? $in['email'] : '');
	$form = (string) $in['form'];
	if (!array_key_exists($form, dh_autofill_forms()))
		throw new InvalidArgumentException('unknown form');
	$ip = isset($in['client_ip']) ? substr((string) $in['client_ip'], 0, 45) : '';
	$req = isset($in['request_id']) ? (string) $in['request_id'] : '';
	$now = REQUEST_TIME;
	dh_autofill_cleanup();
	$eh = dh_autofill_hmac($email);

	// a_email is varchar(100): a longer one is treated as invalid (nothing stored or sent).
	$valid = (bool) filter_var($email, FILTER_VALIDATE_EMAIL) && mb_strlen($email, 'UTF-8') <= 100;
	$otp_id = bin2hex(random_bytes(16));
	$code = sprintf('%06d', random_int(0, 999999));

	// Limits and the code row go in one lock, so a parallel burst cannot pass the limits.
	// The Mailgun call stays outside it.
	$wait = 0;
	if ($valid)
	{
		if (!dh_manageapp_lock_acquire('dh_autofill_start', 5.0, 1))
		{
			dh_autofill_log('email_start', $eh, $form, 'limited', $ip, $req);
			return array('status' => 'limited', 'retry_after' => 5);
		}
		try
		{
			// Limits: per email 1 a minute, 3 an hour, 10 a day; per client IP 60 an hour.
			$times = db_query("select afc_created from dh_autofill_code where afc_email_hash = :h and afc_created > :t order by afc_created desc",
				array(':h' => $eh, ':t' => $now - 86400))->fetchCol();
			if ($times && $now - $times[0] < 60)
				$wait = max($wait, 60 - ($now - $times[0]));
			$hour = array();
			foreach ($times as $t)
				if ($t > $now - 3600)
					$hour[] = $t;
			if (count($hour) >= 3)
				$wait = max($wait, $hour[2] + 3600 - $now);
			if (count($times) >= 10)
				$wait = max($wait, $times[9] + 86400 - $now);
			if ($ip !== '')
			{
				$ipt = db_query("select afc_created from dh_autofill_code where afc_client_ip = :ip and afc_created > :t order by afc_created desc",
					array(':ip' => $ip, ':t' => $now - 3600))->fetchCol();
				if (count($ipt) >= 60)
					$wait = max($wait, $ipt[59] + 3600 - $now);
			}
			// Global cap on code emails per hour, whoever asks.
			$cap = (int) variable_get('autofill_global_hourly_cap', 1000);
			$all = (int) db_query("select count(*) from dh_autofill_code where afc_created > :t", array(':t' => $now - 3600))->fetchField();
			if ($all >= $cap)
			{
				$wait = 300;
				watchdog('autofill', 'Global hourly code cap reached', array(), WATCHDOG_WARNING);
			}
			if ($wait <= 0)
				db_insert('dh_autofill_code')->fields(array(
					'afc_otp_id' => $otp_id, 'afc_email' => $email, 'afc_email_hash' => $eh,
					'afc_code_hash' => dh_autofill_hmac($otp_id.'|'.$code), 'afc_form' => $form, 'afc_client_ip' => $ip,
					'afc_tries' => 0, 'afc_created' => $now, 'afc_expires' => $now + 600,
				))->execute();
		}
		finally
		{
			lock_release('dh_autofill_start');
		}
	}
	if ($wait > 0)
	{
		dh_autofill_log('email_start', $eh, $form, 'limited', $ip, $req);
		return array('status' => 'limited', 'retry_after' => (int) $wait);
	}
	if ($valid)
	{
		$sent = call_user_func($mail_fn, $email, $code);
		dh_autofill_log('email_start', $eh, $form, $sent ? 'sent' : 'mail_failed', $ip, $req);
	}
	else
		dh_autofill_log('email_start', $eh, $form, 'bad_email', $ip, $req);
	return array('otp_id' => $otp_id, 'expires_in' => 600, 'resend_after' => 60);
}

/**
 * email/verify: check the code (5 wrong tries at most), then group the email's applications
 * into people for the form the code was asked for, and issue a 15-minute token.
 */
function dh_autofill_email_verify($in)
{
	$otp_id = (string) $in['otp_id'];
	$code = trim((string) $in['code']);
	$now = REQUEST_TIME;
	$row = db_query("select * from dh_autofill_code where afc_otp_id = :id", array(':id' => $otp_id))->fetchAssoc();
	if (!$row || $row['afc_used'] || $row['afc_expires'] < $now)
	{
		dh_autofill_log('email_verify', $row ? $row['afc_email_hash'] : '', $row ? $row['afc_form'] : '', 'expired', $row ? $row['afc_client_ip'] : '');
		return array('status' => 'expired');
	}
	$forms = dh_autofill_forms();
	if (!array_key_exists($row['afc_form'], $forms))
	{
		dh_autofill_log('email_verify', $row['afc_email_hash'], $row['afc_form'], 'expired', $row['afc_client_ip']);
		return array('status' => 'expired');
	}
	// The atomic try counter is the gate: every check, right or wrong, takes one of the 5 tries first.
	$took = db_update('dh_autofill_code')->expression('afc_tries', 'afc_tries + 1')
		->condition('afc_otp_id', $otp_id)->condition('afc_tries', 5, '<')->isNull('afc_used')->execute();
	if ($took != 1)
	{
		$used = db_query("select afc_used from dh_autofill_code where afc_otp_id = :id", array(':id' => $otp_id))->fetchField();
		$status = $used ? 'expired' : 'too_many_attempts';
		dh_autofill_log('email_verify', $row['afc_email_hash'], $row['afc_form'], $status, $row['afc_client_ip']);
		return array('status' => $status);
	}
	if (!preg_match('/^\d{6}$/', $code) || !hash_equals($row['afc_code_hash'], dh_autofill_hmac($otp_id.'|'.$code)))
	{
		$tries = (int) db_query("select afc_tries from dh_autofill_code where afc_otp_id = :id", array(':id' => $otp_id))->fetchField();
		$status = ($tries >= 5) ? 'too_many_attempts' : 'wrong_code';
		if ($tries >= 5)
			db_update('dh_autofill_code')->fields(array('afc_email' => ''))->condition('afc_otp_id', $otp_id)->execute();
		dh_autofill_log('email_verify', $row['afc_email_hash'], $row['afc_form'], $status, $row['afc_client_ip']);
		return array('status' => $status);
	}
	// The code is used once: only the call that marks it gets a token.
	$n = db_update('dh_autofill_code')->fields(array('afc_used' => $now, 'afc_email' => ''))
		->condition('afc_otp_id', $otp_id)->isNull('afc_used')->execute();
	if ($n != 1)
		return array('status' => 'expired');
	$m = dh_autofill_match(dh_autofill_applications($row['afc_email']), $forms[$row['afc_form']]);
	$token = rtrim(strtr(base64_encode(random_bytes(32)), '+/', '-_'), '=');
	db_insert('dh_autofill_token')->fields(array(
		'aft_hash' => hash('sha256', $token), 'aft_email_hash' => $row['afc_email_hash'], 'aft_form' => $row['afc_form'],
		'aft_result' => $m['result'], 'aft_source_app' => ($m['result'] == 'single') ? $m['source']['a_id'] : NULL,
		'aft_client_ip' => $row['afc_client_ip'], 'aft_created' => $now, 'aft_expires' => $now + 900,
	))->execute();
	$out = array('status' => 'verified', 'token' => $token, 'result' => $m['result']);
	if ($m['result'] == 'single')
		$out['person'] = array(
			'masked_name' => dh_autofill_mask_name($m['source']['a_f_name'], $m['source']['a_l_name']),
			'last_applied_month' => substr($m['source']['a_created'], 0, 7),
		);
	dh_autofill_log('email_verify', $row['afc_email_hash'], $row['afc_form'], $m['result'], $row['afc_client_ip']);
	return $out;
}

/** The source block of fetch: application id, created, centre name and course type name. */
function dh_autofill_source_info($a_id)
{
	$r = db_query("select a.a_id, a.a_created, ce.c_name centre_name, td.td_val1 course_type
		from dh_applicant a
		left join dh_center ce on ce.c_id = a.a_center
		left join dh_course c on c.c_id = a.a_course
		left join dh_type_detail td on td.td_id = c.c_course_type
		where a.a_id = :id", array(':id' => $a_id))->fetchAssoc();
	return array('application_id' => (int) $r['a_id'], 'created' => $r['a_created'],
		'centre_name' => (string) $r['centre_name'], 'course_type' => (string) $r['course_type']);
}

/**
 * fetch: the listed fields of the token's source application. Only a 'single' token for the
 * same form, and only once; anything else answers invalid_token.
 */
function dh_autofill_fetch($in)
{
	$hash = hash('sha256', (string) $in['token']);
	$form = (string) $in['form'];
	$t = db_query("select * from dh_autofill_token where aft_hash = :h", array(':h' => $hash))->fetchAssoc();
	$ok = $t && !$t['aft_used'] && $t['aft_expires'] >= REQUEST_TIME && $t['aft_result'] == 'single'
		&& $t['aft_form'] === $form && $t['aft_source_app'];
	if ($ok)
		$ok = db_update('dh_autofill_token')->fields(array('aft_used' => REQUEST_TIME))
			->condition('aft_hash', $hash)->isNull('aft_used')->execute() == 1;
	if (!$ok)
	{
		dh_autofill_log('fetch', $t ? $t['aft_email_hash'] : '', $form, 'invalid_token', $t ? $t['aft_client_ip'] : '');
		return array('status' => 'invalid_token');
	}
	if (!db_query("select count(*) from dh_applicant where a_id = :id", array(':id' => $t['aft_source_app']))->fetchField())
	{
		dh_autofill_log('fetch', $t['aft_email_hash'], $form, 'invalid_token', $t['aft_client_ip']);
		return array('status' => 'invalid_token');
	}
	$out = array('source' => dh_autofill_source_info($t['aft_source_app']), 'fields' => dh_autofill_fields($t['aft_source_app'], $form));
	dh_autofill_log('fetch', $t['aft_email_hash'], $form, 'fetched', $t['aft_client_ip']);
	return $out;
}

/** Answer a call: JSON with the HTTP status, never cached. Ends the request. */
function dh_autofill_reply($status, $data)
{
	$texts = array(200 => 'OK', 400 => 'Bad Request', 403 => 'Forbidden', 405 => 'Method Not Allowed',
		500 => 'Internal Server Error', 503 => 'Service Unavailable');
	drupal_add_http_header('Status', $status.' '.$texts[$status]);
	drupal_add_http_header('Cache-Control', 'no-store');
	drupal_json_output($data);
	drupal_exit();
}

/** The caller must send the API key and come from an allowed address (both kept in Drupal variables). */
function dh_autofill_caller_ok()
{
	$want = (string) variable_get('autofill_api_key_hash', '');
	$key = isset($_SERVER['HTTP_X_API_KEY']) ? (string) $_SERVER['HTTP_X_API_KEY'] : '';
	if ($want === '' || $key === '' || !hash_equals($want, hash('sha256', $key)))
		return FALSE;
	$ips = array_filter(array_map('trim', explode(',', (string) variable_get('autofill_allowed_ips', ''))));
	return in_array(ip_address(), $ips, TRUE);
}

/** Page callback of autofill/email/start, autofill/email/verify and autofill/fetch. */
function dh_autofill_endpoint($step)
{
	drupal_page_is_cacheable(FALSE);
	if (!variable_get('autofill_enabled', 0))
		dh_autofill_reply(503, array('status' => 'disabled'));
	if ($_SERVER['REQUEST_METHOD'] !== 'POST')
		dh_autofill_reply(405, array('status' => 'method_not_allowed'));
	if (!dh_autofill_caller_ok())
		dh_autofill_reply(403, array('status' => 'forbidden'));
	$in = json_decode(file_get_contents('php://input'), TRUE);
	$need = array('email_start' => array('email', 'form', 'client_ip'), 'email_verify' => array('otp_id', 'code'), 'fetch' => array('token', 'form'));
	$bad = !is_array($in) || !isset($need[$step]);
	if (!$bad)
		foreach ($need[$step] as $k)
			if (!isset($in[$k]) || !is_scalar($in[$k]))
				$bad = TRUE;
	$forms = dh_autofill_forms();
	if (!$bad && isset($in['form']) && !isset($forms[$in['form']]))
		$bad = TRUE;
	if ($bad)
		dh_autofill_reply(400, array('status' => 'bad_request'));
	try
	{
		if ($step == 'email_start')
			$out = dh_autofill_email_start($in);
		elseif ($step == 'email_verify')
			$out = dh_autofill_email_verify($in);
		else
			$out = dh_autofill_fetch($in);
	}
	catch (Throwable $e)
	{
		watchdog('autofill', 'Autofill @s failed: @c', array('@s' => $step, '@c' => get_class($e)), WATCHDOG_ERROR);
		dh_autofill_reply(500, array('status' => 'error'));
	}
	dh_autofill_reply(200, $out);
}
