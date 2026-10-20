<?php
/**
 * Teacher bio data: the BIO DATA page of the AT acceptance letter (ATAL), with family members,
 * marital status and a photo. Filled by the teacher on My Profile or by the AT Book team on any
 * teacher's profile (the profile form, address-book.inc dh_atportal_profile). All optional.
 *
 * Kept in dh_teacher_bio (one row per teacher) and dh_teacher_family (dh_manageapp deploy
 * 2026-10-14-teacher-bio.sql); marital status and photo use dh_teacher's own t_martial_status
 * and t_photo. Changes go to dh_teacher_log as 'Teacher Updated', so they show in the profile's
 * Change Log next to the dh_teacher changes its trigger logs. There is no health note: the
 * senior teachers' review (2026-10-15) said health matters are not kept in DIPI, only discussed
 * in person, so the profile shows a short note instead.
 */

/* Bio fields stored in dh_teacher_bio, with the names shown in the Change Log. */
function dh_teacher_bio_labels()
{
	return array(
		'tb_birth_place' => 'Place of birth',
		'tb_nationality' => 'Nationality',
		'tb_first_course_year' => 'First course: year',
		'tb_first_course_month' => 'First course: month',
		'tb_first_course_by' => 'First course: conducted by',
		'tb_education' => 'Education',
		'tb_languages' => 'Languages known',
		'tb_occupation' => 'Business / job',
		'tb_activities' => 'Other activities and memberships',
		'tb_serve_time' => 'Time to serve courses (next 5 years)',
	);
}

function dh_teacher_marital_options()
{
	return array('Unmarried' => 'Unmarried', 'Married' => 'Married', 'Widowed' => 'Widowed', 'Divorced' => 'Divorced', 'Separated' => 'Separated');
}

function dh_teacher_relation_options()
{
	return array('Spouse', 'Son', 'Daughter', 'Father', 'Mother', 'Brother', 'Sister', 'Other');
}

/**
 * Date of birth as Day / Month / Year dropdowns (as on the schedule site's application forms),
 * in place of the calendar. Posts t_dob[day], t_dob[month], t_dob[year]; the newest year is first.
 * A saved year outside the list is added to it, so saving the profile never loses it.
 */
function dh_teacher_dob_field($dob, $weight)
{
	$d = $m = $y = '';
	if (preg_match('/^(\d{4})-(\d{2})-(\d{2})/', (string) $dob, $p) && $p[1] > 0)
		list(, $y, $m, $d) = array_map('intval', $p);
	$now = (int) date('Y');
	$years = drupal_map_assoc(range($now - 10, $now - 105));
	if ($y && !isset($years[$y]))
		$years[$y] = $y;
	krsort($years);
	$months = array();
	for ($i = 1; $i <= 12; $i++)
		$months[$i] = date('M', mktime(0, 0, 0, $i, 1, 2000));
	$part = array('#type' => 'select', '#title_display' => 'invisible');
	return array(
		'#type' => 'container', '#tree' => TRUE, '#weight' => $weight, '#attributes' => array('class' => array('dh-dob')),
		'label' => array('#markup' => '<label for="edit-t-dob-day">Date of Birth</label>'),
		'day' => $part + array('#title' => 'Day', '#options' => array('' => 'Day') + drupal_map_assoc(range(1, 31)), '#default_value' => $d),
		'month' => $part + array('#title' => 'Month', '#options' => array('' => 'Month') + $months, '#default_value' => $m),
		'year' => $part + array('#title' => 'Year', '#options' => array('' => 'Year') + $years, '#default_value' => $y),
	);
}

/* The posted date of birth as 'Y-m-d', NULL when none is chosen, or FALSE when it is not a whole, real date. */
function dh_teacher_dob_value($in)
{
	if (!is_array($in))
		return NULL;
	// a profile page opened before the dropdowns came still posts the calendar's t_dob[date]
	if (isset($in['date']) && !isset($in['year']))
		$in = preg_match('/^(\d{4})-(\d{1,2})-(\d{1,2})$/', trim($in['date']), $p) ? array('day' => $p[3], 'month' => $p[2], 'year' => $p[1]) : array();
	$d = isset($in['day']) ? (int) $in['day'] : 0;
	$m = isset($in['month']) ? (int) $in['month'] : 0;
	$y = isset($in['year']) ? (int) $in['year'] : 0;
	if (!$d && !$m && !$y)
		return NULL;
	if (!$d || !$m || !$y || !checkdate($m, $d, $y) || sprintf('%04d-%02d-%02d', $y, $m, $d) > date('Y-m-d'))
		return FALSE;
	return sprintf('%04d-%02d-%02d', $y, $m, $d);
}

