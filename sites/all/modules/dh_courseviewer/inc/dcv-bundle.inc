<?php
/**
 * @file
 * DCV bundle assembly: seat-config geometry, curated students, seats.
 */

/**
 * Parse cs_seat_config INI into structured sections for the app to render.
 * Emits only [RIGHT] (Male) and [LEFT] (Female). Group overrides use the
 * GROUP<n>-<Key> convention. Pure — unit tested.
 */
function dcv_parse_seat_config($ini_string, $naming_conv = 0) {
  $raw = parse_ini_string($ini_string, true, INI_SCANNER_RAW);
  if ($raw === false) { return array(); }
  $key_map = array(
    'SeatsPerRow'       => 'seats_per_row',
    'SeatsPerRowChowky' => 'seats_per_row_chowky',
    'SeatDirection'     => 'seat_direction',
    'ChowkyPosition'    => 'chowky_position',
    'EmptySeats'        => 'empty_seats',
  );
  $int_keys = array('seats_per_row', 'seats_per_row_chowky');
  $sides = array('RIGHT' => array('M', 'Male'), 'LEFT' => array('F', 'Female'));
  $out = array();
  foreach ($sides as $key => $gl) {
    if (!isset($raw[$key]) || !is_array($raw[$key])) { continue; }
    $s = $raw[$key];
    $sec = array(
      'key' => $key, 'gender' => $gl[0], 'label' => $gl[1],
      'seats_per_row' => 0, 'seats_per_row_chowky' => 0,
      'seat_direction' => '', 'chowky_position' => '', 'empty_seats' => '',
      'naming_conv' => (int) $naming_conv,
      'groups' => array(),
    );
    $groups = array();
    foreach ($s as $k => $v) {
      if (preg_match('/^GROUP(\d+)-(.+)$/', $k, $m)) {
        $n = (int) $m[1];
        if (!isset($groups[$n])) { $groups[$n] = array('n' => $n); }
        if (isset($key_map[$m[2]])) {
          $field = $key_map[$m[2]];
          $groups[$n][$field] = in_array($field, $int_keys) ? (int) $v : $v;
        }
      }
      elseif (isset($key_map[$k])) {
        $field = $key_map[$k];
        $sec[$field] = in_array($field, $int_keys) ? (int) $v : $v;
      }
    }
    ksort($groups);
    $sec['groups'] = array_values($groups);
    $out[] = $sec;
  }
  return $out;
}

/**
 * Read a student photo (Drupal file uri, e.g. private://...), downscale it to a
 * small thumbnail and return it as a base64 data URI for offline embedding in the
 * bundle. Returns '' when the file is missing/unreadable (e.g. on a dev box whose
 * private files live only on prod/S3) or GD is unavailable — the app then shows a
 * placeholder. Max edge ~220px, JPEG q70 to keep the bundle small.
 */
function dcv_photo_data_uri($uri) {
  if (!is_string($uri) || $uri === '' || !extension_loaded('gd')) {
    return '';
  }
  $bytes = dcv_read_file_bytes($uri);
  if ($bytes === '' || $bytes === false) {
    return '';
  }
  $src = @imagecreatefromstring($bytes);
  if (!$src) {
    return '';
  }
  $w = imagesx($src); $h = imagesy($src);
  $max = 220;
  $scale = min(1.0, $max / max($w, $h));
  $nw = max(1, (int) round($w * $scale)); $nh = max(1, (int) round($h * $scale));
  $dst = imagecreatetruecolor($nw, $nh);
  imagecopyresampled($dst, $src, 0, 0, 0, 0, $nw, $nh, $w, $h);
  ob_start();
  imagejpeg($dst, NULL, 70);
  $data = ob_get_clean();
  imagedestroy($src); imagedestroy($dst);
  return 'data:image/jpeg;base64,' . base64_encode($data);
}

/**
 * Raw bytes of a Drupal file uri. Local filesystem first (dev / sites whose private
 * files are on disk); falls back to S3 via dh_manageapp's s3_get_file() for prod,
 * where files live on S3 and are referenced as private://. Returns '' if unreadable.
 */
