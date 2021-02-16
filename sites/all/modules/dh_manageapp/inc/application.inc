<?php


function dh_ma_applicant_form($form, &$form_state, $edit_data = array())
{
	/*$conf_detail = '';
	if (trim($edit_data['a_conf_no']) <> '')
		$conf_detail = '<h2>Conf No: '.$edit_data['a_conf_no'].'</h2>';
	$form['a'] = array('#markup' => $conf_detail);
	*/
	$add = 0;
	if (count($edit_data) == 0)
		$add = 1;
	if ( isset($edit_data['ref']) && ($edit_data['ref'] == 1))
		$add = 1;
	if ($add)
	{
		$centre_id = arg(2);
		$course_id = arg(3);

	}
	else
	{
		$centre_id = $edit_data['a_center'];
		$course_id = $edit_data['a_course'];

	}
	if ($course_id == '')
	{
		$course_id = $form_state['input']['course'];
		$centre_id = $form_state['input']['centre'];
	}
	$f_r = db_query("select c_name, c_finalized from dh_course where c_id=$course_id")->fetchAssoc();
	$finalized = $f_r['c_finalized'];
	$course_name = l($f_r['c_name'], "course/$centre_id/$course_id");
	if ( $finalized )
	{
		drupal_set_message("Course has been Finalized, cannot add/edit application", "error");
		drupal_goto("course/$centre_id/$course_id");
	}

	$LC = $TEEN = false;

	$course_check = strtolower(db_query("select td_val3 from dh_course left join dh_type_detail on c_course_type=td_id where c_id=$course_id")->fetchField());
	if ($course_check == 'lc')
		$LC = true;
	elseif ($course_check == 'teen')
		$TEEN = true;
		
	
	$group = 'Personal Information';
	$form['#attributes'] = array('class' => array('container-inline')); 
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false, '#prefix' => '<h2>'.$course_name.'</h2>');
	$form[$group]['a_f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#size' => '30', '#weight' => 1, 
		'#default_value' => isset($edit_data['a_f_name'])?$edit_data['a_f_name']:'', '#required' => 1, '#maxlength' => 50  );
	$form[$group]['a_m_name'] = array('#title' => 'Middle Name', '#type' => 'textfield', '#size' => '30', '#weight' => 2,
		'#default_value' => isset($edit_data['a_m_name'])?$edit_data['a_m_name']:'', '#maxlength' => 50);
	$form[$group]['a_l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#size' => '30', '#weight' => 3, 
		'#default_value' => isset($edit_data['a_l_name'])?$edit_data['a_l_name']:'', '#required' => 1, '#maxlength' => 50);
	$form[$group]['a_gender'] = array('#title' => 'Gender', '#type' => 'radios', '#options' => array("M" => 'Male', 'F' => 'Female')  , 
		'#default_value' => isset($edit_data['a_gender'])?$edit_data['a_gender']:'M', '#weight' => 4, '#required' => 1 , 
		'#ajax' => array(
		      // most notably 'mousedown', 'blur', and 'submit'.
		      // 'event' => 'change',
		      'callback' => 'ajax_section_dropdown_callback',
		      'wrapper' => 'dropdown-section-replace',
		    ),
		);

	$form[$group]['a_dob'] = array('#title' => 'Date of Birth', '#type' => 'date_popup', '#size' => '20', '#weight' => 10, 
		'#default_value' => isset($edit_data['a_dob'])?$edit_data['a_dob']:'', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'), '#suffix' => ''); /*'<div class="form-item">DOB Checked<input type="checkbox" id="edit-checked-dob" /></div>'); */
	$form[$group]['age'] = array('#title' => 'Age', '#type' => 'textfield', '#size' => '5', '#weight' => 11, 
		'#default_value' => '', '#required' => 0, '#maxlength' => 10 );
	
	$country_codes = db_query("select c_isd_code, CONCAT(c_name,' ','(',c_isd_code,')') from dh_country order by c_name")->fetchAllKeyed();
	$country_codes['0'] = 'Choose';
	$form[$group]['a_mob_country'] = array('#title' => 'Country Code', '#type' => 'select', '#options' =>  $country_codes,  '#weight' => 12, 
		'#default_value' => isset($edit_data['a_mob_country'])?$edit_data['a_mob_country']:'0', '#required' => 0, );
	
	$form[$group]['a_phone_mobile'] = array('#title' => 'Mobile', '#type' => 'textfield', '#size' => '20', '#weight' => 13, 
		'#default_value' => isset($edit_data['a_phone_mobile'])?$edit_data['a_phone_mobile']:'', '#required' => 0, '#maxlength' => 15 );
	$form[$group]['a_phone_home'] = array('#title' => 'Phone (Home)', '#type' => 'textfield', '#size' => '20', '#weight' => 13.5, 
		'#default_value' => isset($edit_data['a_phone_home'])?$edit_data['a_phone_home']:'', '#required' => 0, '#maxlength' => 20 );	
	$form[$group]['a_phone_office'] = array('#title' => 'Phone (Office)', '#type' => 'textfield', '#size' => '20', '#weight' => 14, 
		'#default_value' => isset($edit_data['a_phone_office'])?$edit_data['a_phone_office']:'', '#required' => 0, '#maxlength' => 20 );


	$form[$group]['a_address'] = array('#title' => 'Address', '#type' => 'textarea',  
		'#default_value' => isset($edit_data['a_address'])?$edit_data['a_address']:'', '#rows' => 3,  '#weight' => 20, '#required' => 1, 
		'#suffix' => '', '#maxlength' => 200 ); 
	$form[$group]['a_old'] = array('#title' => 'Is Old Student', '#type' => 'radios', '#options' => array("0" => 'New', '1' => 'Old')  , 
		'#default_value' => isset($edit_data['a_old'])?$edit_data['a_old']:'0', '#weight' => 21, '#required' => 1);
	$form[$group]['a_type'] = array('#title' => 'Applicant Type', '#type' => 'radios', '#options' => array("Student" => 'Student', 'Sevak' => 'Sevak')  , 
		'#default_value' => isset($edit_data['a_type'])?$edit_data['a_type']:'Student', '#weight' => 22, '#required' => 1);
	$form[$group]['a_alist'] = array('#title' => 'A-List?', '#type' => 'checkbox',  '#weight' => 23, 
		'#default_value' => isset($edit_data['a_alist'])?$edit_data['a_alist']:'' );
	$form[$group]['a_monk'] = array('#title' => 'Non-householder?', '#type' => 'checkbox',  '#weight' => 24, 
		'#default_value' => isset($edit_data['a_monk'])?$edit_data['a_monk']:'' );


	$form[$group]['a_zip'] = array('#title' => 'Zip', '#type' => 'textfield', '#size' => '20', '#weight' => 25, 
		'#default_value' => (isset($edit_data['a_zip']))?$edit_data['a_zip']:'', '#maxlength' => 20 );
	$form[$group]['a_country'] = array('#title' => 'Country', '#type' => 'textfield', '#size' => '20', '#weight' => 26, 
		'#default_value' => isset($edit_data['a_country'])?$edit_data['a_country']:'', '#required' => 1, '#maxlength' => 5,
				'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),

	);
	$city_name = '';
	if (!$add)
	{
		if ($edit_data['a_city'] <> '')
			$city_name = db_query("select c_name from dh_city where c_id='".$edit_data['a_city']."'")->fetchField();
	}
	$form[$group]['a_state'] = array('#title' => 'State', '#type' => 'textfield', '#size' => '20', '#weight' => 27, 
		'#default_value' => isset($edit_data['a_state'])?$edit_data['a_state']:'', '#required' => 1, '#maxlength' => 5 );
	$form[$group]['a_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '20', '#weight' => 28, 
		'#default_value' => $add?'':$city_name, '#description' => isset($edit_data['a_city_str'])?$edit_data['a_city_str']:'', '#maxlength' => 100);
	$q = "select td_val1, td_val3 from dh_type_detail where td_key='APPLICATION-MODE' order by td_val1";
	$modes = db_query($q)->fetchAllKeyed();
	$modes[''] = 'Choose';
	$form[$group]['a_source'] = array('#title' => 'Application Mode', '#type' => 'select', '#options' => $modes, '#weight' => 29,
		'#default_value' => isset($edit_data['a_source'])?$edit_data['a_source']:'', '#required' => 1  );


	$form[$group]['a_education'] = array('#title' => 'Education', '#type' => 'textfield', '#size' => '20', '#weight' => 35, 
		'#default_value' => isset($edit_data['a_education'])?$edit_data['a_education']:'', '#required' => 0, '#maxlength' => 25 );
	$form[$group]['a_occupation'] = array('#title' => 'Occupation', '#type' => 'textfield', '#size' => '20', '#weight' => 36, 
		'#default_value' => isset($edit_data['a_occupation'])?$edit_data['a_occupation']:'', '#required' => 0, '#maxlength' => 50 );
	$form[$group]['a_company'] = array('#title' => 'Company', '#type' => 'textfield', '#size' => '20', '#weight' => 37, 
		'#default_value' => isset($edit_data['a_company'])?$edit_data['a_company']:'', '#required' => 0, '#maxlength' => 50 );
	$form[$group]['a_department'] = array('#title' => 'Dept', '#type' => 'textfield', '#size' => '20', '#weight' => 38, 
		'#default_value' => isset($edit_data['a_department'])?$edit_data['a_department']:'', '#required' => 0, '#maxlength' => 50 );
	$form[$group]['a_designation'] = array('#title' => 'Designation', '#type' => 'textfield', '#size' => '20', '#weight' => 39, 
		'#default_value' => isset($edit_data['a_designation'])?$edit_data['a_designation']:'', '#required' => 0, '#maxlength' => 50);


	$doc_types = array('' => 'Choose', 'a_passport' => 'Passport', 'a_pancard' => 'Pancard', 'a_aadhar' => 'Aadhar', 'a_voter_id' => 'National ID');
	$default_doc_type = '';
	$default_doc_id = '';
	if (!$add || (isset($edit_data['a_passport'])) )
	{
		if ( $edit_data['a_passport'] <> '' ) $default_doc_type = 'a_passport';
		if ( $edit_data['a_pancard'] <> '' ) $default_doc_type = 'a_pancard';
		if ( $edit_data['a_aadhar'] <> '' ) $default_doc_type = 'a_aadhar';
		if ( $edit_data['a_voter_id'] <> '' ) $default_doc_type = 'a_voter_id';
		if ($default_doc_type <> '')
			$default_doc_id = $edit_data[$default_doc_type];		
	}

	$form[$group]['document_type'] = array('#title' => 'Photo ID', '#type' => 'select', '#multiple' => false,  '#weight' => 41, 
		'#default_value' => $default_doc_type, '#options' => $doc_types, '#required' => 0, );
	$form[$group]['document_id'] = array('#title' => 'Identifier', '#type' => 'textfield', '#size' => '20', '#weight' => 42, 
		'#default_value' => $default_doc_id, '#required' => 0, '#maxlength' => 20 );
	$form[$group]['a_langs'] = array('#title' => 'Languages Known', '#type' => 'textfield', '#size' => '25', '#weight' => 43, 
		'#default_value' => isset($edit_data['a_langs'])?$edit_data['a_langs']:'', '#required' => 0, '#maxlength' => 50 );
	$form[$group]['a_email'] = array('#title' => 'Email', '#type' => 'textfield', '#size' => '25', '#weight' => 44, 
		'#default_value' => isset($edit_data['a_email'])?$edit_data['a_email']:'', '#required' => 0, '#maxlength' => 100 );

	$proficiency = db_query("select td_key, td_key from dh_type_detail where td_type='LANG-PROFICIENCY'")->fetchAllKeyed();
	$proficiency[''] = 'Choose';
	$langs = db_query("select l_name, l_name from dh_languages order by l_name")->fetchAllKeyed();
	$langs[''] = 'Choose';
	$form[$group]['a_lang_1'] = array('#title' => 'Course Language 1', '#type' => 'select', '#options' =>  $langs,  '#weight' => 45, 
		'#default_value' => isset($edit_data['a_lang_1'])?$edit_data['a_lang_1']:'', '#required' => 0, );
	$form[$group]['a_lang_1_level'] = array('#title' => 'Lang 1 Proficiency', '#type' => 'select', '#options' =>  $proficiency,  '#weight' => 46, 
		'#default_value' => isset($edit_data['a_lang_1_level'])?$edit_data['a_lang_1_level']:'', '#required' => 0, );

	$form[$group]['a_lang_2'] = array('#title' => 'Course Language 2', '#type' => 'select', '#options' =>  $langs,  '#weight' => 47, 
		'#default_value' => isset($edit_data['a_lang_2'])?$edit_data['a_lang_2']:'', '#required' => 0, );
	$form[$group]['a_lang_2_level'] = array('#title' => 'Lang 2 Proficiency', '#type' => 'select', '#options' =>  $proficiency,  '#weight' => 48, 
		'#default_value' => isset($edit_data['a_lang_2_level'])?$edit_data['a_lang_2_level']:'', '#required' => 0, );

	$form[$group]['a_lang_3'] = array('#title' => 'Course Language 3', '#type' => 'select', '#options' =>  $langs,  '#weight' => 49, 
		'#default_value' => isset($edit_data['a_lang_3'])?$edit_data['a_lang_3']:'', '#required' => 0, );
	$form[$group]['a_lang_3_level'] = array('#title' => 'Lang 3 Proficiency', '#type' => 'select', '#options' =>  $proficiency,  '#weight' => 50, 
		'#default_value' => isset($edit_data['a_lang_3_level'])?$edit_data['a_lang_3_level']:'', '#required' => 0, );

	$form[$group]['a_lang_discourse'] = array('#title' => 'Discourse/Instructions Language', '#type' => 'select', '#options' =>  $langs,  '#weight' => 51, 
		'#default_value' => isset($edit_data['a_lang_discourse'])?$edit_data['a_lang_discourse']:'', '#required' => 0, );
	$form[$group]['a_lang_discourse_level'] = array('#title' => 'Disc Lang Proficiency', '#type' => 'select', '#options' =>  $proficiency,  '#weight' => 52, 
		'#default_value' => isset($edit_data['a_lang_discourse_level'])?$edit_data['a_lang_discourse_level']:'', '#required' => 0, );


	$photo = '';
	if (!$add)
	{
		$photo = '<div class="right"><img width="220px" src="'.file_create_url($edit_data['a_photo']).'"/></div>';
		if ($edit_data['a_uri'] <> '')
		{
			$photo = '<div class="right"><a href="'.file_create_url($edit_data['a_uri']).'"><img width="220px" src="'.file_create_url($edit_data['a_photo']).'"/></a></div>';
		}
	}

	$group = 'Additional Information';
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Additional Information',  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['ae_desc_physical'] = array('#title' => 'Physical Health', '#type' => 'textarea',  
		'#default_value' => isset($edit_data['ae_desc_physical'])?$edit_data['ae_desc_physical']:'', '#rows' => 3,  '#weight' => 50, '#required' => 0,
		'#suffix' => $photo);
	$form[$group]['ae_desc_mental'] = array('#title' => 'Mental Health', '#type' => 'textarea',  
		'#default_value' => isset($edit_data['ae_desc_mental'])?$edit_data['ae_desc_mental']:'', '#rows' => 3,  '#weight' => 51, 
		'#required' => 0);

	$form[$group]['ae_desc_addiction_current'] = array('#title' => 'Addiction Details', '#type' => 'textarea',  
		'#default_value' => isset($edit_data['ae_desc_addiction_current'])?$edit_data['ae_desc_addiction_current']:'', '#rows' => 3,  '#weight' => 52, 
		'#required' => 0);
	$t = '';
	if (isset($edit_data['ae_desc_other_technique']))
		$t = $edit_data['ae_desc_other_technique'];
	if (isset($edit_data['ae_desc_other_technique_old']))
		$t .= ' '.$edit_data['ae_desc_other_technique_old'];

	$form[$group]['ae_desc_other_technique'] = array('#title' => 'Other Meditation Techniques', '#type' => 'textarea',  
		'#default_value' => $t?$t:'', '#rows' => 3,  '#weight' => 53, '#required' => 0);

	$form[$group]['ae_desc_medication'] = array('#title' => 'Medication', '#type' => 'textarea',  
		'#default_value' => isset($edit_data['ae_desc_medication'])?$edit_data['ae_desc_medication']:'', '#rows' => 3,  '#weight' => 54, 
		'#required' => 0);
	$form[$group]['ae_pregnant_detail'] = array('#title' => 'Pregnancy Details', '#type' => 'textarea',  
		'#default_value' => $add?'':$edit_data['ae_pregnant_detail'], '#rows' => 3,  '#weight' => 55, '#required' => 0, '#maxlength' => 100);

	$form[$group]['a_emergency_name'] = array('#title' => 'Emergency Name', '#type' => 'textfield',  
		'#default_value' => isset($edit_data['a_emergency_name'])?$edit_data['a_emergency_name']:'', '#size' => 30,  '#weight' => 56, 
		'#required' => 0, '#maxlength' => 50);
	$form[$group]['a_emergency_relation'] = array('#title' => 'Emergency Relation', '#type' => 'textfield',  
		'#default_value' => $add?'':$edit_data['a_emergency_relation'], '#size' => 30,  '#weight' => 57, 
		'#required' => 0, '#maxlength' => 50);
	$form[$group]['a_emergency_num'] = array('#title' => 'Emergency Contact No', '#type' => 'textfield',  
		'#default_value' => $add?'':$edit_data['a_emergency_num'], '#size' => 20,  '#weight' => 58, 
		'#required' => 0, '#maxlength' => 40);
	$form[$group]['a_internal_note'] = array('#title' => 'Internal Note', '#type' => 'textarea',  
		'#default_value' => $add?'':$edit_data['a_internal_note'], '#rows' => 3,  '#weight' => 59, '#required' => 0, '#maxlength' => 200);
	$form[$group]['a_learn_about_vip'] = array('#title' => 'Recommended By', '#type' => 'textarea',  
		'#default_value' => $add?'':$edit_data['a_learn_about_vip'], '#rows' => 3,  '#weight' => 60, '#required' => 0, '#maxlength' => 255);
	$form[$group]['a_friend_family_details'] = array('#title' => 'Friend/Family Details', '#type' => 'textarea',  
 		'#default_value' => $add?'':$edit_data['a_friend_family_details'], '#rows' => 3,  '#weight' => 61, '#required' => 0, '#maxlength' => 50);
	$form[$group]['a_extra'] = array('#title' => 'Other Information', '#type' => 'textarea',  
 		'#default_value' => $add?'':$edit_data['a_extra'], '#rows' => 3,  '#weight' => 62, '#required' => 0, '#maxlength' => 250);



	$group = 'Course Information';
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Course Information',  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['ac_teen'] = array('#title' => 'Teenagers', '#type' => 'textfield', '#size' => '5', '#weight' => 59, 
		'#default_value' => isset($edit_data['ac_teen'])?$edit_data['ac_teen']:'0', '#required' => 0, '#maxlength' => 3 );
	$form[$group]['ac_10d'] = array('#title' => '10 Day', '#type' => 'textfield', '#size' => '5', '#weight' => 60, 
		'#default_value' => isset($edit_data['ac_10d'])?$edit_data['ac_10d']:'0', '#required' => 0, '#maxlength' => 3 );
	$form[$group]['ac_stp'] = array('#title' => 'STP', '#type' => 'textfield', '#size' => '5', '#weight' => 61, 
		'#default_value' => isset($edit_data['ac_stp'])?$edit_data['ac_stp']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_spl'] = array('#title' => 'Special', '#type' => 'textfield', '#size' => '5', '#weight' => 62, 
		'#default_value' => isset($edit_data['ac_spl'])?$edit_data['ac_spl']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_20d'] = array('#title' => '20 Day', '#type' => 'textfield', '#size' => '5', '#weight' => 63, 
		'#default_value' => isset($edit_data['ac_20d'])?$edit_data['ac_20d']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_30d'] = array('#title' => '30 Day', '#type' => 'textfield', '#size' => '5', '#weight' => 64, 
		'#default_value' => isset($edit_data['ac_30d'])?$edit_data['ac_30d']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_45d'] = array('#title' => '45 Day', '#type' => 'textfield', '#size' => '5', '#weight' => 65, 
		'#default_value' => isset($edit_data['ac_45d'])?$edit_data['ac_45d']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_60d'] = array('#title' => '60 Day', '#type' => 'textfield', '#size' => '5', '#weight' => 66, 
		'#default_value' => isset($edit_data['ac_60d'])?$edit_data['ac_60d']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_tsc'] = array('#title' => 'TSC', '#type' => 'textfield', '#size' => '5', '#weight' => 67, 
		'#default_value' => isset($edit_data['ac_tsc'])?$edit_data['ac_tsc']:'0', '#required' => 0, '#maxlength' => 2 );
	$form[$group]['ac_service'] = array('#title' => 'Service', '#type' => 'textfield', '#size' => '5', '#weight' => 68, 
		'#default_value' => isset($edit_data['ac_service'])?$edit_data['ac_service']:'0', '#required' => 0, '#maxlength' => 3 );
	$form[$group]['ac_teacher'] = array('#title' => '&nbsp;&nbsp;Is a Teacher?', '#type' => 'radios', 
		'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 69, 
		'#default_value' => isset($edit_data['ac_teacher'])?$edit_data['ac_teacher']:'0', '#required' => 0, );

	$form[$group]['ac_teacher_code'] = array( '#title' => 'Teacher AT Code',  '#type' => 'textfield', 
		'#weight' => 1, '#size' => 25, '#required' => 0, '#weight' => 70,
		'#default_value' => isset($edit_data['ac_teacher_code'])?$edit_data['ac_teacher_code']:'',
 		'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),
	 ) ;

	$year = array('' => 'Choose');
	for($i=1960; $i<=date('Y'); $i++ )	$year[$i] = $i;
	$month = array('' => 'Choose', '1' => 'Jan', '2' => 'Feb', '3' => 'Mar', '4' => 'Apr', '5' => 'May', '6' => 'Jun', '7' => 'Jul', 
		'8' => 'Aug', '9' => 'Sep', '10' => 'Oct', '11' => 'Nov', '12' => 'Dec');
	$day = array('' => 'Choose');
	for($i=1; $i<=31; $i++ ) $day[$i] = sprintf("%02d", $i);

	$form[$group]['ac_first_location_str'] = array('#title' => 'First Course Location', '#type' => 'textfield', '#size' => '25', '#weight' => 75, 
		'#default_value' => isset($edit_data['ac_first_location_str'])?$edit_data['ac_first_location_str']:'', 
		'#required' => 0, '#maxlength' => 100 );
	$form[$group]['ac_first_teacher_str'] = array('#title' => 'First Course Teacher', '#type' => 'textfield', '#size' => '25', '#weight' => 76, 
		'#default_value' => isset($edit_data['ac_first_teacher_str'])?$edit_data['ac_first_teacher_str']:'', '#required' => 0, '#maxlength' => 100 );
	$form[$group]['ac_first_year'] = array('#title' => 'Year', '#type' => 'select', '#multiple' => false, '#options' => $year , '#weight' => 77, 
		'#default_value' => isset($edit_data['ac_first_year'])?$edit_data['ac_first_year']:'', '#required' => 0, );
	$form[$group]['ac_first_month'] = array('#title' => 'Month', '#type' => 'select', '#options' => $month , '#multiple' => false, '#weight' => 78, 
		'#default_value' => isset($edit_data['ac_first_month'])?$edit_data['ac_first_month']:'', '#required' => 0, );
	$form[$group]['ac_first_day'] = array('#title' => 'Day', '#type' => 'select', '#options' => $day ,'#multiple' => false, '#weight' => 79, 
		'#default_value' => isset($edit_data['ac_first_day'])?$edit_data['ac_first_day']:'', '#required' => 0, );

	$form[$group]['ac_last_location_str'] = array('#title' => 'Last Course Location', '#type' => 'textfield', '#size' => '25', '#weight' => 80, 
		'#default_value' => isset($edit_data['ac_last_location_str'])?$edit_data['ac_last_location_str']:'', '#required' => 0, '#maxlength' => 100 );
	$form[$group]['ac_last_teacher_str'] = array('#title' => 'Last Course Teacher', '#type' => 'textfield', '#size' => '25', '#weight' => 81, 
		'#default_value' => isset($edit_data['ac_last_teacher_str'])?$edit_data['ac_last_teacher_str']:'', '#required' => 0, '#maxlength' => 100);
	$form[$group]['ac_last_year'] = array('#title' => 'Year', '#type' => 'select', '#multiple' => false, '#options' => $year , '#weight' => 82, 
		'#default_value' => isset($edit_data['ac_last_year'])?$edit_data['ac_last_year']:'', '#required' => 0, );
	$form[$group]['ac_last_month'] = array('#title' => 'Month', '#type' => 'select', '#options' => $month , '#multiple' => false, '#weight' => 83, 
		'#default_value' => isset($edit_data['ac_last_month'])?$edit_data['ac_last_month']:'', '#required' => 0, );
	$form[$group]['ac_last_day'] = array('#title' => 'Day', '#type' => 'select', '#options' => $day ,'#multiple' => false, '#weight' => 84, 
		'#default_value' => isset($edit_data['ac_last_day'])?$edit_data['ac_last_day']:'', '#required' => 0, );
	$form[$group]['ac_practice_details'] = array('#title' => 'Practice Details', '#type' => 'textarea',  
		'#default_value' => $add?'':$edit_data['ac_practice_details'], '#rows' => 3,  '#weight' => 85, '#required' => 0, '#maxlength' => 100);
	$form[$group]['ae_seva_details'] = array('#title' => 'Seva Details', '#type' => 'textarea',  
		'#default_value' => $add?'':$edit_data['ae_seva_details'], '#rows' => 3,  '#weight' => 86, '#required' => 0, '#maxlength' => 300);

	if ( $LC )
	{
		$group = 'Long Course Information';
		$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
		$course_types = db_query("select td_key, td_val1 from dh_type_detail where td_type='COURSE-TYPE' and td_val2 > 1")->fetchAllKeyed();
		$course_types[''] = 'Choose';
		$form[$group]['al_recent_sat_type'] = array('#title' => 'Recent Course Sat', '#type' => 'select', '#weight' => 1, '#options' => $course_types, 
			'#default_value' => isset($edit_data['al_recent_sat_type'])?$edit_data['al_recent_sat_type']:'', '#required' => 0, );
		$form[$group]['al_last_lc_type'] = array('#title' => 'Last LC Course', '#type' => 'select', '#weight' => 2, '#options' => $course_types, 
			'#default_value' => isset($edit_data['al_last_lc_type'])?$edit_data['al_last_lc_type']:'', '#required' => 0, );
		$form[$group]['al_last_lc_location'] = array('#title' => 'Last LC Course Location', '#type' => 'textfield', '#size' => '25', '#weight' => 3, 
			'#default_value' => isset($edit_data['al_last_lc_location'])?$edit_data['al_last_lc_location']:'', '#required' => 0, '#maxlength' => 30 );
		$form[$group]['al_last_lc_teacher'] = array('#title' => 'Last LC Course Teacher', '#type' => 'textfield', '#size' => '25', '#weight' => 4, 
			'#default_value' => isset($edit_data['al_last_lc_teacher'])?$edit_data['al_last_lc_teacher']:'', '#required' => 0, '#maxlength' => 100 );
		$form[$group]['al_last_lc_date'] = array('#title' => 'LC Date', '#type' => 'textfield', '#weight' => 5, '#size' => 10,
			'#default_value' => isset($edit_data['al_last_lc_date'])?$edit_data['al_last_lc_date']:'', '#required' => 0, '#maxlength' => 10 );

		$form[$group]['al_committed'] = array('#title' => 'Fully Committed?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 6, 
			'#default_value' => isset($edit_data['al_committed'])?$edit_data['al_committed']:'0', '#required' => 0, );
		$form[$group]['al_exclusive_2yrs'] = array('#title' => 'Exclusively Involved?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 7, 
			'#default_value' => isset($edit_data['al_exclusive_2yrs'])?$edit_data['al_exclusive_2yrs']:'0', '#required' => 0, );
		$form[$group]['al_5_precepts'] = array('#title' => 'Maintained 5 Precepts?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 8, 
			'#default_value' => isset($edit_data['al_5_precepts'])?$edit_data['al_5_precepts']:'0', '#required' => 0, );
		$form[$group]['al_intoxicants'] = array('#title' => 'Abstained Intoxicants?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 9, 
			'#default_value' => isset($edit_data['al_intoxicants'])?$edit_data['al_intoxicants']:'0', '#required' => 0, );
		$form[$group]['al_sexual_misconduct'] = array('#title' => 'Abstained Sexual Misconduct?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 10, 
			'#default_value' => isset($edit_data['al_sexual_misconduct'])?$edit_data['al_sexual_misconduct']:'0', '#required' => 0, );


		$form[$group]['al_at_year'] = array('#title' => 'If AT Year Apptd?', '#type' => 'textfield', 
			'#size' => 5, '#weight' => 11, '#maxlength' => 10,
			'#default_value' => isset($edit_data['al_at_year'])?$edit_data['al_at_year']:'', '#required' => 0, );
		$form[$group]['al_daily_practice_yrs'] = array('#title' => 'Daily Practice Yrs?', '#type' => 'textfield', 
			'#size' => 5, '#weight' => 12, '#maxlength' => 10,
			'#default_value' => isset($edit_data['al_daily_practice_yrs'])?$edit_data['al_daily_practice_yrs']:'', '#required' => 0, );
		$form[$group]['al_daily_practice_details'] = array('#title' => 'Daily Practice Details?', '#type' => 'textfield', 
			'#size' => 80, '#weight' => 13, '#maxlength' => 150, 
			'#default_value' => isset($edit_data['al_daily_practice_details'])?$edit_data['al_daily_practice_details']:'', '#required' => 0, );


		$form[$group]['al_relationship'] = array('#title' => 'In Relationship?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 14, 
			'#default_value' => isset($edit_data['al_relationship'])?$edit_data['al_relationship']:'0', '#required' => 0,
			'#suffix' => '<div class="relationship">' );
		$form[$group]['al_relationship_lifelong'] = array('#title' => 'LifeLong?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 15, 
			'#default_value' => isset($edit_data['al_relationship_lifelong'])?$edit_data['al_relationship_lifelong']:'0', '#required' => 0, 
			'#suffix' => '');
		$form[$group]['al_spouse_name'] = array('#title' => 'Spouse Name', '#type' => 'textfield', 
			'#size' => 40, '#weight' => 16, '#maxlength' => 60, 
			'#default_value' => isset($edit_data['al_spouse_name'])?$edit_data['al_spouse_name']:'', '#required' => 0,
			 );

		$form[$group]['al_relation_harmonious'] = array('#title' => 'Relation Harmonious?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 17, 
			'#default_value' => isset($edit_data['al_relation_harmonious'])?$edit_data['al_relation_harmonious']:'0', '#required' => 0, );
		$form[$group]['al_spouse_approve'] = array('#title' => 'Spouse In Favor?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 18, 
			'#default_value' => isset($edit_data['al_spouse_approve'])?$edit_data['al_spouse_approve']:'0', '#required' => 0, );
		$form[$group]['al_spouse_meditator'] = array('#title' => 'Is Spouse Meditator?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 19, 
			'#default_value' => isset($edit_data['al_spouse_meditator'])?$edit_data['al_spouse_meditator']:'0', '#required' => 0, );
		$form[$group]['al_spouse_other_technique'] = array('#title' => 'Spouse Other Technique?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 20, 
			'#default_value' => isset($edit_data['al_spouse_other_technique'])?$edit_data['al_spouse_other_technique']:'0', '#required' => 0,
			'#suffix' => '</div>' );


		$form[$group]['al_left_course_details'] = array('#title' => 'Refused Admission/Left Course Details', '#type' => 'textarea', 
			'#rows' => 3, '#weight' => 21, '#maxlength' => 100, 
			'#default_value' => isset($edit_data['al_left_course_details'])?$edit_data['al_left_course_details']:'', '#required' => 0, );
		$form[$group]['al_reduce_practice_details'] = array('#title' => 'Asked to reduce meditation Details', '#type' => 'textarea', 
			'#rows' => 3, '#weight' => 22, '#maxlength' => 100, 
			'#default_value' => isset($edit_data['al_reduce_practice_details'])?$edit_data['al_reduce_practice_details']:'', '#required' => 0, );
		$form[$group]['al_personal_tragedy_details'] = array('#title' => 'Personal Tragedy in Last year Details', '#type' => 'textarea', 
			'#rows' => 3, '#weight' => 23, '#maxlength' => 100,
			'#default_value' => isset($edit_data['al_personal_tragedy_details'])?$edit_data['al_personal_tragedy_details']:'', '#required' => 0, );
		$form[$group]['al_difficulty_details'] = array('#title' => 'Any Difficulties encountered Details', '#type' => 'textarea', 
			'#rows' => 3, '#weight' => 24, '#maxlength' => 100,
			'#default_value' => isset($edit_data['al_difficulty_details'])?$edit_data['al_difficulty_details']:'', '#required' => 0, );
		$form[$group]['al_special_req_details'] = array('#title' => 'Special Requirement Details', '#type' => 'textarea', 
			'#rows' => 3, '#weight' => 25, '#maxlength' => 120,
			'#default_value' => isset($edit_data['al_special_req_details'])?$edit_data['al_special_req_details']:'', '#required' => 0, );
		$form[$group]['al_additional_info'] = array('#title' => 'Any Additional Information', '#type' => 'textarea', 
			'#rows' => 3, '#weight' => 26, '#maxlength' => 100,
			'#default_value' => isset($edit_data['al_additional_info'])?$edit_data['al_additional_info']:'', '#required' => 0, );
		$form[$group]['al_arrival'] = array('#title' => 'Arrival at Centre?', '#type' => 'textfield', 
			 '#weight' => 27, '#maxlength' => 50,
			'#default_value' => isset($edit_data['al_arrival'])?$edit_data['al_arrival']:'', '#required' => 0, );	

		$form[$group]['al_recommending'] = array('#title' => 'Recommending AT', '#type' => 'textfield', 
			 '#weight' => 29, '#maxlength' => 100,
			'#default_value' => isset($edit_data['al_recommending'])?$edit_data['al_recommending']:'', '#required' => 0, 
				'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),

		);	
		$form[$group]['al_area_at'] = array('#title' => 'Area Teacher?', '#type' => 'textfield', 
			 '#weight' => 30, '#maxlength' => 100, 
			'#default_value' => isset($edit_data['al_area_at'])?$edit_data['al_area_at']:'', '#required' => 0, 
				'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),

		);	

	}

	if ( $TEEN )
	{
		$group = 'Teenagers Course Information';
		$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
		$form[$group]['ae_father'] = array('#title' => 'Name of Father', '#type' => 'textfield', '#weight' => 0,
			'#default_value' => isset($edit_data['ae_father'])?$edit_data['ae_father']:'', '#required' => 0, );
		$form[$group]['ae_mother'] = array('#title' => 'Name of Mother', '#type' => 'textfield', '#weight' => 1,
			'#default_value' => isset($edit_data['ae_mother'])?$edit_data['ae_mother']:'', '#required' => 0, );
		$form[$group]['ae_parent_course'] = array('#title' => 'Parents have attended course?', '#type' => 'radios', 
			'#options' => array('0' => 'No', '1' => 'Yes'), '#weight' => 2,
			'#default_value' => isset($edit_data['ae_parent_course'])?$edit_data['ae_parent_course']:'0', '#required' => 0, );
		$form[$group]['al_recommending'] = array('#title' => 'Recommending AT', '#type' => 'textfield', 
			 '#weight' => 3, '#maxlength' => 100,
			'#default_value' => isset($edit_data['al_recommending'])?$edit_data['al_recommending']:'', '#required' => 1, );
	}

	$group = 'Attending';
	//$sections = array('' => 'Choose', 'A' => 'A', 'B' => 'B');
	//$rooms = array('' => 'Choose', '110' => '110', '201' => '201');
	$cells = array('' => 'Choose');
	//$sections = array('' => 'Choose');
	$groups = array();
	for($i=1; $i<10; $i++)
		$groups[$i] = $i;

	$attended = 0;	
	if (!$add)
	{
		if (isset($edit_data['aa_acco']) && ($edit_data['aa_acco'] <> '') )
			$attended = 1;		
	}
	$gender_selected = isset($form_state['values']['a_gender']) ? $form_state['values']['a_gender'] : 'M';

	$room_info = '';
	if (!$add)
		if (isset($edit_data['aa_acco']) && ($edit_data['aa_acco'] <> '') )
			$room_info = '<h2>'.$edit_data['aa_section'].'-'.$edit_data['aa_acco'].' (<a id="change-room" href="#">Change Room</a>)</h2>';
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Attending Course',  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['attending'] = array('#title' => '&nbsp;Attending Course?', '#type' => 'radios', 
		'#options' => array('0' => 'No', '1' => 'Yes') ,'#weight' => 48, 
		'#default_value' => $attended, '#required' => 0, '#suffix' => '<div id="room-info"><p>'.$room_info.'</p></div><div id="attending-fields"><div id="room-section">'
		  );

	$temp = _ajax_get_section_options($gender_selected, $centre_id);
	$form[$group]['aa_section'] = array('#title' => 'Section', '#type' => 'select', '#options' => _ajax_get_section_options($gender_selected, $centre_id) ,'#multiple' => false, '#weight' => 49, 
		'#default_value' => $add?key( array_splice($temp, 1)):$edit_data['aa_section'], '#required' => 0, 
		'#ajax' => array(
		      // most notably 'mousedown', 'blur', and 'submit'.
		      // 'event' => 'change',
		      'callback' => 'ajax_acco_dropdown_callback',
		      'wrapper' => 'dropdown-acco-replace',
		    ),
		'#prefix' => '<div id="dropdown-section-replace">', '#suffix' => '</div>'
	 );
	$section_selected = isset($form_state['values']['aa_section']) ? $form_state['values']['aa_section'] : '';

	$form['acco_old'] = array('#type' => 'hidden', '#value' => $add?'':$edit_data['aa_acco']);
	$form['course'] = array('#type' => 'hidden', '#value' => $course_id);
	$form['centre'] = array('#type' => 'hidden', '#value' => $centre_id);
	$form[$group]['aa_acco'] = array('#title' => 'Room', '#type' => 'select', 
		'#options' => _ajax_get_acco_options($section_selected, $gender_selected, $centre_id, $course_id) ,'#multiple' => false, '#weight' => 50, 
		//'#default_value' => $add?'':$edit_data['aa_acco'], '#required' => 0,
		'#default_value' => '', '#required' => 0, 
		'#prefix' => '<div id="dropdown-acco-replace">' , '#suffix' => '</div>');
	$form[$group]['aa_laundry'] = array('#title' => 'Laundry Token', '#type' => 'textfield', '#size' => 5, '#weight' => 90, 
		'#default_value' => $add?'':$edit_data['aa_laundry'], '#required' => 0, '#prefix' => '</div>' );
	$form[$group]['aa_valuable'] = array('#title' => 'Valuable Token', '#type' => 'textfield', '#size' => 5, '#weight' => 91, 
		'#default_value' => $add?'':$edit_data['aa_valuable'], '#required' => 0, );
	$form[$group]['aa_cell'] = array('#title' => 'Cell No', '#type' => 'textfield', '#size' => 5 , '#weight' => 92, 
		'#default_value' => $add?'':$edit_data['aa_cell'], '#required' => 0, );
	$form[$group]['aa_cell_fixed'] = array('#title' => '&nbsp;Cell No Fixed?', '#type' => 'checkbox',  '#weight' => 93, 
		'#default_value' => $add?'0':$edit_data['aa_cell_fixed'], '#required' => 0, );


	$form[$group]['aa_comment'] = array('#title' => 'Comments', '#type' => 'textarea', '#maxlength' => 200,  
		'#default_value' => $add?'':$edit_data['aa_comment'], '#rows' => 3,  '#weight' => 95, '#required' => 0);

	$special = '';
	if (!$add)
	{
		if ($edit_data['aa_chowky']) $special = 'chowky';	
		if ($edit_data['aa_chair']) $special = 'chair';
		if ($edit_data['aa_backrest']) $special = 'backrest';		
	}

	$form[$group]['special'] = array('#title' => 'Special Seating', '#type' => 'radios', 
		'#options' => array('' => 'None', "chowky" => 'Chowky', 'chair' => 'Chair', 'backrest' => 'BackRest')  , 
		'#default_value' => $special, '#weight' => 96, '#required' => 0);

	$form[$group]['aa_group'] = array('#title' => 'Group', '#type' => 'select', 
		'#options' => $groups  , 
		'#default_value' => $add?'1':$edit_data['aa_group'] , '#weight' => 97, '#required' => 0 , '#suffix' => '</div>');

	$group = "Course Left";
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Course Left',  '#collapsible' => true, '#collapsed' => true);
	$form[$group]['aa_left'] = array('#title' => 'Course Left', '#type' => 'radios', 
		'#options' => array('0' => 'No', "1" => 'Yes')  , 
		'#default_value' => $add?'0':$edit_data['aa_left'], '#weight' => 98, '#required' => 0);
	$form[$group]['aa_left_on'] = array('#title' => 'Left on Date/Time', '#type' => 'date_popup', '#size' => 20, '#weight' => 99, 
		'#default_value' => $add?'':$edit_data['aa_left_on'], '#required' => 0, 
		'#date_format' => 'Y-m-d H:i', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 0, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-1:0', '#date_label_position' => 'above',
	);
	$form[$group]['aa_left_comment'] = array('#title' => 'Left Comments', '#type' => 'textarea',  
		'#default_value' => $add?'':$edit_data['aa_left_comment'], '#rows' => 3,  '#weight' => 100, '#required' => 0);


	if ($add)
		$action = 'Add Application';
	else
		$action = 'Update';
	//$form['return'] = array('#type' => 'hidden', '#value' => $_SERVER['REQUEST_URI']);
	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 110);

	$js ='
		(function ($) {
			function check_teacher()
			{
				if($("input[name=ac_teacher_code]:checked").val() == "1")
					$("#edit-ac-teacher-code").prop("disabled", false);
				else
				{
					$("#edit-ac-teacher-code").prop("disabled", true);				
					$("#edit-ac-teacher-code").val("");
				}
			}
			function check_oldstudent()
			{
				if($("input[name=a_old]:checked").val() == "1")
					$("#edit-course-information :input").prop("disabled", false);
				else
					$("#edit-course-information :input").prop("disabled", true);				

			}
			function check_attending()
			{
				if($("input[name=attending]:checked").val() == "1")
					$("#attending-fields :input").prop("disabled", false);
				else
					$("#attending-fields :input").prop("disabled", true);				
			}
			$(document).ready(function(){
				check_attending();
				check_teacher();
				$("input[type=radio][name=attending]").change(function(){
					check_attending();
				});
				$("input[type=radio][name=a_old]").change(function(){
					check_oldstudent();
				});
				$("input[type=radio][name=ac_teacher_code]").change(function(){
					alert("called");
					check_teacher();
				});

				$("#edit-ac-teacher-code").autocomplete({
					source: function( request, response ) {
						$("#edit-ac-teacher-code").addClass( "throbbing" );
				        $.ajax({
				          url: "/autocomplete/get-teacher/" + request.term,
				          dataType: "json",
				          success: function( data ) {
				          	$("#edit-ac-teacher-code").removeClass( "throbbing" );
				            response( data );
				          }
				        });
					},
		  			minLength: 1,
		  			select: function( event, ui ) {
		        		//console.log( "Selected: " + ui.item.value + " aka " + ui.item.code );
		        		$( "#edit-ac-teacher-code" ).val( ui.item.code );
		        		return false;
		      		}
		  		});

			});
		})(jQuery);
	';
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	//drupal_add_js($js, 'inline');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/location-common.js');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/manageapp.js');

	return $form;
}