/* The bio tables exist (they are created by a deploy step; until then the profile works as before). */
function dh_teacher_bio_ready()
{
	$ok = &drupal_static(__FUNCTION__);
	if (!isset($ok))
		$ok = db_table_exists('dh_teacher_bio') && db_table_exists('dh_teacher_family');
	return $ok;
}

/* The teacher behind the logged-in AT Portal account (user name "<t_code>.<t_gender>"), any status. */
function dh_teacher_own_id()
{
	global $user;
	$p = strrpos($user->name, '.');
	if (!$user->uid || $p === FALSE)
		return 0;
	return (int) db_query("select t_id from dh_teacher where t_code = :c and t_gender = :g limit 1",
		array(':c' => substr($user->name, 0, $p), ':g' => substr($user->name, $p + 1)))->fetchField();
}

/**
 * "Need help? Write to …" for the profile and the dashboard reminder. The address is a site setting
 * (variable dh_atportal_helpline, set with drush vset), not in the code; nothing shows while it is empty.
 */
function dh_teacher_helpline()
{
	$h = trim((string) variable_get('dh_atportal_helpline', ''));
	if ($h === '')
		return '';
	$link = valid_email_address($h) ? '<a href="mailto:'.check_plain($h).'">'.check_plain($h).'</a>' : check_plain($h);
	return '<span class="dh-helpline">'.t('Need help? Write to !h.', array('!h' => $link)).'</span>';
}

function dh_teacher_bio_load($t_id)
{
	$bio = array_fill_keys(array_keys(dh_teacher_bio_labels()), '');
	if ($t_id && dh_teacher_bio_ready())
	{
		$row = db_query("select * from dh_teacher_bio where tb_teacher = :t", array(':t' => $t_id))->fetchAssoc();
		if ($row)
			foreach ($bio as $k => $v)
				$bio[$k] = ($row[$k] === NULL) ? '' : (string) $row[$k];
	}
	return $bio;
}

function dh_teacher_family_load($t_id)
{
	if (!$t_id || !dh_teacher_bio_ready())
		return array();
	return db_query("select tf_name, tf_relation, tf_birth_year, tf_meditator, tf_courses from dh_teacher_family where tf_teacher = :t order by tf_id",
		array(':t' => $t_id))->fetchAll(PDO::FETCH_ASSOC);
}

/* One family member as a line for the Change Log. */
function _dh_teacher_family_line($r)
{
	$s = $r['tf_name'];
	$d = array_filter(array($r['tf_relation'], $r['tf_birth_year'] ? 'born '.$r['tf_birth_year'] : '',
		$r['tf_meditator'] ? 'meditator'.($r['tf_courses'] !== '' && $r['tf_courses'] !== NULL ? ', '.$r['tf_courses'].' courses' : '') : ''));
	return $d ? $s.' ('.implode(', ', $d).')' : $s;
}

function _dh_teacher_log_change($t_id, $old, $new)
{
	global $user;
	if (!$old && !$new)
		return;
	db_insert('dh_teacher_log')->fields(array('tl_teacher' => $t_id, 'tl_teacher_role' => 0, 'tl_event' => 'Teacher Updated',
		'tl_msg' => json_encode(array('old' => $old, 'new' => $new)), 'tl_user' => $user->uid))->execute();
}

/**
 * Clean the bio values posted by the form. Unknown keys are dropped; years and months are
 * numbers or empty; languages are names from dh_languages.
 */
function dh_teacher_bio_clean($input)
{
	$out = array();
	foreach (array_keys(dh_teacher_bio_labels()) as $k)
	{
		if (!array_key_exists($k, $input))
			continue;
		$v = $input[$k];
		if ($k == 'tb_languages')
			$v = implode(', ', array_filter(array_map('trim', is_array($v) ? array_values($v) : explode(',', (string) $v))));
		elseif ($k == 'tb_first_course_year' || $k == 'tb_first_course_month')
			$v = ((string) $v === '' || !ctype_digit((string) $v)) ? '' : (string) (int) $v;
		else
			$v = trim((string) $v);
		$out[$k] = $v;
	}
	return $out;
}

