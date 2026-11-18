<?php
// Shared by test-referral-form.php and test-referral-save-allowlist.php: a complete referral form POST.
function t_input($over = array()) {
  return $over + array('s_f_name' => 'zzqtest', 's_m_name' => '', 's_l_name' => 'formone', 's_gender' => 'M', 's_dob' => array('date' => '1980-04-05'),
    's_email' => 'zzqform1@example.invalid', 's_phone_home' => '', 's_phone_mobile' => '9855555555', 's_phone_office' => '', 's_address' => 'Somewhere',
    's_city' => '', 's_state' => '', 's_country' => 'IN', 's_zip' => '', 's_education' => '', 's_occupation' => '', 's_company' => '', 's_department' => '',
    's_designation' => '', 's_aadhar' => '', 's_pancard' => '', 's_voter_id' => '', 's_passport' => '',
    'sci_teen' => '0', 'sci_10d' => '3', 'sci_stp' => '', 'sci_spl' => '0', 'sci_20d' => '0', 'sci_30d' => '0', 'sci_45d' => '0', 'sci_60d' => '0', 'sci_tsc' => '0', 'sci_service' => '1',
    'r_listed_10d' => '1', 'r_listed_stp' => '', 'r_listed_spl' => '', 'r_listed_20d' => '', 'r_listed_30d' => '', 'r_listed_45d' => '', 'r_listed_60d' => '', 'r_listed_tsc' => '',
    'r_listed_10d_seva' => '', 'r_listed_lc_seva' => '', 'r_listed_longseva' => '', 'r_special' => '0',
    'r_start' => array('date' => date('Y-m-d')), 'r_end' => array('date' => date('Y-m-d', strtotime('+90 days'))), 'r_reason' => 'zzq reason', 'r_listed_by' => 'zzq',
    'r_teacher' => '', 'r_course_center' => '', 'r_course_type' => '', 'r_course_start' => array('date' => ''));
}