function dcv_read_file_bytes($uri) {
  $path = drupal_realpath($uri);
  if ($path && is_readable($path)) {
    $b = @file_get_contents($path);
    if ($b !== false) { return $b; }
  }
  if (!function_exists('s3_get_file') && module_exists('dh_manageapp')) {
    module_load_include('inc', 'dh_manageapp', 'inc/dana-s3');
  }
  if (function_exists('s3_get_file')) {
    $bucket = variable_get('s3_bucket_name', '');
    if ($bucket !== '') {
      $ret = s3_get_file($bucket, file_uri_target($uri), 'file');
      if (!empty($ret['success']) && !empty($ret['data'])) {
        $b = @file_get_contents($ret['data']);
        @unlink($ret['data']);
        if ($b !== false) { return $b; }
      }
    }
  }
  return '';
}

/**
 * Full Dipi application view for every attending student, keyed by id.
 * Mirrors dh_application_view (search.inc) field set; grouped JSON; read-only.
 */
function dcv_course_students($course_id) {
  $course_id = (int) $course_id;
  $cell_plan = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'cell')   : 'main';
  $din_plan  = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'dining') : 'main';
  $col_cell = ($cell_plan === 'group') ? 'aa_group_cell'   : 'aa_cell';
  $col_din  = ($din_plan  === 'group') ? 'aa_group_dining' : 'aa_dining';

  $sql = "select
      a_id,
      a_conf_no, concat_ws(' ', nullif(trim(a_f_name),''), nullif(trim(a_m_name),''), nullif(trim(a_l_name),'')) as fullname,
      a_gender, a_status, a_dob, TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) as age,
      a_old, a_monk, a_alist, a_photo, a_type,
      a_education, a_occupation, a_occupation_past, a_company, a_designation, a_department,
      a_willing_to_serve,
      a_passport, a_voter_id, a_pancard, a_aadhar,
      a_address, a_zip, a_email, a_phone_home, a_phone_office, a_phone_mobile,
      a_emergency_name, a_emergency_relation, a_emergency_num,
      a_friend_family_details, a_internal_note, a_extra, a_learn_about_vip, a_purpose,
      a_lang_1, a_lang_2, a_lang_3, a_langs, a_lang_discourse, a_created, a_center,
      ci.c_name as city, a_city_str, s.s_name as state, co.c_name as country,
      c.c_name as course_name, td.td_key as course_type_key,
      td.td_val1 as course_type_label, td.td_val2 as course_days,
      ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_service,
      CONCAT_WS('-', NULLIF(ac_first_year,''), NULLIF(ac_first_month,''), NULLIF(ac_first_day,'')) as first_course_date,
      ac_first_location_str as first_course_location, ac_first_teacher_str as first_course_teacher,
      CONCAT_WS('-', NULLIF(ac_last_year,''), NULLIF(ac_last_month,''), NULLIF(ac_last_day,'')) as last_course_date,
      ac_last_location_str as last_course_location, ac_last_teacher_str as last_course_teacher,
      ac_practice_details, ac_teacher,
      ae_desc_physical, ae_desc_mental, ae_desc_medication,
      CONCAT(IFNULL(ae_desc_other_technique,''),' ',IFNULL(ae_desc_other_technique_old,'')) as other_technique,
      ae_desc_addiction_current, ae_pregnant, ae_pregnant_detail, ae_id_issued, ae_id_issued_by,
      ae_nationality, ae_father, ae_mother, ae_parent_course, ae_teen_std,
      aa_section, aa_acco, aa_cell, aa_group, aa_backrest, aa_chowky, aa_chair, aa_comment,
      $col_cell as eff_cell, $col_din as eff_dining,
      COALESCE((select l.l_name from dh_languages l
                where lower(l.l_code)=lower(a_lang_discourse) or l.l_name=a_lang_discourse
                limit 1), a_lang_discourse) as lang_discourse,
      al_committed, al_committed_details, al_exclusive_2yrs, al_daily_practice_yrs,
      al_daily_practice_details, al_5_precepts, al_intoxicants, al_sexual_misconduct,
      al_recent_sat_type, al_last_lc_type, al_last_lc_date, al_last_lc_location, al_last_lc_teacher,
      al_served_other, al_relationship, al_spouse_name, al_spouse_meditator,
      al_left_course, al_left_course_details, al_reduce_practice, al_personal_tragedy,
      al_difficulty, al_special_req, al_special_req_details, al_additional_info, al_arrival,
      al_exclusive_2yrs_details, al_5_precepts_details, al_reduce_practice_details,
      al_personal_tragedy_details, al_difficulty_details, al_relationship_lifelong,
      al_relation_harmonious, al_spouse_approve, al_spouse_other_technique,
      al_recommending, al_recommending_approved, al_area_at, al_area_at_approved, a_referral
    from dh_applicant
      left join dh_applicant_course   ac on a_id = ac_applicant
      left join dh_applicant_attended aa on a_id = aa_applicant
      left join dh_applicant_extra    ae on a_id = ae_applicant
      left join dh_applicant_lc       al on a_id = al_applicant
      left join dh_course c on a_course = c.c_id
      left join dh_type_detail td on c.c_course_type = td.td_id
      left join dh_country co on a_country = co.c_code
      left join dh_city ci on a_city = ci.c_id
      left join dh_state s on (a_state = s.s_code and a_country = s.s_country)
    where a_course = :c and a_attended = 1 and a_type = 'Student'";

  $S = function($v) { return is_null($v) ? '' : (string) $v; };
  $I = function($v) { return (int) $v; };
  $B = function($v) { return (bool) $v; };
  $out = array();
  foreach (db_query($sql, array(':c' => $course_id)) as $r) {
    $acco = trim($S($r->aa_section) . '-' . $S($r->aa_acco), '-');
    $short = $I($r->ac_teen) + $I($r->ac_10d) + $I($r->ac_stp);
    $long  = $I($r->ac_20d) + $I($r->ac_30d) + $I($r->ac_45d) + $I($r->ac_60d) + $I($r->ac_tsc) + $I($r->ac_spl);
    $out[(string) $r->a_id] = array(
      'identity' => array(
        'name' => $S($r->fullname), 'conf_no' => $S($r->a_conf_no), 'gender' => $S($r->a_gender),
        'status' => $S($r->a_status), 'dob' => $S($r->a_dob), 'age' => is_null($r->age) ? null : $I($r->age),
        'old_student' => $B($r->a_old), 'monk' => $B($r->a_monk), 'a_list' => $B($r->a_alist),
        'type' => $S($r->a_type), 'photo' => $S($r->a_photo),
        'photo_data' => dcv_photo_data_uri($r->a_photo), 'willing_to_serve' => $B($r->a_willing_to_serve),
        'education' => $S($r->a_education), 'occupation' => $S($r->a_occupation),
        'occupation_past' => $S($r->a_occupation_past), 'company' => $S($r->a_company),
        'designation' => $S($r->a_designation), 'department' => $S($r->a_department),
        'nationality' => $S($r->ae_nationality),
      ),
      'ids' => array(
        'passport' => $S($r->a_passport), 'voter_id' => $S($r->a_voter_id),
        'pancard' => $S($r->a_pancard), 'aadhar' => $S($r->a_aadhar),
        'id_issued' => $S($r->ae_id_issued), 'id_issued_by' => $S($r->ae_id_issued_by),
      ),
      'contact' => array(
        'address' => $S($r->a_address), 'zip' => $S($r->a_zip), 'email' => $S($r->a_email),
        'phone_home' => $S($r->a_phone_home), 'phone_office' => $S($r->a_phone_office),
        'phone_mobile' => $S($r->a_phone_mobile), 'emergency_name' => $S($r->a_emergency_name),
        'emergency_relation' => $S($r->a_emergency_relation), 'emergency_num' => $S($r->a_emergency_num),
        'city' => $S($r->city), 'city_str' => $S($r->a_city_str), 'state' => $S($r->state),
        'country' => $S($r->country),
      ),
      'languages' => array(
        'lang_1' => $S($r->a_lang_1), 'lang_2' => $S($r->a_lang_2), 'lang_3' => $S($r->a_lang_3),
        'langs' => $S($r->a_langs), 'lang_discourse' => $S($r->lang_discourse),
      ),
      'course' => array(
        'course_name' => $S($r->course_name), 'course_type_key' => $S($r->course_type_key),
        'course_type_label' => $S($r->course_type_label), 'course_days' => $S($r->course_days),
        'created' => $S($r->a_created), 'centre' => $I($r->a_center),
      ),
      'meditation' => array(
        'short_courses' => $short, 'long_courses' => $long,
        'teen' => $I($r->ac_teen), '10d' => $I($r->ac_10d), 'stp' => $I($r->ac_stp), 'spl' => $I($r->ac_spl),
        '20d' => $I($r->ac_20d), '30d' => $I($r->ac_30d), '45d' => $I($r->ac_45d), '60d' => $I($r->ac_60d),
        'tsc' => $I($r->ac_tsc), 'service' => $I($r->ac_service),
        'first_course_date' => $S($r->first_course_date), 'first_course_location' => $S($r->first_course_location),
        'first_course_teacher' => $S($r->first_course_teacher),
        'last_course_date' => $S($r->last_course_date), 'last_course_location' => $S($r->last_course_location),
        'last_course_teacher' => $S($r->last_course_teacher),
        'practice_details' => $S($r->ac_practice_details), 'other_technique' => trim($S($r->other_technique)),
        'is_teacher' => $B($r->ac_teacher),
      ),
      'health' => array(
        'physical' => $S($r->ae_desc_physical), 'mental' => $S($r->ae_desc_mental),
        'medication' => $S($r->ae_desc_medication), 'addiction_current' => $S($r->ae_desc_addiction_current),
        'pregnant' => $B($r->ae_pregnant), 'pregnant_detail' => $S($r->ae_pregnant_detail),
      ),
      'family' => array(
        'father' => $S($r->ae_father), 'mother' => $S($r->ae_mother),
        'parent_course' => $S($r->ae_parent_course), 'teen_std' => $S($r->ae_teen_std),
      ),
      'seating' => array(
        'section' => $S($r->aa_section), 'room' => $acco, 'cell' => $S($r->eff_cell),
        'dining' => $S($r->eff_dining), 'group' => $I($r->aa_group),
        'backrest' => $B($r->aa_backrest), 'chowky' => $B($r->aa_chowky), 'chair' => $B($r->aa_chair),
        'comment' => $S($r->aa_comment),
      ),
      'notes' => array(
        // Legacy rows store a literal "0" placeholder in this field — treat it as blank.
        'friend_family_details' => ($S($r->a_friend_family_details) === '0') ? '' : $S($r->a_friend_family_details),
        'internal_note' => $S($r->a_internal_note),
        // a_extra is the printed-form question "current family relationships and mental state".
        'extra' => $S($r->a_extra),
        'learn_about_vip' => $S($r->a_learn_about_vip),
        'purpose' => $S($r->a_purpose),
      ),
      'lc' => array(
        'committed' => $S($r->al_committed), 'committed_details' => $S($r->al_committed_details),
        'exclusive_2yrs' => $S($r->al_exclusive_2yrs), 'daily_practice_yrs' => $S($r->al_daily_practice_yrs),
        'daily_practice_details' => $S($r->al_daily_practice_details), 'five_precepts' => $S($r->al_5_precepts),
        'intoxicants' => $S($r->al_intoxicants), 'sexual_misconduct' => $S($r->al_sexual_misconduct),
        'recent_sat_type' => $S($r->al_recent_sat_type), 'last_lc_type' => $S($r->al_last_lc_type),
        'last_lc_date' => $S($r->al_last_lc_date), 'last_lc_location' => $S($r->al_last_lc_location),
        'last_lc_teacher' => $S($r->al_last_lc_teacher), 'served_other' => $S($r->al_served_other),
        'relationship' => $S($r->al_relationship), 'spouse_name' => $S($r->al_spouse_name),
        'spouse_meditator' => $S($r->al_spouse_meditator), 'left_course' => $S($r->al_left_course),
        'left_course_details' => $S($r->al_left_course_details), 'reduce_practice' => $S($r->al_reduce_practice),
        'personal_tragedy' => $S($r->al_personal_tragedy), 'difficulty' => $S($r->al_difficulty),
        'special_req' => $S($r->al_special_req), 'special_req_details' => $S($r->al_special_req_details),
        'additional_info' => $S($r->al_additional_info), 'arrival' => $S($r->al_arrival),
        'exclusive_2yrs_details' => $S($r->al_exclusive_2yrs_details),
        'five_precepts_details' => $S($r->al_5_precepts_details),
        'reduce_practice_details' => $S($r->al_reduce_practice_details),
        'personal_tragedy_details' => $S($r->al_personal_tragedy_details),
        'difficulty_details' => $S($r->al_difficulty_details),
        'relationship_lifelong' => $S($r->al_relationship_lifelong),
        'relation_harmonious' => $S($r->al_relation_harmonious),
        'spouse_approve' => $S($r->al_spouse_approve),
        'spouse_other_technique' => $S($r->al_spouse_other_technique),
      ),
      // Long-course recommendation / area-teacher approval (shown for LC applicants).
      'lc_approval' => array(
        'reco_teacher' => $S($r->al_recommending), 'reco_status' => $S($r->al_recommending_approved),
        'area_teacher' => $S($r->al_area_at), 'area_status' => $S($r->al_area_at_approved),
      ),
      // Related application-view sections (mirror dh_application_view AJAX panels).
      'referral'         => dcv_student_referral($r->a_referral, $r->course_type_key, $r->a_type),
      'reviews'          => dcv_student_reviews($r->a_id),
      'previous_courses' => dcv_student_previous_courses($r->a_id),
      'activity'         => dcv_student_activity($r->a_id),
      'clarifications'   => dcv_student_clarifications($r->a_id),
    );
  }
  return $out;
}