/** Save bio values (only the keys given) and log what changed. */
function dh_teacher_bio_save($t_id, $values)
{
	global $user;
	if (!$t_id || !dh_teacher_bio_ready())
		return;
	$labels = dh_teacher_bio_labels();
	$old = dh_teacher_bio_load($t_id);
	$new = $old;
	foreach (dh_teacher_bio_clean($values) as $k => $v)
		$new[$k] = $v;
	$lo = array(); $ln = array();
	foreach ($labels as $k => $label)
		if ((string) $old[$k] !== (string) $new[$k])
		{
			$lo[$label] = $old[$k];
			$ln[$label] = $new[$k];
		}
	if (!$lo)
		return;
	$fields = array();
	foreach ($labels as $k => $label)
		$fields[$k] = (in_array($k, array('tb_first_course_year', 'tb_first_course_month')) && $new[$k] === '') ? NULL : $new[$k];
	$fields['tb_updated'] = date('Y-m-d H:i:s');
	$fields['tb_updated_by'] = $user->uid;
	db_merge('dh_teacher_bio')->key(array('tb_teacher' => $t_id))->fields($fields)->execute();
	_dh_teacher_log_change($t_id, $lo, $ln);
}

/** Clean the family rows posted by the form (JSON from the family table). */
function dh_teacher_family_clean($json)
{
	$rows = json_decode((string) $json, TRUE);
	$out = array();
	if (!is_array($rows))
		return $out;
	foreach ($rows as $r)
	{
		$name = isset($r['name']) ? trim(mb_substr((string) $r['name'], 0, 100)) : '';
		if ($name === '')
			continue;
		$rel = isset($r['relation']) && in_array($r['relation'], dh_teacher_relation_options()) ? $r['relation'] : '';
		$by = isset($r['birth_year']) && ctype_digit((string) $r['birth_year']) && $r['birth_year'] >= 1900 && $r['birth_year'] <= date('Y') ? (string) (int) $r['birth_year'] : '';
		$med = !empty($r['meditator']) ? 1 : 0;
		$c = $med && isset($r['courses']) && ctype_digit((string) $r['courses']) ? (string) (int) $r['courses'] : '';
		$out[] = array('tf_name' => $name, 'tf_relation' => $rel, 'tf_birth_year' => $by, 'tf_meditator' => $med, 'tf_courses' => $c);
	}
	return $out;
}

/** Replace the family list when it changed, and log it. */
function dh_teacher_family_save($t_id, $rows)
{
	global $user;
	if (!$t_id || !dh_teacher_bio_ready())
		return;
	$old = dh_teacher_family_load($t_id);
	$norm = function ($list) {
		return array_map(function ($r) {
			return array('tf_name' => (string) $r['tf_name'], 'tf_relation' => (string) $r['tf_relation'], 'tf_birth_year' => (string) $r['tf_birth_year'],
				'tf_meditator' => (int) $r['tf_meditator'], 'tf_courses' => (string) $r['tf_courses']);
		}, $list);
	};
	if ($norm($old) === $norm($rows))
		return;
	$txn = db_transaction();
	db_delete('dh_teacher_family')->condition('tf_teacher', $t_id)->execute();
	foreach ($rows as $r)
		db_insert('dh_teacher_family')->fields(array('tf_teacher' => $t_id, 'tf_name' => $r['tf_name'], 'tf_relation' => $r['tf_relation'],
			'tf_birth_year' => $r['tf_birth_year'] === '' ? NULL : $r['tf_birth_year'], 'tf_meditator' => $r['tf_meditator'],
			'tf_courses' => $r['tf_courses'] === '' ? NULL : $r['tf_courses'], 'tf_updated' => date('Y-m-d H:i:s'), 'tf_updated_by' => $user->uid))->execute();
	_dh_teacher_log_change($t_id, array('Family members' => implode('; ', array_map('_dh_teacher_family_line', $old))),
		array('Family members' => implode('; ', array_map('_dh_teacher_family_line', $rows))));
}

