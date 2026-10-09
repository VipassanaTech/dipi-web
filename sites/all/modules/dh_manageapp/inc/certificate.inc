<?php
/**
 * Course certificates.
 *
 * A centre can print a certificate ("This is to certify that ... has attended 10 day
 * Vipassana Course ...") for a student or Dhamma server who attended a course, from Day 1
 * (the day after the start date), for real courses only (not Work Period, AT Workshop /
 * Meeting or Other). It is the "Certificate" action on a search-result row; the PDF opens
 * in a new tab.
 *
 * Every certificate is kept in a register (dh_certificate) with a number per centre per
 * year, e.g. CAKKA/2026/0001. Reprints keep the number and the first issue date. A centre
 * finds the student from a certificate number by typing it in the search form's
 * "Confirmation / Certificate No" box. Each issue and reprint is written to the
 * application's activity.
 *
 * The centre text (heading, description, address and contact lines, number prefix) is
 * set per centre on centre/{c}/certificate (dh_certificate_setting); until then it is
 * built from the centre record. The Dhamma wheel is the one in the centre websites' logo
 * (imgs/certificate-wheel.png). Tables: deploy/2026-10-09-certificates.sql.
 */

/* Course types that get no certificate (dh_type_detail.td_key). */
function dh_cert_excluded_types()
{
	return array('ServicePeriod', 'OSProgram', 'Other');   // Work Period, AT Workshop/Meeting, Other
}

/* TRUE if a course can give certificates today: live, a real course type, and past Day 0. */
function dh_cert_course_ok($course_id)
{
	$cache = &drupal_static(__FUNCTION__, array());
	if (!array_key_exists($course_id, $cache))
	{
		$c = db_query("select c_start, c_deleted, c_cancelled, td_key from dh_course left join dh_type_detail on td_id = c_course_type where c_id = :c",
			array(':c' => $course_id))->fetchObject();
		$cache[$course_id] = $c && !$c->c_deleted && !$c->c_cancelled
			&& !in_array($c->td_key, dh_cert_excluded_types())
			&& date('Y-m-d') > substr($c->c_start, 0, 10);
	}
	return $cache[$course_id];
}

/* TRUE if this applicant (array with a_attended, a_type, a_course) may get a certificate. */
function dh_cert_eligible($app)
{
	return $app && $app['a_attended'] == 1 && in_array($app['a_type'], array('Student', 'Sevak')) && dh_cert_course_ok($app['a_course']);
}

/* The centre's certificate text: its saved settings, or defaults built from dh_center. */
function dh_cert_settings($centre)
{
	$s = db_query("select * from dh_certificate_setting where certs_center = :c", array(':c' => $centre))->fetchAssoc();
	if ($s)
		return array('prefix' => $s['certs_prefix'], 'title' => $s['certs_title'], 'subtitle' => $s['certs_subtitle'],
			'centre_name' => $s['certs_centre_name'], 'description' => $s['certs_description'],
			'footer' => $s['certs_footer'], 'bottom' => $s['certs_bottom'], 'saved' => TRUE);
	return dh_cert_defaults($centre) + array('saved' => FALSE);
}