/**
 * Referral block for an applicant, mirroring dh_application_view: only returned
 * when the referral is still valid (today <= r_end) AND its listed level matches
 * the course level. Returns null otherwise (app treats null as "no referral").
 */
function dcv_student_referral($referral_id, $course_type_key, $applicant_type) {
  if (empty($referral_id)) { return NULL; }
  $ref = db_query("select concat(ifnull(t_f_name,''),' ',ifnull(t_l_name,''),' (',t_code,')') as teacher, r.*
                   from dh_referral r left join dh_teacher t on r.r_teacher=t.t_id where r_id=:id",
    array(':id' => (int) $referral_id))->fetchAssoc();
  if (!$ref) { return NULL; }
  if (!empty($ref['r_end']) && date_create() > date_create($ref['r_end'])) { return NULL; }
  $level = array('10-Day'=>1,'10-DayExecutive'=>1,'Satipatthana'=>2,'10-DaySpecial'=>3,'TSC'=>4,
                 '20-DayOSC'=>5,'30-DayOSC'=>6,'45-DayOSC'=>7,'60-DayOSC'=>8);
  $cl = isset($level[$course_type_key]) ? $level[$course_type_key] : 0;
  $type = strtolower((string) $applicant_type);
  $show = 0;
  if ($ref['r_listed_10d']) { $show = 1; }
  elseif ($ref['r_listed_10d_seva'] && $type == 'sevak') { $show = 1; }
  elseif ($ref['r_listed_stp'] && $cl >= 2) { $show = 1; }
  elseif ($ref['r_listed_spl'] && $cl >= 3) { $show = 1; }
  elseif ($ref['r_listed_tsc'] && $cl >= 4) { $show = 1; }
  elseif ($ref['r_listed_20d'] && $cl >= 5) { $show = 1; }
  elseif ($ref['r_listed_30d'] && $cl >= 6) { $show = 1; }
  elseif ($ref['r_listed_45d'] && $cl >= 7) { $show = 1; }
  elseif ($ref['r_listed_60d'] && $cl >= 8) { $show = 1; }
  elseif ($ref['r_listed_lc_seva'] && $type == 'sevak' && $cl >= 3) { $show = 1; }
  if (!$show) { return NULL; }
  $listed_for = '';
  foreach (array('10d','stp','spl','20d','30d','45d','60d','tsc','10d_seva','lc_seva','longseva') as $a) {
    if ($ref['r_listed_'.$a]) { $listed_for .= strtoupper($a).' | '; }
  }
  if ($ref['r_center']) {
    $title = 'Center RefListed by'; $by = (string) $ref['r_listed_by']; $special = 'Not Applicable';
  }
  else {
    $title = 'Referring AT'; $by = (string) $ref['teacher']; $special = $ref['r_special'] ? 'Yes' : 'No';
  }
  return array(
    'listed_title' => $title, 'listed_by' => $by, 'listed_for' => trim($listed_for, ' | '),
    'reason' => trim(strip_tags((string) $ref['r_reason'])),
    'start' => (string) $ref['r_start'], 'end' => (string) $ref['r_end'], 'special_list' => $special,
  );
}