function ajax_section_dropdown_callback($form, $form_state) {
	$form['Attending']['aa_section']['#value'] = '';
  	return $form['Attending']['aa_section'];
}

function ajax_acco_dropdown_callback($form, $form_state) {

//	if ($form_state['input']['aa_acco'] <> '')
//		$form['Attending']['aa_acco']['#options'][$form_state['input']['aa_acco']] = $form_state['input']['aa_acco']; 
	$form['Attending']['aa_acco']['#value'] = '';
  	return $form['Attending']['aa_acco'];
}

function _ajax_get_section_options($gender, $centre)
{
	$sections = array('' => 'Choose');
	$q = "select csa_section from dh_center_setting_acco where csa_center='$centre' and csa_gender='$gender'";
	$res = db_query($q);
	while($r = $res->fetchAssoc())
		$sections[$r['csa_section']] = $r['csa_section'];
	return $sections;
}

function _ajax_get_acco_options($section, $gender, $centre, $course)
{
	//watchdog("acco", "Sec - $section, Gen - $gender, Centre - $centre, Course -  $course");
	$rooms = array('' => 'Choose');

	$q = "select csa_room from dh_center_setting_acco where csa_center='$centre' and csa_section='$section' and csa_gender='$gender'";
	$result = db_query($q);
	$i = 0;
	while($r = $result->fetchAssoc())
	{
		$temp = explode(",", $r['csa_room'] );
		foreach ($temp as $v) 
		{
			$temp1 = explode(":", $v);
			if (count($temp1) == 2)
			{
				for($i = $temp1[0]; $i<= $temp1[1]; $i++)
					$rooms[ $i ] = $i;
			}
			else
				$rooms[$v] = $v;
		}
	}
	$alloted = array();
	$q = "select aa_acco from dh_applicant_attended left join dh_applicant on aa_applicant=a_id where a_course='$course' and a_gender='$gender' and aa_section='$section'";
	$result = db_query($q);
	while($r = $result->fetchAssoc())
		$alloted[] = $r['aa_acco'];

	foreach($alloted as $v ) 
		if ( isset($rooms[$v]) )
			unset($rooms[$v]);
	//watchdog('lala', print_r($result,true)) ;
	//watchdog('lala', $q.print_r($rooms, true) );
	return $rooms;
}