/* Default certificate text for a centre, from its dh_center record. */
function dh_cert_defaults($centre)
{
	$c = db_query("select c.*, ci.c_name as city_name from dh_center c left join dh_city ci on ci.c_id = c.c_city where c.c_id = :c",
		array(':c' => $centre))->fetchAssoc();
	$name = trim($c['c_name']);
	$city = trim((string) $c['city_name']);
	$state = db_query("select s_name from dh_state where s_code = :s order by (s_country = :co) desc limit 1",
		array(':s' => $c['c_state'], ':co' => $c['c_country']))->fetchField();
	$state = $state ? trim($state) : trim((string) $c['c_state']);
	$address = trim(preg_replace('/\s+/', ' ', (string) $c['c_address']));
	$place = implode(', ', array_filter(array($address, ($city !== '' && stripos($address, $city) === FALSE) ? $city : '')));
	$prefix = strtoupper(preg_replace('/[^A-Za-z0-9]/', '', (string) $c['c_subdomain']));
	return array(
		'prefix' => $prefix !== '' ? substr($prefix, 0, 12) : 'C' . (int) $centre,
		'title' => mb_strtoupper($name, 'UTF-8'),
		'subtitle' => mb_strtoupper(($city !== '' ? $city . ' · ' : '') . 'Vipassana Meditation Centre', 'UTF-8'),
		'centre_name' => $name,
		'description' => "$name is a Vipassana Meditation Centre" . ($city !== '' ? " at $city" : '') . ($state !== '' ? " in the state of $state" : '') . ".\n"
			. "$name is associated with the chain of meditation centres established to teach and practice Vipassana meditation in the tradition of Sayagyi U Ba Khin as taught by Shri S. N. Goenka.",
		'footer' => implode("\n", array_filter(array(
			"$name Vipassana Meditation Centre",
			$place,
			trim((string) $c['c_phone']) !== '' ? 'Phone: ' . trim($c['c_phone']) : '',
			implode(', ', array_filter(array(trim((string) $c['c_email']) !== '' ? 'Email: ' . trim($c['c_email']) : '', trim((string) $c['c_url']) !== '' ? 'Website: ' . trim($c['c_url']) : ''))),
		))),
		// The address field often already holds the city or state: do not repeat them.
		'bottom' => implode(' · ', array_filter(array($address, ($city !== '' && stripos($address, $city) === FALSE) ? $city : '', ($state !== '' && stripos($address, $state) === FALSE) ? $state : '')))
			. (trim((string) $c['c_pincode']) !== '' ? ' – ' . trim($c['c_pincode']) : ''),
	);
}

/**
 * The register entry for this application: the existing one (a reprint: print count +1),
 * or a new one with the next number for the centre and year. Each issue / reprint is
 * written to the application's activity. Returns the dh_certificate row, or FALSE.
 */
function dh_cert_issue($app)
{
	global $user;
	$old = db_query("select * from dh_certificate where cert_applicant = :a", array(':a' => $app['a_id']))->fetchObject();
	if ($old)
	{
		db_query("update dh_certificate set cert_prints = cert_prints + 1, cert_last_print = :now where cert_id = :id",
			array(':now' => date('Y-m-d H:i:s'), ':id' => $old->cert_id));
		logit($app['a_center'], 'Certificate', $app['a_id'], 'Certificate ' . $old->cert_number . ' printed again');
		return $old;
	}
	$s = dh_cert_settings($app['a_center']);
	$year = (int) date('Y');
	for ($try = 0; $try < 5; $try++)
	{
		$serial = 1 + (int) db_query("select max(cert_serial) from dh_certificate where cert_center = :c and cert_year = :y",
			array(':c' => $app['a_center'], ':y' => $year))->fetchField();
		$number = sprintf('%s/%d/%04d', $s['prefix'], $year, $serial);
		try
		{
			db_insert('dh_certificate')->fields(array(
				'cert_applicant' => $app['a_id'], 'cert_course' => $app['a_course'], 'cert_center' => $app['a_center'],
				'cert_year' => $year, 'cert_serial' => $serial, 'cert_number' => $number, 'cert_issued' => date('Y-m-d'),
				'cert_issued_by' => $user->uid, 'cert_prints' => 1, 'cert_last_print' => date('Y-m-d H:i:s'),
			))->execute();
			logit($app['a_center'], 'Certificate', $app['a_id'], 'Certificate ' . $number . ' issued');
			return db_query("select * from dh_certificate where cert_applicant = :a", array(':a' => $app['a_id']))->fetchObject();
		}
		catch (PDOException $e)
		{
			// Someone else took this number (or issued this student's certificate) at the same moment.
			$row = db_query("select * from dh_certificate where cert_applicant = :a", array(':a' => $app['a_id']))->fetchObject();
			if ($row)
				return $row;
		}
	}
	return FALSE;
}