/** Clarification messages/uploads for an applicant (dh_applicant_clarification). */
function dcv_student_clarifications($a_id) {
  $out = array();
  $res = db_query("select ac_msg, ac_file, ac_tstamp from dh_applicant_clarification where ac_app=:a order by ac_tstamp desc",
    array(':a' => (int) $a_id));
  foreach ($res as $r) {
    $out[] = array(
      'when' => (string) $r->ac_tstamp,
      'message' => trim(strip_tags((string) $r->ac_msg)),
      'has_file' => !empty($r->ac_file),
    );
  }
  return $out;
}

/** Internal review comments for an applicant (dh_applicant_reviews). Plain text. */
function dcv_student_reviews($a_id) {
  $out = array();
  $res = db_query("select ar_review, ar_created, ar_user, name from dh_applicant_reviews left join users on ar_user=uid where ar_applicant=:a order by ar_created desc", array(':a' => (int) $a_id));
  foreach ($res as $r) {
    $out[] = array(
      'review'  => trim(preg_replace('/\s+/', ' ', strip_tags((string) $r->ar_review))),
      'created' => is_numeric($r->ar_created) ? date('Y-m-d', (int) $r->ar_created) : (string) $r->ar_created,
      'by'      => (string) $r->name,
    );
  }
  return $out;
}