function dh_ma_applicant_form_validate($form, &$form_state)
{
	if ($form_state['input']['a_gender'] == '')
        form_set_error('a_gender', t('Gender not selected'));

    $teacher_code = $form_state['input']['ac_teacher_code'];
    if (!preg_match("/-/",$teacher_code) && (strpos($teacher_code, " ") !== false)) {
        form_set_error('ac_teacher_code', t('Incorrect AT code.	Please choose AT name from the drop down list'));
    }

}

function dh_ma_applicant_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];
	$ajax = '';
	if (arg(2) == 'edit')
		$ajax = arg(3);
	elseif ( arg(1) == 'add')
		$ajax = arg(4); 

	foreach ($form_state['input'] as $key => $value) 
	{
		if ( substr($key,0,2) == 'a_')
			$app[$key] = $value;
		if ( substr($key,0,3) == 'ac_')
			$app_ac[$key] = $value;
		if ( substr($key,0,3) == 'ae_')
			$app_ae[$key] = $value;
		if ( substr($key,0,3) == 'aa_')
			$app_aa[$key] = $value;
		if ( substr($key,0,3) == 'al_')
			$app_al[$key] = $value;
	}
	$app['a_medication'] = $app['a_problem_physical'] = $app['a_problem_mental'] = $app['a_addiction_current'] = $app['a_other_technique'] = 0;
	if ( trim($input['ae_desc_medication']) <> '' )
		$app['a_medication'] = 1;
	if ( trim($input['ae_desc_physical']) <> '' )
		$app['a_problem_physical'] = 1;
	if ( trim($input['ae_desc_mental']) <> '' )
		$app['a_problem_mental'] = 1;
	if ( trim($input['ae_desc_addiction_current']) <> '' )
		$app['a_addiction_current'] = 1;
	if ( trim($input['ae_desc_other_technique']) <> '' )
		$app['a_other_technique'] = 1;

	if ($input['document_type'] <> '')
		$app[$input['document_type']] = $input['document_id'];

	if (isset($input['a_alist']) && ($input['a_alist']))
		$app['a_alist'] = 1;
	else
		$app['a_alist'] = 0;

	if (isset($input['a_monk']) && ($input['a_monk']))
		$app['a_monk'] = 1;
	else
		$app['a_monk'] = 0;


	if ($app['a_city'] <> '')
	{	
		$q = "select c_id from dh_city where c_country='".$app['a_country']."' and c_state='".$app['a_state']."' and c_name='".$app['a_city']."' limit 1";
		$city_id = db_query($q)->fetchField();
		//watchdog('lala - $city_id', $q);
		if ( $city_id <> '')
			$app['a_city'] = $city_id;
		else
		{
				$q = "select ci.c_id from dh_pin_code p left join dh_city ci on p.pc_city=ci.c_id left join 
					dh_state s on (ci.c_state=s.s_code and ci.c_country=s.s_country) left join dh_country co on ci.c_country=co.c_code where 
					pc_pin='".$app['a_zip']."' limit 0,1";
				$city = db_query($q)->fetchField();
				if ($city > 0)
					$app['a_city'] = $city;
				else
				{
					/* We just could not find the city, so lets just add it */
					$f['c_country'] = $app['a_country'];
					$f['c_state'] = $app['a_state'];
					$f['c_name'] = $app['a_city'];
					$city = db_insert('dh_city')->fields($f)->execute();
					$app['a_city'] = $city;
				}
			//unset($app['a_city']);

		}

	}
	else
		unset($app['a_city']);
	if (!isset($app_aa['aa_left']))
		$app_aa['aa_left'] = 0;
	elseif ($app_aa['aa_left'])
	{
		$app['a_status'] = 'Left';
	}

	if ($input['attending'] && ( !$app_aa['aa_left']) ) 
		$app['a_attended'] = 1;
	else
		$app['a_attended'] = 0;
	$app['a_updated_by'] = $user->uid;
	$app['a_updated'] = date('Y-m-d H:i:s');
	$a = 'N';
	if ($app['a_old']) $a = 'O';
	if ( strtolower($app['a_type']) == 'sevak' )
		$new = 'S'.$app['a_gender'];
	else
		$new = $a.$app['a_gender'];

	$extra_id = $app_course_id = $app_attended_id = $app_lc_id = $app_id = ''; 
	$course_id = 0; $centre_id = 0;

	//if (trim($app['a_dob']) == '')
	//	unset($app['a_dob']);
	$status_ce = db_query("select td_key, td_val1 from dh_type_detail where td_type = 'COURSE-SYSTEM-STATUS' and td_key in ('STATUS-CONFIRMED', 'STATUS-EXPECTED') ")->fetchAllKeyed();
   $app['a_f_name'] = ucwords(strtolower($app['a_f_name']) );
   $app['a_l_name'] = ucwords(strtolower($app['a_l_name']) );

   $referral = is_in_referal_list($app);
   $app['a_referral'] = $referral?$referral:"0";
	if (arg(2) == 'edit')
	{
		$app_id = arg(1);
		$res = db_query("select a_course, a_conf_no, a_center, a_status from dh_applicant where a_id=$app_id")->fetchAssoc();
		$course_id = $res['a_course'];
		$centre_id = $res['a_center'];
		$old_status = $res['a_status'];

		if ($input['attending'] || in_array($old_status, array_values($status_ce) )  )
		{
			if (!isset($app['a_status'])) $app['a_status'] = '';
			if ( $app['a_status'] <> 'Left' )
			{
				$app['a_status'] =  $status_ce['STATUS-CONFIRMED'];
				$conf_no = $res['a_conf_no'];
				if ( strlen($new) == 2 )
				{
					if (( $conf_no == '' ) || ( substr($conf_no,0,2) <> $new ))
					{
						$new_conf_no = $new.db_query("select nextval('".$res['a_course']."-$new')")->fetchField();
						$app['a_conf_no'] = $new_conf_no; 
					}				
				}				
			}
		}

		db_update('dh_applicant')->fields($app)->condition('a_id', $app_id )->execute();
		$extra_id = db_query("select ae_id from dh_applicant_extra where ae_applicant=".$app_id)->fetchField();
		$app_course_id = db_query("select ac_id from dh_applicant_course where ac_applicant=".$app_id." limit 1")->fetchField();
		$app_attended_id = db_query("select aa_id from dh_applicant_attended where aa_applicant=".$app_id)->fetchField();
		$app_lc_id = db_query("select al_id from dh_applicant_lc where al_applicant=".$app_id)->fetchField();

	}
	else
	{
		if ($app['a_source'] <> 'Manual' )
			$app['a_status'] =  'Received';
		else
		{
			$status_confirmed = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-CONFIRMED'")->fetchField();
			$new_conf_no = $new.db_query("select nextval('".arg(3)."-$new')")->fetchField();
			$app['a_status'] =  $status_confirmed;
			$app['a_conf_no'] = $new_conf_no; 
		}
		$app['a_center'] = arg(2);
		$app['a_course'] = arg(3);
		$app['a_created_by'] = $user->uid;
		$app['a_created'] = date('Y-m-d H:i:s');
		$centre_id = arg(2);
		$course_id = arg(3);
		$app_id = db_insert('dh_applicant')->fields($app)->execute();
		dh_send_letter('applicant', $app_id, $app['a_status'] );

	}
	/* Insert or update applicant_extra */
	if ( $app['a_medication'] || $app['a_problem_physical'] || $app['a_problem_mental'] || 
		$app['a_addiction_current'] || $app['a_other_technique'] || trim($input['ae_pregnant_detail']) <> '' || arg(2) == 'edit' )
	{
		if ( ($input['ae_pregnant_detail'] <> '') && (! in_array(strtolower($input['ae_pregnant_detail']), array("-", "no")) )) 
			$app_ae['ae_pregnant'] = 1;
		else
			$app_ae['ae_pregnant'] = 0;
		$app_ae['ae_updated_by'] = $user->uid;
		$app_ae['ae_updated'] = date('Y-m-d H:i:s');
		if ($extra_id > 0)
		{				
			db_update('dh_applicant_extra')->fields($app_ae)->condition('ae_id', $extra_id)->execute();
		}
		else
		{
			$app_ae['ae_applicant'] = $app_id;
			$app_ae['ae_created_by'] = $user->uid;
			db_insert('dh_applicant_extra')->fields($app_ae)->execute();				
		}
	}			
	/* Insert or update applicant_course */
	if ( $app['a_old'] )
	{
		if ($app_ac['ac_first_year'] == '') $app_ac['ac_first_year'] = 0;
		if ($app_ac['ac_last_year'] == '') $app_ac['ac_last_year'] = 0;
		if ($app_ac['ac_first_month'] == '') $app_ac['ac_first_month'] = 0;
		if ($app_ac['ac_last_month'] == '') $app_ac['ac_last_month'] = 0;
		if ($app_ac['ac_first_day'] == '') $app_ac['ac_first_day'] = 0;
		if ($app_ac['ac_last_day'] == '') $app_ac['ac_last_day'] = 0;

		$app_ac['ac_updated_by'] = $user->uid;
		$app_ac['ac_updated'] = date('Y-m-d H:i:s');
		if ($app_ac['ac_teen'] == '') $app_ac['ac_teen'] = 0;
		if ($app_ac['ac_10d'] == '') $app_ac['ac_10d'] = 0;
		if ($app_ac['ac_20d'] == '') $app_ac['ac_20d'] = 0;
		if ($app_ac['ac_30d'] == '') $app_ac['ac_30d'] = 0;
		if ($app_ac['ac_45d'] == '') $app_ac['ac_45d'] = 0;
		if ($app_ac['ac_60d'] == '') $app_ac['ac_60d'] = 0;
		if ($app_ac['ac_stp'] == '') $app_ac['ac_stp'] = 0;
		if ($app_ac['ac_spl'] == '') $app_ac['ac_spl'] = 0;
		if ($app_ac['ac_tsc'] == '') $app_ac['ac_tsc'] = 0;
		if ($app_ac['ac_service'] == '') $app_ac['ac_service'] = 0;
		if ($app_ac['ac_teacher'] == '') $app_ac['ac_teacher'] = 0;

		if ($app_course_id > 0)
		{				
			db_update('dh_applicant_course')->fields($app_ac)->condition('ac_id', $app_course_id)->execute();
		}
		else
		{
			$app_ac['ac_applicant'] = $app_id;
			$app_ac['ac_created_by'] = $user->uid;
			db_insert('dh_applicant_course')->fields($app_ac)->execute();				
		}
	}

	if ( isset($app_al) )
	{
		if (!$app_al['al_relationship'])
		{
			$app_al['al_relationship_lifelong'] = null;
			$app_al['al_spouse_name'] = null;
			$app_al['al_relation_harmonious'] = null;
			$app_al['al_spouse_approve'] = null;
			$app_al['al_spouse_meditator'] = null;
			$app_al['al_spouse_other_technique'] = null;
		}
		if ( $app_al['al_left_course_details'] <> '' )
			$app_al['al_left_course'] = 1;
		else
			$app_al['al_left_course'] = 0;

		if ( $app_al['al_reduce_practice_details'] <> '' )
			$app_al['al_reduce_practice'] = 1;
		else
			$app_al['al_reduce_practice'] = 0;

		if ( $app_al['al_personal_tragedy_details'] <> '' )
			$app_al['al_personal_tragedy'] = 1;
		else
			$app_al['al_personal_tragedy'] = 0;

		if ( $app_al['al_difficulty_details'] <> '' )
			$app_al['al_difficulty'] = 1;
		else
			$app_al['al_difficulty'] = 0;

		if ( $app_al['al_special_req_details'] <> '' )
			$app_al['al_special_req'] = 1;
		else
			$app_al['al_special_req'] = 0;

		if ($app_al['al_at_year'] == '')
			$app_al['al_at_year'] = null;

		if ($app_lc_id > 0)
		{				
			db_update('dh_applicant_lc')->fields($app_al)->condition('al_id', $app_lc_id)->execute();
		}
		else
		{
			$app_al['al_applicant'] = $app_id;
			db_insert('dh_applicant_lc')->fields($app_al)->execute();				
		}
	}

	if ($input['attending'])
	{
		if ($app_aa['aa_acco'] == '')
		{
			/* Nothing was selected, so retain old value */
			unset($app_aa['aa_acco']);
			unset($app_aa['aa_section']);

		}
		//print_r( $app_aa['aa_left_on'] ) ;
		if ($app_aa['aa_left'])
		{
			if (is_array($app_aa['aa_left_on']))
			{
				$app_aa['aa_left_on'] = $app_aa['aa_left_on']['date'].' '.$app_aa['aa_left_on']['time'].':00';

			}			
		}
		else
			unset($app_aa['aa_left_on']);

		if (isset($input['aa_cell_fixed']) && ($input['aa_cell_fixed']))
			$app_aa['aa_cell_fixed'] = 1;
		else
			$app_aa['aa_cell_fixed'] = 0;

		$app_aa['aa_updated_by'] = $user->uid;
		$app_aa['aa_updated'] = date('Y-m-d H:i:s');
		$app_aa['aa_chowky'] = $app_aa['aa_chair'] = $app_aa['aa_backrest'] = 0;
		if ($input['special'] == 'chowky')
			$app_aa['aa_chowky'] = 1;
		if ($input['special'] == 'chair')
			$app_aa['aa_chair'] = 1;
		if ($input['special'] == 'backrest')
			$app_aa['aa_backrest'] = 1;

		if ($app_attended_id > 0)
		{				
			db_update('dh_applicant_attended')->fields($app_aa)->condition('aa_id', $app_attended_id)->execute();
		}
		else
		{
			$app_aa['aa_applicant'] = $app_id;
			$app_aa['aa_created_by'] = $user->uid;
			db_insert('dh_applicant_attended')->fields($app_aa)->execute();				
		}			
	}
	else
	{
		db_query("delete from dh_applicant_attended where aa_applicant=".$app_id);		
	}
		//drupal_set_message("Applicant data updated!");
	create_application_pdf($app_id);
	//drupal_goto("zero-day");
	//$return_url = $input['return'];
	$return_url = "zero-day/$centre_id/$course_id";
	if ($_SESSION['lasturi'] <> '')
		$return_url = $_SESSION['lasturi'];
	//drupal_set_message($_SESSION['lasturi']);
	$temp = parse_url($return_url);
	$qry = array();
	if (isset($temp['query']))
		parse_str($temp['query'], $qry);
	if ( strstr($temp['path'], "search-app") || (strstr($temp['path'], "search-course")) )
	{
		drupal_goto("course/$centre_id/$course_id");
		//drupal_goto("search-course/$centre_id/$course_id", array('query' => array('s' => '', 't' => '', 'g' => '')));		
	}
	else
		drupal_goto($temp['path'], array('query' => $qry ));
	if($ajax <> '')
	{
		if ($add)
			$msg = "Conf No $new_conf_no created successfully!";
		else
			$msg = $app['a_f_name'].' '.$app['a_l_name']." details updated successfully";
		// Tell the browser to close the modal.
		$form_state['ajax_commands'][] = ctools_modal_command_dismiss();

		// Tell the browser to replace the old link with the new one.
		//$form_state['ajax_commands'][] = ajax_command_replace('#replace-me', dh_applicant_attended($course_id));	
		$form_state['ajax_commands'][] = ctools_ajax_command_redirect("/zero-day/$centre_id/$course_id");	
		$form_state['ajax_commands'][] = ctools_modal_command_display(t('Message'), $msg);

	}
 
}