/* The items counted in "Bio data: X of 12 filled". */
function dh_teacher_bio_completeness($t, $bio)
{
	$items = array(
		'Date of birth' => !empty($t['t_dob']) && $t['t_dob'] != '0000-00-00',
		'Mobile' => trim((string) $t['t_mob_phone']) !== '',
		'Email' => trim((string) $t['t_email']) !== '',
		'Marital status' => trim((string) $t['t_martial_status']) !== '',
		'Photo' => trim((string) $t['t_photo']) !== '',
		'Place of birth' => $bio['tb_birth_place'] !== '',
		'Nationality' => $bio['tb_nationality'] !== '',
		'First course' => $bio['tb_first_course_year'] !== '',
		'First course: conducted by' => $bio['tb_first_course_by'] !== '',
		'Education' => $bio['tb_education'] !== '',
		'Languages known' => $bio['tb_languages'] !== '',
		'Business / job' => $bio['tb_occupation'] !== '',
	);
	return array('filled' => count(array_filter($items)), 'total' => count($items), 'missing' => array_keys(array_diff($items, array(TRUE))));
}

/** Save the photo chosen on the form (already cropped by the photo editor). */
function dh_teacher_photo_save($t_id, $file)
{
	global $user;
	$image = image_load($file->uri);
	if ($image)
	{
		image_scale($image, 800);
		image_save($image);
	}
	$ext = strtolower(pathinfo($file->filename, PATHINFO_EXTENSION));
	if (!in_array($ext, array('jpg', 'jpeg', 'png')))
		$ext = 'jpg';
	$dir = 'private://teacher-photo';
	file_prepare_directory($dir, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);
	$uri = "$dir/t-$t_id.$ext";
	// a different extension from last time leaves no stale file behind
	foreach (array('jpg', 'jpeg', 'png') as $e)
		if ($e != $ext && file_exists(drupal_realpath("$dir/t-$t_id.$e")))
			file_unmanaged_delete("$dir/t-$t_id.$e");
	if (!file_unmanaged_copy($file->uri, $uri, FILE_EXISTS_REPLACE))
		return FALSE;
	// a copy in S3 as for applicant photos (the server's private files are not backed up there)
	if (function_exists('s3_put_file') && variable_get('s3_bucket_name', ''))
	{
		$ret = s3_put_file(variable_get('s3_bucket_name', ''), drupal_realpath($uri), str_replace('private://', '', $uri));
		if (!$ret['success'])
			watchdog('S3TeacherPhoto', $ret['message']);
	}
	db_update('dh_teacher')->fields(array('t_photo' => $uri, 't_updated' => date('Y-m-d H:i:s'), 't_updated_by' => $user->uid))->condition('t_id', $t_id)->execute();
	return $uri;
}

/**
 * Who may see a teacher's photo: the teacher (me), the AT Book team, and anyone who can open the
 * AT Address Book (teachers and centres), for the teachers the Address Book lists to them.
 */
function dh_teacher_photo_access($id)
{
	if ($id === 'me')
		return user_access('access at profile');
	if (!ctype_digit((string) $id))
		return FALSE;
	if (user_access('at portal superadmin') || user_access('access any profile'))
		return TRUE;
	if (!user_access('access at address book'))
		return FALSE;
	// the Address Book hides teachers in Training from those who cannot add teachers
	$status = db_query("select t_status from dh_teacher where t_id = :t", array(':t' => (int) $id))->fetchField();
	return $status !== FALSE && ($status != 'Training' || user_access('add assistant teacher'));
}

/** at-portal/teacher-photo/{me|t_id}: the teacher's photo (see dh_teacher_photo_access). */
function dh_teacher_photo_page($id)
{
	$t_id = ($id === 'me') ? dh_teacher_own_id() : (int) $id;
	$uri = $t_id ? db_query("select t_photo from dh_teacher where t_id = :t", array(':t' => $t_id))->fetchField() : '';
	if (!$uri)
		return MENU_NOT_FOUND;
	$ext = strtolower(pathinfo($uri, PATHINFO_EXTENSION));
	$type = $ext == 'png' ? 'image/png' : 'image/jpeg';
	$path = drupal_realpath($uri);
	if ($path && is_file($path))
		$data = file_get_contents($path);
	else
	{
		$ret = s3_get_file(variable_get('s3_bucket_name', ''), str_replace('private://', '', $uri), 'stream');
		if (!$ret['success'])
			return MENU_NOT_FOUND;
		$data = $ret['data'];
	}
	drupal_add_http_header('Content-Type', $type);
	drupal_add_http_header('Cache-Control', 'private, max-age=300');
	print $data;
	drupal_exit();
}