/** Applicant's course history (same person by name + email/zip/dob), like _search_student. */
function dcv_student_previous_courses($a_id) {
  $d = db_query("select a_f_name, a_l_name, a_dob, a_email, a_zip from dh_applicant where a_id=:i", array(':i' => (int) $a_id))->fetchAssoc();
  if (!$d) { return array(); }
  $args = array(':f' => $d['a_f_name'], ':l' => $d['a_l_name']);
  if ($d['a_email'] !== '' && $d['a_email'] !== NULL) { $extra = " and a_email = :m"; $args[':m'] = $d['a_email']; }
  elseif ($d['a_zip'] !== '' && $d['a_zip'] !== NULL) { $extra = " and a_zip = :z"; $args[':z'] = $d['a_zip']; }
  else { $extra = " and a_dob = :b"; $args[':b'] = $d['a_dob']; }
  $sql = "select c_name, a_type, a_status, a_attended, a_address
          from dh_applicant left join dh_course on a_course=c_id left join dh_type_detail on td_id=c_course_type
          where a_f_name = :f and a_l_name = :l and (td_val3 is null or td_val3 <> '1-Day') $extra
          order by c_start desc limit 15";
  $out = array();
  foreach (db_query($sql, $args) as $r) {
    $out[] = array(
      'course' => (string) $r->c_name, 'type' => (string) $r->a_type,
      'status' => (string) $r->a_status, 'attended' => (bool) $r->a_attended,
      'address' => (string) $r->a_address,
    );
  }
  return $out;
}