function app_delete( $id )
{
	global $user; 
	$temp = db_query("select a_center, CONCAT(a_f_name, ' ', a_l_name) as 'name' from dh_applicant where a_id=$id");
	$row = $temp->fetchAssoc();
	$fields = array();
	$fields['l_center'] = $row['a_center'];
	$fields['l_module'] = 'Applicant';
	$fields['l_identifier'] = $id;
	$fields['l_event'] = 'Deleted';
	$fields['l_msg'] = 'Applicant '.$row['name']." ($id) Deleted";
	$fields['l_user'] = $user->uid;
	db_insert('dh_log')->fields( $fields )->execute();

	$q = "delete from dh_applicant_course where ac_applicant=$id";
	db_query($q);
	$q = "delete from dh_applicant_lc where al_applicant=$id";
	db_query($q);
	$q = "delete from dh_applicant_extra where ae_applicant=$id";
	db_query($q);
	$q = "delete from dh_applicant where a_id=$id";
	db_query($q);
}

function is_in_referal_list( $row )
{
	$referral = 0;
	$q = "select r_id from dh_referral r left join dh_student s on (r.r_student=s.s_id) 
		where s_f_name = '".$row['a_f_name']."' and s_l_name='".$row['a_l_name']."' and 
		( s_email='".$row['a_email']."' or s_phone_mobile='".$row['a_phone_mobile']."' ) limit 1";
	$referral = db_query($q)->fetchField();
	return $referral;
}



