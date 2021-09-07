<?php


function dh_atportal_address_book()
{
	$out = '<h1>AT Address Book</h1><ul><li><h2>'.l("Add Assistant Teacher", "at-portal/address-book/add").'</h2></li>'.
	'</ul>';
	return $out;
}

function dh_atportal_profile($form, &$form_state, $edit_data = array())
{
	global $user;
	$at_code = arg(3);
	$form['a'] = array('#markup' => '<h2>Profile for '.$at_code.'</h2>');

	$group = 'Personal Information';
	$form['#attributes'] = array('class' => array('container-inline'));
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['t_f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#size' => '30', '#weight' => 1, 
		'#default_value' => isset($edit_data['t_f_name'])?$edit_data['t_f_name']:'', '#required' => 1, '#maxlength' => 50  );
	$form[$group]['t_m_name'] = array('#title' => 'Middle Name', '#type' => 'textfield', '#size' => '30', '#weight' => 2,
		'#default_value' => isset($edit_data['t_m_name'])?$edit_data['t_m_name']:'', '#maxlength' => 50);
	$form[$group]['t_l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#size' => '30', '#weight' => 3, 
		'#default_value' => isset($edit_data['t_l_name'])?$edit_data['t_l_name']:'', '#required' => 1, '#maxlength' => 50);
	$form[$group]['t_gender'] = array('#title' => 'Gender', '#type' => 'radios', '#options' => array("M" => 'Male', 'F' => 'Female')  , 
		'#default_value' => isset($edit_data['t_gender'])?$edit_data['t_gender']:'M', '#weight' => 4, '#required' => 1 , 
		'#ajax' => array(
		      // most notably 'mousedown', 'blur', and 'submit'.
		      // 'event' => 'change',
		      'callback' => 'ajax_section_dropdown_callback',
		      'wrapper' => 'dropdown-section-replace',
		    ),
		);
	$form[$group]['t_dob'] = array('#title' => 'Date of Birth', '#type' => 'date_popup', '#size' => '20', '#weight' => 10, 
		'#default_value' => isset($edit_data['t_dob'])?$edit_data['t_dob']:'', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'), '#suffix' => ''); /*'<div class="form-item">DOB Checked<input type="checkbox" id="edit-checked-dob" /></div>'); */
	$form[$group]['age'] = array('#title' => 'Age', '#type' => 'textfield', '#size' => '5', '#weight' => 11, 
		'#default_value' => '', '#required' => 0, '#maxlength' => 10 );
	
	$form[$group]['t_address'] = array('#title' => 'Address', '#type' => 'textarea', '#size' => '30', '#weight' => 3, 
		'#default_value' => isset($edit_data['t_address'])?$edit_data['t_address']:'', '#required' => 1, '#maxlength' => 50);
	$country_codes = db_query("select c_isd_code, CONCAT(c_name,' ','(',c_isd_code,')') from dh_country order by c_name")->fetchAllKeyed();
	$country_codes['0'] = 'Choose';
	$form[$group]['t_country'] = array('#title' => 'Country Code', '#type' => 'select', '#options' =>  $country_codes,  '#weight' => 12, 
		'#default_value' => isset($edit_data['t_country'])?$edit_data['t_country']:'0', '#required' => 0, );
	
	$form[$group]['t_mob_phone'] = array('#title' => 'Mobile', '#type' => 'textfield', '#size' => '20', '#weight' => 13, 
		'#default_value' => isset($edit_data['t_mob_phone'])?$edit_data['t_mob_phone']:'', '#required' => 0, '#maxlength' => 15 );
	$form[$group]['t_email'] = array('#title' => 'Mobile', '#type' => 'textfield', '#size' => '20', '#weight' => 14, 
		'#default_value' => isset($edit_data['t_email'])?$edit_data['t_email']:'', '#required' => 0, '#maxlength' => 15 );

	return $form;		
}

function dh_manage_at()
{
	switch( arg(2) )
	{
		case 'add':
			return drupal_get_form('dh_atportal_profile');
			break;
		case 'edit':			
			$row = db_query("select * from dh_teacher where t_code='an3'")->fetchAssoc();
			return drupal_get_form('dh_atportal_profile', $row);
			break;
	}	
}