/** Applicant activity log (status changes, letters, LC workflow) from dh_log. */
function dcv_student_activity($a_id) {
  $out = array();
  $res = db_query("select l_msg, name, l_tstamp from dh_log left join users on l_user=uid where l_module='Applicant' and l_identifier=:i and l_event in ('Status Change','Letter','LC Workflow') order by l_tstamp desc", array(':i' => (string) $a_id));
  foreach ($res as $r) {
    $out[] = array(
      'when'     => is_numeric($r->l_tstamp) ? date('Y-m-d H:i', (int) $r->l_tstamp) : (string) $r->l_tstamp,
      'activity' => trim(strip_tags((string) $r->l_msg)),
      'by'       => (string) $r->name,
    );
  }
  return $out;
}

/**
 * Occupied seats + per-seat card fields for a course, honouring the course's
 * seat plan (main vs group). Seat labels are Dipi's cached values (aa_seat /
 * aa_group_seat) — not recomputed. Read-only, parameterized.
 */
function dcv_course_seats($course_id) {
  $course_id = (int) $course_id;
  // dh_course_plan() lives in dh_manageapp/inc/allocate.inc (dependency).
  $plan = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'seat') : 'main';
  if ($plan === 'group') {
    $col_c = 'aa_group_seat_col'; $col_r = 'aa_group_seat_row'; $col_l = 'aa_group_seat';
  }
  else {
    $col_c = 'aa_seat_col'; $col_r = 'aa_seat_row'; $col_l = 'aa_seat';
  }
  // Cell and dining have their OWN main/group setting, independent of seat plan.
  $cell_plan = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'cell')   : 'main';
  $din_plan  = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'dining') : 'main';
  $col_cell = ($cell_plan === 'group') ? 'aa_group_cell'   : 'aa_cell';
  $col_din  = ($din_plan  === 'group') ? 'aa_group_dining' : 'aa_dining';
  // Column names are chosen from a fixed whitelist above — safe to interpolate.
  $sql = "select a_id, a_gender, a_old, a_lang_discourse,
            concat_ws(' ', nullif(trim(a_f_name),''), nullif(trim(a_m_name),''), nullif(trim(a_l_name),'')) as name,
            TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) as age,
            ac_teen, ac_10d, ac_stp, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_spl,
            aa_section, aa_acco, aa_cell, aa_group,
            aa_chowky, aa_chair, aa_backrest,
            ae_desc_physical, ae_desc_mental, ae_desc_medication, ae_desc_addiction_current,
            $col_r as seat_row, $col_c as seat_col, $col_l as seat_label
            , $col_cell as cell, $col_din as dining,
            COALESCE((select l.l_name from dh_languages l
                      where lower(l.l_code)=lower(a_lang_discourse) or l.l_name=a_lang_discourse
                      limit 1), a_lang_discourse) as lang_discourse
          from dh_applicant
            left join dh_applicant_course   on a_id = ac_applicant
            left join dh_applicant_attended on a_id = aa_applicant
            left join dh_applicant_extra    on a_id = ae_applicant
          where a_course = :c and a_attended = 1 and a_type = 'Student'
            and $col_r is not null and $col_c is not null";
  // A free-text health field counts as "present" only if it holds a real value
  // (not blank / a negative like "No", "None", "NA").
  $present = function ($v) {
    $t = strtolower(trim((string) $v));
    return $t !== '' && !in_array($t, array('no', 'none', 'na', 'n/a', 'nil', '-', '0'), true);
  };
  $out = array();
  foreach (db_query($sql, array(':c' => $course_id)) as $r) {
    // Room only; the cell is exported separately (as 'cell') so the app can render it once.
    $acco = trim($r->aa_section . '-' . $r->aa_acco, '-');
    $short = (int) $r->ac_teen + (int) $r->ac_10d + (int) $r->ac_stp;
    $long  = (int) $r->ac_20d + (int) $r->ac_30d + (int) $r->ac_45d
           + (int) $r->ac_60d + (int) $r->ac_tsc + (int) $r->ac_spl;
    $out[] = array(
      'section'    => ($r->a_gender === 'M') ? 'RIGHT' : 'LEFT',
      'group'      => (int) $r->aa_group,
      'row'        => (int) $r->seat_row,
      'col'        => (int) $r->seat_col,
      'label'      => (string) $r->seat_label,
      'chowky'     => (bool) $r->aa_chowky,
      'chair'      => (bool) $r->aa_chair,
      'student_id' => (int) $r->a_id,
      'name'       => $r->name,
      'acco'       => $acco,
      'age'        => is_null($r->age) ? null : (int) $r->age,
      'old_student'   => (bool) $r->a_old,
      'short_courses' => $short,
      'long_courses'  => $long,
      'backrest'      => (bool) ($r->aa_backrest || $r->aa_chowky),
      'cell'           => is_null($r->cell) ? '' : (string) $r->cell,
      'dining'         => is_null($r->dining) ? '' : (string) $r->dining,
      'lang_discourse' => is_null($r->lang_discourse) ? '' : (string) $r->lang_discourse,
      'health_physical'    => $present($r->ae_desc_physical),
      'health_mental'      => $present($r->ae_desc_mental),
      'health_medication'  => $present($r->ae_desc_medication),
      'health_intoxicants' => $present($r->ae_desc_addiction_current),
    );
  }
  return $out;
}