/* The Dhamma wheel, exactly as in the centre websites' header logo (imgs/certificate-wheel.png,
   cut from sarita.vridhamma.org/themes/custom/dhamma/images/header-bg.png), as an inline image. */
function dh_cert_wheel_src()
{
	$png = @file_get_contents(drupal_get_path('module', 'dh_manageapp') . '/imgs/certificate-wheel.png');
	return $png ? 'data:image/png;base64,' . base64_encode($png) : '';
}

/**
 * The certificate page as HTML (A4, printed by wkhtmltopdf with no margins).
 * $v: name, days, from, to, number, issued, server (TRUE for a Dhamma server).
 */
function dh_cert_html($s, $v)
{
	$e = function ($x) { return check_plain($x); };
	$lines = function ($x) use ($e) { return implode('<br>', array_map($e, array_filter(array_map('trim', explode("\n", (string) $x)), 'strlen'))); };
	$footer = array_values(array_filter(array_map('trim', explode("\n", (string) $s['footer'])), 'strlen'));
	$foot = $footer ? '<b>' . $e(array_shift($footer)) . '</b>' . ($footer ? '<br>' . implode('<br>', array_map($e, $footer)) : '') : '';
	$verb = !empty($v['server']) ? 'has served in' : 'has attended';
	// Each sentence stays on one line: shrink the text a little for long names / centre names.
	$len = function ($x) { return mb_strlen((string) $x, 'UTF-8'); };
	$name_pt = $len($v['name']) > 42 ? 11 : ($len($v['name']) > 32 ? 12.5 : 14.5);
	$l3 = $len($v['from']) + $len($v['to']) + $len($s['centre_name']);
	$body_pt = $l3 > 52 ? 11.5 : ($l3 > 42 ? 13 : 14.5);
	// The heading, the line under it and the number also stay on one line: scale them down when long
	// (about 14 heading letters fit at full size).
	$tk = min(1, 14 / max(1, $len($s['title'])));
	$title_css = sprintf('font-size:%.1fpt; letter-spacing:%.2fmm;', max(9, 34 * $tk), 3.2 * $tk);
	$sk = min(1, 56 / max(1, $len($s['subtitle'])));
	$sub_css = sprintf('font-size:%.1fpt; letter-spacing:%.2fmm;', max(6.5, 9.5 * $sk), 0.9 * $sk);
	$num_pt = $len($v['number']) > 22 ? 8.5 : ($len($v['number']) > 18 ? 9.5 : 10.5);
	return '<!doctype html><html><head><meta charset="utf-8"><title>' . $e('Certificate ' . $v['number']) . '</title><style>
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; }
body { width: 210mm; height: 296.5mm; background: #1f2c5a; font-family: Georgia, "DejaVu Serif", serif; color: #1f2c5a; }
.page { position: absolute; left: 7mm; top: 7mm; width: 196mm; height: 282.5mm; background: #fbf9f3; border: 0.4mm solid #c9b37a; }
.top { position: absolute; left: 14mm; right: 14mm; top: 16mm; height: 30mm; font-size: 11pt; }
.top .l { position: absolute; left: 0; top: 0; width: 52mm; }
.top .r { position: absolute; right: 0; top: 0; width: 52mm; text-align: right; }
.top .val { display: block; margin-top: 3mm; padding-bottom: 1mm; border-bottom: 0.3mm dashed #8a93b5; min-height: 6mm; font-size: 10.5pt; white-space: nowrap; }
.wheel { position: absolute; left: 89.3mm; top: 9mm; width: 17.4mm; height: 26mm; }
.wheel img { display: block; width: 17.4mm; height: 26mm; }
.title { position: absolute; left: 6mm; right: 6mm; top: 43mm; height: 17mm; line-height: 17mm; text-align: center; font-size: 34pt; font-weight: bold; letter-spacing: 3.2mm; white-space: nowrap; overflow: hidden; }
.rule { position: absolute; left: 30mm; right: 30mm; top: 61mm; border-top: 0.35mm solid #1f2c5a; }
.subtitle { position: absolute; left: 8mm; right: 8mm; top: 63.5mm; text-align: center; font-size: 9.5pt; letter-spacing: 0.9mm; white-space: nowrap; overflow: hidden; }
.concern { position: absolute; left: 10mm; right: 10mm; top: 84mm; text-align: center; font-size: 13pt; letter-spacing: 1.6mm; }
.orn { position: absolute; left: 10mm; right: 10mm; top: 92mm; text-align: center; color: #c9a043; font-size: 9pt; letter-spacing: 1mm; }
.desc { position: absolute; left: 20mm; right: 20mm; top: 104mm; text-align: center; font-size: 12pt; line-height: 2.05; }
.body { position: absolute; left: 22mm; right: 22mm; top: 154mm; font-size: 14.5pt; line-height: 1; }
.body p { margin: 0 0 9.5mm; white-space: nowrap; }
.fill { display: inline-block; border-bottom: 0.3mm dashed #8a93b5; text-align: center; padding: 0 2mm 1mm; font-weight: bold; }
.sign { position: absolute; left: 14mm; top: 214mm; width: 60mm; border-top: 0.3mm dashed #8a93b5; padding-top: 3mm; font-size: 11.5pt; }
.foot { position: absolute; left: 14mm; right: 14mm; top: 233mm; font-size: 10.5pt; line-height: 1.75; }
.bottom { position: absolute; left: 14mm; right: 14mm; top: 268mm; border-top: 0.35mm solid #c9a043; padding-top: 2.5mm; text-align: center; font-size: 8.5pt; }
</style></head><body><div class="page">
<div class="top"><div class="l">Certificate Number:<span class="val" style="font-size:' . $num_pt . 'pt">' . $e($v['number']) . '</span></div>'
	. '<div class="r">Date of issuance:<span class="val">' . $e($v['issued']) . '</span></div></div>
<div class="wheel"><img src="' . dh_cert_wheel_src() . '" width="66" height="98"></div>
<div class="title" style="' . $title_css . '">' . $e($s['title']) . '</div><div class="rule"></div>
<div class="subtitle" style="' . $sub_css . '">' . $e($s['subtitle']) . '</div>
<div class="concern">TO WHOMSOEVER IT MAY CONCERN</div><div class="orn">• —— •</div>
<div class="desc">' . $lines($s['description']) . '</div>
<div class="body" style="font-size:' . $body_pt . 'pt">
<p>This is to certify that <span class="fill" style="min-width:95mm; font-size:' . $name_pt . 'pt">' . $e($v['name']) . '</span></p>
<p>' . $verb . ' <span class="fill" style="min-width:22mm">' . $e($v['days']) . '</span> day Vipassana Course organised</p>
<p>from <span class="fill" style="min-width:34mm">' . $e($v['from']) . '</span> to <span class="fill" style="min-width:34mm">' . $e($v['to']) . '</span> at ' . $e($s['centre_name']) . '.</p>
</div>
<div class="sign">Authorised Signatory</div>
<div class="foot">' . $foot . '</div>
<div class="bottom">' . $e($s['bottom']) . '</div>
</div></body></html>';
}

/* HTML -> PDF bytes, A4 with no margins (the navy border is part of the page). */
function dh_cert_pdf_bytes($html)
{
	$stub = tempnam('/tmp', 'DIPI-CERT');
	$in = $stub . '.html';
	$out = $stub . '.pdf';
	file_put_contents($in, $html);
	exec('/usr/local/bin/wkhtmltopdf --page-size A4 -T 0 -B 0 -L 0 -R 0 --disable-smart-shrinking --load-error-handling ignore --load-media-error-handling ignore '
		. escapeshellarg($in) . ' ' . escapeshellarg($out) . ' 2>/dev/null');
	$bytes = @file_get_contents($out);
	@unlink($in);
	@unlink($out);
	@unlink($stub);
	return $bytes;
}

function _dh_cert_send_pdf($bytes, $filename, $title)
{
	if (!$bytes)
	{
		drupal_set_message(t('The certificate could not be made. Please try again.'), 'error');
		return '';
	}
	header('Content-Type: application/pdf');
	header('Content-Disposition: inline; filename="' . $filename . '"');
	dh_pdf_echo_with_title($bytes, $title);
	drupal_exit();
}

/* certificate/{app id}: issue (or reprint) the certificate and show it as a PDF. */
function dh_certificate_pdf($app_id)
{
	$app = db_query("select a_id, a_f_name, a_m_name, a_l_name, a_attended, a_type, a_course, a_center from dh_applicant where a_id = :a",
		array(':a' => $app_id))->fetchAssoc();
	if (!dh_cert_eligible($app))
		return '<p>' . t('A certificate can be made only for a student or Dhamma server who attended the course, from Day 1 of the course, and not for Work Period, AT Workshop / Meeting or Other courses.') . '</p>';
	$cert = dh_cert_issue($app);
	if (!$cert)
	{
		drupal_set_message(t('A certificate number could not be given. Please try again.'), 'error');
		return '';
	}
	$course = db_query("select c_start, c_end, td_val2 from dh_course left join dh_type_detail on td_id = c_course_type where c_id = :c",
		array(':c' => $app['a_course']))->fetchAssoc();
	$v = array(
		'name' => dh_day0_name($app),
		'days' => (string) $course['td_val2'],
		'from' => date('j M Y', strtotime($course['c_start'])),
		'to' => date('j M Y', strtotime($course['c_end'])),
		'number' => $cert->cert_number,
		'issued' => date('j M Y', strtotime($cert->cert_issued)),
		'server' => ($app['a_type'] == 'Sevak'),
	);
	$s = dh_cert_settings($app['a_center']);
	$file = 'certificate-' . str_replace('/', '-', $cert->cert_number) . '.pdf';
	return _dh_cert_send_pdf(dh_cert_pdf_bytes(dh_cert_html($s, $v)), $file, 'Certificate ' . $cert->cert_number . ' ' . $v['name']);
}

/* centre/{c}/certificate/preview: a sample certificate with the centre's current text. */
function dh_certificate_preview($centre)
{
	$s = dh_cert_settings($centre);
	$v = array('name' => 'Sample Student Name', 'days' => '10', 'from' => date('j M Y', strtotime('-11 days')), 'to' => date('j M Y'),
		'number' => $s['prefix'] . '/' . date('Y') . '/0000', 'issued' => date('j M Y'), 'server' => !empty($_GET['server']));
	return _dh_cert_send_pdf(dh_cert_pdf_bytes(dh_cert_html($s, $v)), 'certificate-sample.pdf', 'Sample certificate');
}

/* centre/{c}/certificate: the centre's certificate text. */
function dh_cert_settings_form($form, &$form_state, $centre)
{
	$s = dh_cert_settings($centre);
	$name = db_query("select c_name from dh_center where c_id = :c", array(':c' => $centre))->fetchField();
	drupal_set_title(t('Certificate settings: @c', array('@c' => $name)));
	$form['#centre'] = $centre;
	$form['intro'] = array('#markup' => '<p>' . t('This text is printed on the course certificates of this centre (the "Certificate" action in search results).')
		. ($s['saved'] ? '' : ' <b>' . t('Not saved yet: the text below is made from the centre record. Please check it and save.') . '</b>')
		. '</p><p>' . l(t('Preview a sample certificate'), 'centre/' . $centre . '/certificate/preview', array('attributes' => array('target' => '_blank')))
		. ' &middot; ' . l(t('sample for a Dhamma server'), 'centre/' . $centre . '/certificate/preview', array('query' => array('server' => 1), 'attributes' => array('target' => '_blank')))
		. ' ' . t('(save first to see your changes)') . '</p>');
	$form['title'] = array('#type' => 'textfield', '#title' => t('Heading'), '#default_value' => $s['title'], '#required' => TRUE, '#maxlength' => 200,
		'#description' => t('Large heading, e.g. DHAMMA CAKKA'));
	$form['subtitle'] = array('#type' => 'textfield', '#title' => t('Line under the heading'), '#default_value' => $s['subtitle'], '#maxlength' => 200,
		'#description' => t('e.g. SARNATH · VIPASSANA MEDITATION CENTRE'));
	$form['description'] = array('#type' => 'textarea', '#title' => t('Centre description'), '#default_value' => $s['description'], '#rows' => 5,
		'#description' => t('Printed in the middle, centred. Each line here is a line on the certificate.'));
	$form['centre_name'] = array('#type' => 'textfield', '#title' => t('Centre name in the sentence'), '#default_value' => $s['centre_name'], '#required' => TRUE, '#maxlength' => 200,
		'#description' => t('Used in "... from 1 October to 12 October at <b>this name</b>."'));
	$form['footer'] = array('#type' => 'textarea', '#title' => t('Address and contact (bottom left)'), '#default_value' => $s['footer'], '#rows' => 5,
		'#description' => t('The first line is printed in bold.'));
	$form['bottom'] = array('#type' => 'textfield', '#title' => t('Last line (bottom of the page)'), '#default_value' => $s['bottom'], '#maxlength' => 300);
	$form['prefix'] = array('#type' => 'textfield', '#title' => t('Certificate number prefix'), '#default_value' => $s['prefix'], '#required' => TRUE, '#maxlength' => 12, '#size' => 14,
		'#description' => t('Letters and digits only. Numbers look like @n. Certificates already issued keep their number.', array('@n' => $s['prefix'] . '/' . date('Y') . '/0001')));
	$form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
	return $form;
}

function dh_cert_settings_form_validate($form, &$form_state)
{
	$centre = $form['#centre'];
	$p = strtoupper(trim($form_state['values']['prefix']));
	if (!preg_match('/^[A-Z0-9]{2,12}$/', $p))
		form_set_error('prefix', t('The prefix must be 2 to 12 letters or digits.'));
	else
	{
		// Another centre may not use the same prefix (saved, or by default from its subdomain).
		$taken = db_query("select certs_center from dh_certificate_setting where certs_prefix = :p and certs_center <> :c", array(':p' => $p, ':c' => $centre))->fetchField()
			|| db_query("select c_id from dh_center where upper(c_subdomain) = :p and c_id <> :c and c_id not in (select certs_center from dh_certificate_setting)", array(':p' => $p, ':c' => $centre))->fetchField()
			|| db_query("select 1 from dh_certificate where cert_number like :p and cert_center <> :c limit 1", array(':p' => db_like($p) . '/%', ':c' => $centre))->fetchField();
		if ($taken)
			form_set_error('prefix', t('Another centre already uses the prefix @p.', array('@p' => $p)));
	}
	$form_state['values']['prefix'] = $p;
}

function dh_cert_settings_form_submit($form, &$form_state)
{
	global $user;
	$v = $form_state['values'];
	db_merge('dh_certificate_setting')->key(array('certs_center' => $form['#centre']))->fields(array(
		'certs_prefix' => $v['prefix'], 'certs_title' => trim($v['title']), 'certs_subtitle' => trim($v['subtitle']),
		'certs_centre_name' => trim($v['centre_name']), 'certs_description' => trim($v['description']),
		'certs_footer' => trim($v['footer']), 'certs_bottom' => trim($v['bottom']),
		'certs_updated' => date('Y-m-d H:i:s'), 'certs_updated_by' => $user->uid,
	))->execute();
	drupal_set_message(t('Certificate settings saved.'));
}