/**
 * Add the bio-data parts to the profile form: personal extras (marital status, place of birth,
 * nationality, photo), first course, family, and background and service.
 */
function dh_teacher_bio_form(&$form, $t_id, $edit_data, $self)
{
	if (!dh_teacher_bio_ready())
		return;
	$bio = dh_teacher_bio_load($t_id);
	$family = dh_teacher_family_load($t_id);
	$c = dh_teacher_bio_completeness($edit_data + array('t_dob' => '', 't_mob_phone' => '', 't_email' => '', 't_martial_status' => '', 't_photo' => ''), $bio);
	$form['bio_status'] = array('#markup' => '<div class="dh-bio-status'.($c['filled'] == $c['total'] ? ' dh-bio-done' : '').'">'
		.t('Bio data: @f of @t filled.', array('@f' => $c['filled'], '@t' => $c['total']))
		.($c['missing'] ? ' '.t('Still empty: @m.', array('@m' => implode(', ', $c['missing']))) : '').' '.t('Everything is optional.')
		.($self ? ' '.dh_teacher_helpline() : '').'</div>', '#weight' => 0.5);

	$g = 'Personal Information';
	$form[$g]['t_martial_status'] = array('#title' => 'Marital status', '#type' => 'select', '#options' => dh_teacher_marital_options(),
		'#empty_option' => '- choose -', '#default_value' => isset($edit_data['t_martial_status']) ? $edit_data['t_martial_status'] : '', '#weight' => 4.1);
	$form[$g]['tb_birth_place'] = array('#title' => 'Place of birth', '#type' => 'textfield', '#maxlength' => 100, '#default_value' => $bio['tb_birth_place'], '#weight' => 4.2);
	$nat = db_query("select distinct c_nationality from dh_country where c_nationality <> '' order by c_nationality")->fetchCol();
	$nat = array_combine($nat, $nat);
	if ($bio['tb_nationality'] !== '' && !isset($nat[$bio['tb_nationality']]))
		$nat[$bio['tb_nationality']] = $bio['tb_nationality'];
	$form[$g]['tb_nationality'] = array('#title' => 'Nationality', '#type' => 'select', '#options' => $nat, '#empty_option' => '- choose -',
		'#default_value' => $bio['tb_nationality'], '#weight' => 4.3);
	// the photo has its own section, right after Personal Information
	$form['photo'] = array('#type' => 'fieldset', '#title' => 'Photo', '#weight' => 1.5);
	$photo = '';
	if (!empty($edit_data['t_photo']))
		$photo = '<img class="dh-bio-photo" alt="'.t('Photo').'" src="'.url('at-portal/teacher-photo/'.($self ? 'me' : $t_id), array('query' => array('v' => substr(md5((string) $edit_data['t_updated']), 0, 8)))).'">';
	$form['photo']['photo_now'] = array('#markup' => '<div class="dh-bio-photo-wrap">'.($photo ? $photo : '<div class="dh-bio-nophoto">'.t('No photo yet').'</div>').'</div>');
	$form['photo']['upload_photo'] = array('#title' => $photo ? 'Change photo' : 'Add a photo', '#type' => 'file',
		'#description' => t('A clear photo of your face, JPG or PNG. You can crop it before saving.'));

	$form['dhamma'] = array('#type' => 'fieldset', '#title' => 'Dhamma', '#weight' => 30);
	$years = array();
	for ($y = (int) date('Y'); $y >= 1955; $y--)
		$years[$y] = $y;
	$months = array();
	for ($m = 1; $m <= 12; $m++)
		$months[$m] = date('F', mktime(0, 0, 0, $m, 1));
	$form['dhamma']['tb_first_course_year'] = array('#title' => 'First 10-day course: year', '#type' => 'select', '#options' => $years, '#empty_option' => '- year -', '#default_value' => $bio['tb_first_course_year']);
	$form['dhamma']['tb_first_course_month'] = array('#title' => 'Month (if you remember)', '#type' => 'select', '#options' => $months, '#empty_option' => '- month -', '#default_value' => $bio['tb_first_course_month']);
	$form['dhamma']['tb_first_course_by'] = array('#title' => 'Conducted by', '#type' => 'textfield', '#maxlength' => 150, '#default_value' => $bio['tb_first_course_by']);

	$form['family'] = array('#type' => 'fieldset', '#title' => 'Family members', '#weight' => 31,
		'#description' => t('Name, relation, year of birth, and if they meditate, how many courses they have done.'));
	$form['family']['family_json'] = array('#type' => 'hidden', '#default_value' => json_encode(array_map(function ($r) {
		return array('name' => $r['tf_name'], 'relation' => $r['tf_relation'], 'birth_year' => (string) $r['tf_birth_year'], 'meditator' => (int) $r['tf_meditator'], 'courses' => (string) $r['tf_courses']);
	}, $family)), '#attributes' => array('id' => 'dh-family-json'));
	$form['family']['table'] = array('#markup' => '<div id="dh-family" data-relations="'.check_plain(json_encode(dh_teacher_relation_options())).'"></div>');

	$form['more'] = array('#type' => 'fieldset', '#title' => 'Background and service', '#weight' => 32);
	$langs = db_query("select l_name from dh_languages order by l_name")->fetchCol();
	$langs = array_combine($langs, $langs);
	$sel = array_filter(array_map('trim', explode(',', $bio['tb_languages'])));
	foreach ($sel as $l)
		if (!isset($langs[$l]))
			$langs[$l] = $l;
	$form['more']['tb_languages'] = array('#title' => 'Languages known', '#type' => 'select', '#options' => $langs, '#multiple' => TRUE,
		'#default_value' => array_combine($sel, $sel), '#attributes' => array('id' => 'edit-tb-languages'));
	$form['more']['tb_education'] = array('#title' => 'Education', '#type' => 'textarea', '#rows' => 2, '#default_value' => $bio['tb_education']);
	$form['more']['tb_occupation'] = array('#title' => 'Details of business / job', '#type' => 'textarea', '#rows' => 2, '#default_value' => $bio['tb_occupation']);
	$form['more']['tb_activities'] = array('#title' => 'Other activities and memberships', '#type' => 'textarea', '#rows' => 2, '#default_value' => $bio['tb_activities'],
		'#description' => t('Social, sports, political or other activities, and organisations or clubs you are a member of.'));
	$form['more']['tb_serve_time'] = array('#title' => 'Time you can give for serving courses over the next five years', '#type' => 'textarea', '#rows' => 2, '#default_value' => $bio['tb_serve_time']);

	// in place of a health note (not kept in DIPI): the senior teachers' wording
	if ($self)
		$form['more']['health_note'] = array('#markup' => '<p class="dh-bio-note">'
			.t('Any health-related matters can be discussed with your CAT, or with the conducting teacher when you sit a course.').'</p>');

	drupal_add_js(drupal_get_path('module', 'dh_atportal').'/js/bio.js');
	drupal_add_css(drupal_get_path('module', 'dh_atportal').'/css/bio.css');
	$mp = drupal_get_path('module', 'dh_manageapp');
	drupal_add_css($mp.'/libraries/cropperjs/cropper.min.css');
	drupal_add_css($mp.'/css/app-photo.css');
	drupal_add_js($mp.'/libraries/cropperjs/cropper.min.js');
	drupal_add_js($mp.'/js/app-photo.js');
	drupal_add_js(array('dhAppPhoto' => array('picoPath' => base_path().$mp.'/libraries/picojs/')), 'setting');
}

/** Dashboard box for an Active teacher whose bio data is not complete. */
function dh_teacher_bio_reminder()
{
	if (!dh_teacher_bio_ready())
		return '';
	$t_id = dh_teacher_own_id();
	if (!$t_id)
		return '';
	$t = db_query("select * from dh_teacher where t_id = :t and t_status = 'Active'", array(':t' => $t_id))->fetchAssoc();
	if (!$t)
		return '';
	$c = dh_teacher_bio_completeness($t, dh_teacher_bio_load($t_id));
	if ($c['filled'] == $c['total'])
		return '';
	drupal_add_css(drupal_get_path('module', 'dh_atportal').'/css/bio.css');
	return '<div class="dh-bio-reminder">'.t('Please complete your bio data (@f of @t filled).', array('@f' => $c['filled'], '@t' => $c['total']))
		.' '.l(t('Open My Profile'), 'at-portal/address-book/edit/me').' '.dh_teacher_helpline().'</div>';
}