/** Course header for the bundle, or null if the course does not exist. */
function dcv_course_header($course_id) {
  $r = db_query("select c.c_id, c.c_name, c.c_center, c.c_start, c.c_end,
                        td.td_val1 as course_type, td.td_val3 as course_kind,
                        ce.c_name as centre_name, ce.c_subdomain as subdomain
                 from dh_course c
                   left join dh_type_detail td on c.c_course_type = td.td_id
                   left join dh_center ce on c.c_center = ce.c_id
                 where c.c_id = :c",
    array(':c' => (int) $course_id))->fetchObject();
  if (!$r) { return null; }
  $plan = function_exists('dh_course_plan') ? dh_course_plan((int) $course_id, 'seat') : 'main';
  // Teen/children courses label a_company as "School/College" (vs "Company").
  $youth = in_array(strtolower((string) $r->course_kind), array('teen', 'child'), true);
  return array(
    'id'          => (int) $r->c_id,
    'name'        => $r->c_name,
    'centre'      => (int) $r->c_center,
    'centre_name' => (string) $r->centre_name,
    'subdomain'   => (string) $r->subdomain,
    'start_date'  => $r->c_start,
    'end_date'    => $r->c_end,
    'course_type' => (string) $r->course_type,
    'youth'       => $youth,
    'seat_plan'   => $plan,
  );
}

/** Assemble the full course bundle (spec §5), or null if course missing. */
function dcv_build_bundle($course_id) {
  $header = dcv_course_header($course_id);
  if ($header === null) { return null; }
  $centre = $header['centre'];
  $seat_cfg = db_query("select cs_seat_config from dh_center_setting where cs_center = :c",
    array(':c' => $centre))->fetchField();
  $naming_conv = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center = :c",
    array(':c' => $centre))->fetchField();
  $sections = $seat_cfg ? dcv_parse_seat_config($seat_cfg, (int) $naming_conv) : array();
  $end_ts = !empty($header['end_date']) ? strtotime($header['end_date'] . ' +1 day') : false;
  $expires = $end_ts ? date('Y-m-d', $end_ts) : null;
  $students = dcv_course_students($course_id);
  return array(
    'schema_version' => 2,
    'course'         => $header,
    'sections'       => $sections,
    'seats'          => dcv_course_seats($course_id),
    'students'       => $students ? $students : new stdClass(),
    'generated_at'   => date('c'),
    'expires_after'  => $expires,
  );
}
