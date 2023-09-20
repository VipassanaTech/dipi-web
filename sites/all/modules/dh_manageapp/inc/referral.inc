<?php

function dh_referral_form($form, &$form_state, $edit_data = array()) 
{	
  	drupal_add_library('system', 'ui.autocomplete');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/location-common.js');  	
    drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/manageapp.js');
    drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
    drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
  drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

    if(arg(0) == 'center-referral')
		$center_id = arg(1);
	else
		$center_id = 0;
 	
	if ($center_id)
		$form['b'] = array('#markup' => '<h3>'.l("Back to Dashboard", "centre/".arg(1))."&nbsp|&nbsp".l("Back to Center Referral List", "center-referral/".arg(1)).'</h3>');
 	else
 		$form['b'] = array('#markup' => '<h3>'.l("Back to Dashboard", "centre/".arg(1))."&nbsp|&nbsp".l("Back to Referral List", "referral/".arg(1)).'</h3>');
 	

	$add = 0;
  if(arg(2) == 'add')
    $add = 1;
 

	$group = 'Personal Information';

	$form['#attributes'] = array('class' => array('container-inline')); 

	$form[$group] = array(
		'#type' => 'fieldset',
		'#title' => 'Personal Information', 
		'#collapsible' => true,
		'#collapsed' => false,
	);

	$form[$group]['s_f_name'] = array(
		'#title' => 'First Name',
		'#type' => 'textfield',
		'#size' => '30',
		'#weight' => 1, 
		'#default_value' => isset($edit_data['s_f_name'])?$edit_data['s_f_name']:'',
		'#required' => 1,
		'#maxlength' => 50,
	);

	$form[$group]['s_m_name'] = array(
		'#title' => 'Middle Name',
		'#type' => 'textfield',
		'#size' => '30',
		'#weight' => 2,
		'#default_value' => isset($edit_data['s_m_name'])?$edit_data['s_m_name']:'',
		'#maxlength' => 50,
	);

	$form[$group]['s_l_name'] = array(
		'#title' => 'Last Name',
		'#type' => 'textfield',
		'#size' => '30',
		'#weight' => 3, 
		'#default_value' => isset($edit_data['s_l_name'])?$edit_data['s_l_name']:'',
		'#required' => 1,
		'#maxlength' => 50,
	);

	$form[$group]['s_gender'] = array(
		'#title' => 'Gender',
		'#type' => 'radios',
		'#options' => array("M" => 'Male', 'F' => 'Female')  , 
		'#default_value' => isset($edit_data['s_gender'])?$edit_data['s_gender']:'M',
		'#weight' => 4,
		'#required' => 1 , 
		'#suffix' => '',
		);

	$form[$group]['s_dob'] = array(
		'#title' => 'Date of Birth',
		'#type' => 'date_popup',
		'#size' => '20',
		'#weight' => 5, 
		'#default_value' => isset($edit_data['s_dob'])?$edit_data['s_dob']:'',
		'#date_format' => 'Y-m-d',
		'#datepicker_options' => array(
    		'maxDate' => 0,
    		'#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),
    	'#date_year_range' => '-100:-10',
    	'#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'),
		'#suffix' => '',
	);


	$form[$group]['s_phone_home'] = array(
		'#title' => 'Phone (Home)',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 12, 
		'#default_value' => isset($edit_data['s_phone_home'])?$edit_data['s_phone_home']:'',
		'#required' => 0,
		'#maxlength' => 20,
	);
	
	$form[$group]['s_phone_office'] = array(
		'#title' => 'Phone (Office)',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 13, 
		'#default_value' => isset($edit_data['s_phone_office'])?$edit_data['s_phone_office']:'',
		'#required' => 0,
		'#maxlength' => 20,
	);

	$form[$group]['s_phone_mobile'] = array(
		'#title' => 'Mobile (without country code)',
		'#type' => 'textfield', 
		'#size' => '20', 
		'#weight' => 14, 
		'#default_value' => isset($edit_data['s_phone_mobile'])?$edit_data['s_phone_mobile']:'',
		'#required' => 0,
		'#maxlength' => 15,
	);

	$form[$group]['s_email'] = array(
		'#title' => 'Email',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 15, 
		'#default_value' => isset($edit_data['s_email'])?$edit_data['s_email']:'',
		'#required' => 0,
		'#maxlength' => 50,
	);

	$form[$group]['s_education'] = array(
		'#title' => 'Education',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 15.1, 
		'#default_value' => isset($edit_data['s_education'])?$edit_data['s_education']:'',
		'#required' => 0,
		'#maxlength' => 25,
	);

	$form[$group]['s_occupation'] = array(
		'#title' => 'Occupation',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 15.2, 
		'#default_value' => isset($edit_data['s_occupation'])?$edit_data['s_occupation']:'',
		'#required' => 0,
		'#maxlength' => 50,
	);

	$form[$group]['s_company'] = array(
		'#title' => 'Company',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 15.3, 
		'#default_value' => isset($edit_data['s_company'])?$edit_data['s_company']:'',
		'#required' => 0,
		'#maxlength' => 50,
	);

	$form[$group]['s_department'] = array(
		'#title' => 'Dept',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 15.4, 
		'#default_value' => isset($edit_data['s_department'])?$edit_data['s_department']:'',
		'#required' => 0,
		'#maxlength' => 50,
	);

	$form[$group]['s_designation'] = array(
		'#title' => 'Designation',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 15.5, 
		'#default_value' => isset($edit_data['s_designation'])?$edit_data['s_designation']:'',
		'#required' => 0,
		'#maxlength' => 50,
	);


	$form[$group]['s_address'] = array(
		'#title' => 'Address',
		'#type' => 'textarea',  
		'#default_value' => isset($edit_data['s_address'])?$edit_data['s_address']:'',
		'#rows' => 3, 
		'#weight' => 20,
		'#required' => 0,
		'#suffix' => '',
		'#maxlength' => 200,
	); 

	$form[$group]['s_zip'] = array(
		'#title' => 'Zip',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 25, 
		'#default_value' => (isset($edit_data['s_zip']))?$edit_data['s_zip']:'',
		'#maxlength' => 20,
	);

	$form[$group]['s_country'] = array(
		'#title' => 'Country',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 26, 
		'#default_value' => isset($edit_data['s_country'])?$edit_data['s_country']:'',
		'#required' => 0,
		'#maxlength' => 5,
	);

	$city_name = '';
	if (!empty($edit_data))
	{
		if ((isset($edit_data['s_city'])) && ($edit_data['s_city'] <> ''))
			$city_name = db_query("select c_name from dh_city where c_id='".$edit_data['s_city']."'")->fetchField();
	}

	$form[$group]['s_state'] = array(
		'#title' => 'State',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 27, 
		'#default_value' => isset($edit_data['s_state'])?$edit_data['s_state']:'',
		'#required' => 0,
		'#maxlength' => 5,
	);

	$form[$group]['s_city'] = array(
		'#title' => 'City',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 28, 
		'#default_value' => $add?'':$city_name,
		'#description' => isset($edit_data['s_city'])?$edit_data['s_city']:'',
		'#maxlength' => 100,
	);
	
	$form[$group]['s_pancard'] = array(
		'#title' => 'Pancard',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 29, 
		'#default_value' => isset($edit_data['s_pancard'])?$edit_data['s_pancard']:'',
		'#required' => 0,
		'#maxlength' => 20
	);

	$form[$group]['s_aadhar'] = array(
		'#title' => 'Aadhar',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 29.1, 
		'#default_value' => isset($edit_data['s_aadhar'])?$edit_data['s_aadhar']:'',
		'#required' => 0,
		'#maxlength' => 20
	);

	$form[$group]['s_voter_id'] = array(
		'#title' => 'National ID',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 29.2, 
		'#default_value' => isset($edit_data['s_voter_id'])?$edit_data['s_voter_id']:'',
		'#required' => 0,
		'#maxlength' => 20
	);

	$form[$group]['s_passport'] = array(
		'#title' => 'Passport',
		'#type' => 'textfield',
		'#size' => '20',
		'#weight' => 29.3, 
		'#default_value' => isset($edit_data['s_passport'])?$edit_data['s_passport']:'',
		'#required' => 0,
		'#maxlength' => 20
	);

  
	$group = 'Referral Information';

	$form[$group] = array(
		'#type' => 'fieldset',
		'#title' => 'Referral Information',
		'#collapsible' => true,
		'#collapsed' => false,
	);



	$form[$group]['r_listed_10d'] = array(
		'#title' => '&nbsp;10-Day',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_10d'])?$edit_data['r_listed_10d']:'0',
		'#weight' => 13,
	);

	$form[$group]['r_listed_stp'] = array(
		'#title' => '&nbsp;STP',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_stp'])?$edit_data['r_listed_stp']:'0',
		'#weight' => 14,
	);

	$form[$group]['r_listed_spl'] = array(
		'#title' => '&nbsp;10D-SPL',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_stp'])?$edit_data['r_listed_spl']:'0', 
		'#weight' => 14.1,
	);

	$form[$group]['r_listed_20d'] = array(
		'#title' => '&nbsp;20-Day',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_20d'])?$edit_data['r_listed_20d']:'0',
		'#weight' => 15,
	);

	$form[$group]['r_listed_30d'] = array(
		'#title' => '&nbsp;30-Day',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_30d'])?$edit_data['r_listed_30d']:'0',
		'#weight' => 16,
	);

	$form[$group]['r_listed_45d'] = array(
		'#title' => '&nbsp;45-Day',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_45d'])?$edit_data['r_listed_45d']:'0',
		'#weight' => 17,
	);

	$form[$group]['r_listed_60d'] = array(
		'#title' => '&nbsp;60-Day',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_60d'])?$edit_data['r_listed_60d']:'0',
		'#weight' => 18,
	);

	$form[$group]['r_listed_tsc'] = array(
		'#title' => '&nbsp;TSC',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_tsc'])?$edit_data['r_listed_tsc']:'0',
		'#weight' => 19,
	);

	$form[$group]['r_listed_10d_seva'] = array(
		'#title' => '&nbsp;10-day Seva',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_10d_seva'])?$edit_data['r_listed_10d_seva']:'0',
		'#weight' => 20,
	);

	$form[$group]['r_listed_lc_seva'] = array(
		'#title' => '&nbsp;LC Seva',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_lc_seva'])?$edit_data['r_listed_lc_seva']:'0',
		'#weight' => 21,
	);

	$form[$group]['r_listed_longseva'] = array(
		'#title' => '&nbsp;Long-term Seva',
		'#type' => 'checkbox',
		'#default_value' => isset($edit_data['r_listed_longseva'])?$edit_data['r_listed_longseva']:'0',
		'#weight' => 22,
	);

    $form[$group]['checkall'] = array(
    	'#type' => 'checkbox',
    	'#title' => t('All'),
    	'#attributes' => array('onclick' => 'checkUncheckAll(this);'),
    	'#weight' => 23,
    );

    $form[$group]['r_start'] = array(
    	'#title' => 'Start Date',
    	'#type' => 'date_popup',
    	'#size' => '20',
    	'#weight' => 23.1, 
		'#default_value' => isset($edit_data['r_start'])?$edit_data['r_start']:'',
		'#date_format' => 'Y-m-d',
		'#required' => 1,
		'#datepicker_options' => array(
    		'maxDate' => 0, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    		'#required' => 1,
    	),
    	'#date_year_range' => '-50:0',
    	'#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'),
		'#suffix' => '',
	);

    $form[$group]['r_end'] = array(
    	'#title' => 'End Date',
    	'#type' => 'date_popup',
    	'#size' => '20',
    	'#weight' => 23.2, 
		'#default_value' => isset($edit_data['r_end'])?$edit_data['r_end']:'',
		'#date_format' => 'Y-m-d',
		'#required' => 1,
		'#datepicker_options' => array(
    		'maxDate' => "+2Y",
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    		'#required' => 1,
    	),
    	'#date_year_range' => '0:+2',
    	'#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'),
		'#suffix' => '',
	);
  
    $form[$group]['r_reason'] = array(
    	'#title' => 'Referral Reason/Remarks',
    	'#type' => 'textarea',  
	  	'#default_value' => isset($edit_data['r_reason'])?$edit_data['r_reason']:'',
	  	'#rows' => 3,
	  	'#weight' => 24,
	  	'#required' => 1, 
		'#suffix' => '',
	);
  
  if($center_id)
  {
    $form[$group]['r_listed_by'] = array(
      '#title' => 'Referred By',
      '#type' => 'textarea',
      '#default_value' => isset($edit_data['r_listed_by'])?$edit_data['r_listed_by']:'',
      '#rows' => 3,
      '#weight' => 25,
      '#required' => 1,
      '#suffix' => '',
      '#maxlength' => 200,
    );
  }
  else
  {
    $at_required = 1;
    if(!$add && !isset($edit_data['r_teacher']) )
      $at_required = 0;

    $options_t = db_query("select t_id as 'at-id', CONCAT(t_f_name, ' ', t_l_name, ' (', t_code, ')') as 'area' from dh_teacher where t_status='Active' order by t_f_name, t_l_name")->fetchAllKeyed();
    $options_t[''] = 'Select Referring AT';
    $form[$group]['r_teacher'] = array(
      '#type' => 'select',
      '#options' => $options_t,
      '#title' => 'Referring AT',
      '#default_value' => isset($edit_data['r_teacher'])?$edit_data['r_teacher']:'',
      '#weight' => 25.5,
      '#required' => $at_required,
    );
  }


  	$group = 'Course Info';
	$form[$group] = array(
		'#type' => 'fieldset',
		'#title' => 'Course Information',
		'#collapsible' => true,
		'#collapsed' => false,
	);

	$form[$group]['sci_teen'] = array(
		'#title' => 'Teenagers',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 59, 
		'#default_value' => isset($edit_data['sci_teen'])?$edit_data['sci_teen']:'0',
		'#required' => 0,
		'#maxlength' => 3,
	);

	$form[$group]['sci_10d'] = array(
		'#title' => '10 Day',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 60, 
		'#default_value' => isset($edit_data['sci_10d'])?$edit_data['sci_10d']:'0',
		'#required' => 0,
		'#maxlength' => 3,
	);

	$form[$group]['sci_stp'] = array(
		'#title' => 'STP',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 61, 
		'#default_value' => isset($edit_data['sci_stp'])?$edit_data['sci_stp']:'0',
		'#required' => 0,
		'#maxlength' => 2,
	);

	$form[$group]['sci_spl'] = array(
		'#title' => 'Special',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 62, 
		'#default_value' => isset($edit_data['sci_spl'])?$edit_data['sci_spl']:'0',
		'#required' => 0,
		'#maxlength' => 2,
	);

	$form[$group]['sci_20d'] = array(
		'#title' => '20 Day',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 63, 
		'#default_value' => isset($edit_data['sci_20d'])?$edit_data['sci_20d']:'0',
		'#required' => 0,
		'#maxlength' => 2,
	);

	$form[$group]['sci_30d'] = array(
		'#title' => '30 Day',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 64, 
		'#default_value' => isset($edit_data['sci_30d'])?$edit_data['sci_30d']:'0',
		'#required' => 0,
		'#maxlength' => 2,
	);

	$form[$group]['sci_45d'] = array(
		'#title' => '45 Day',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 65, 
		'#default_value' => isset($edit_data['sci_45d'])?$edit_data['sci_45d']:'0',
		'#required' => 0,
		'#maxlength' => 2,
	);

	$form[$group]['sci_60d'] = array(
		'#title' => '60 Day',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 66, 
		'#default_value' => isset($edit_data['sci_60d'])?$edit_data['sci_60d']:'0',
		'#required' => 0,
		'#maxlength' => 2
	);

	$form[$group]['sci_tsc'] = array(
		'#title' => 'TSC',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 67, 
		'#default_value' => isset($edit_data['sci_tsc'])?$edit_data['sci_tsc']:'0',
		'#required' => 0,
		'#maxlength' => 2
	);

	$form[$group]['sci_service'] = array(
		'#title' => 'Service',
		'#type' => 'textfield',
		'#size' => '5',
		'#weight' => 68, 
		'#default_value' => isset($edit_data['sci_service'])?$edit_data['sci_service']:'0',
		'#required' => 0,
		'#maxlength' => 3
	);


   $js ='
	    function checkUncheckAll(theElement){
	      var theForm = theElement.form, z = 0;
	      for(z=0; z<theForm.length;z++){
	        if(theForm[z].type == "checkbox" && theForm[z].name != "checkall"){
	          theForm[z].checked = theElement.checked;
	        }
	      }
	    }

		(function ($) {
			
			$(document).ready(function(){

				autocomplete_country("#edit-s-country");
				autocomplete_state("#edit-s-state", "#edit-s-country");
				autocomplete_city("#edit-s-city", "#edit-s-country", "#edit-s-state");

		  	$("#edit-s-zip").focusout(function(){
		  		do_pincode($(this).val(), "#edit-s-country", "#edit-s-state", "#edit-s-city" );
	  		});

        $("#edit-r-teacher").select2();

			});
		})(jQuery);
	';

	$css ='
		.form-item-s-phone-mobile {
			clear: both;
		}
	';

	drupal_add_js($js, 'inline');
	drupal_add_css($css, 'inline');

	if (arg(2) == 'add')
		$action = 'Add Referral';
	else
		$action = 'Update';

	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 50);
  
  return $form;
}


function dh_referral_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];

	foreach ($form_state['input'] as $key => $value) 
	{
		if ( substr($key,0,2) == 's_')
			$app[$key] = $value;
		if ( substr($key,0,2) == 'r_')
			$app_r[$key] = $value;
		if ( substr($key,0,4) == 'sci_')
			$app_s[$key] = $value;
	}
	
	if ($app['s_dob']['date'] == '')
		$app['s_dob'] = NULL;
  else
    $app['s_dob'] = $app['s_dob']['date'];

  if ($app_r['r_teacher'] == '')
    $app_r['r_teacher'] = NULL;
	if ($app_r['r_listed_10d'] == '')
		$app_r['r_listed_10d'] = '0';
	if ($app_r['r_listed_stp'] == '')
		$app_r['r_listed_stp'] = '0';
	if ($app_r['r_listed_spl'] == '')
		$app_r['r_listed_spl'] = '0';
	if ($app_r['r_listed_20d'] == '')
		$app_r['r_listed_20d'] = '0';
	if ($app_r['r_listed_30d'] == '')
		$app_r['r_listed_30d'] = '0';
	if ($app_r['r_listed_45d'] == '')
		$app_r['r_listed_45d'] = '0';
	if ($app_r['r_listed_60d'] == '')
		$app_r['r_listed_60d'] = '0';
	if ($app_r['r_listed_tsc'] == '')
		$app_r['r_listed_tsc'] = '0';
	if ($app_r['r_listed_10d_seva'] == '')
		$app_r['r_listed_10d_seva'] = '0';
	if ($app_r['r_listed_lc_seva'] == '')
		$app_r['r_listed_lc_seva'] = '0';
	if ($app_r['r_listed_longseva'] == '')
		$app_r['r_listed_longseva'] = '0';

	if(arg(0) == 'center-referral')
		$app_r['r_center'] = arg(1);
	else
		$app_r['r_center'] = 0;


	if ($app['s_city'] <> '')
	{	
		$q = "select c_id from dh_city where c_country='".$app['s_country']."' and c_state='".$app['s_state']."' and c_name='".$app['s_city']."' limit 1";
		$city_id = db_query($q)->fetchField();
		//watchdog('lala - $city_id', $q);
		if ( $city_id <> '')
			$app['s_city'] = $city_id;
		else
		{
				$q = "select ci.c_id from dh_pin_code p left join dh_city ci on p.pc_city=ci.c_id left join 
					dh_state s on (ci.c_state=s.s_code and ci.c_country=s.s_country) left join dh_country co on ci.c_country=co.c_code where 
					pc_pin='".$app['s_zip']."' limit 0,1";
				$city = db_query($q)->fetchField();
				if ($city > 0)
					$app['s_city'] = $city;
				else
				{
					/* We just could not find the city, so lets just add it */
					$f['c_country'] = $app['s_country'];
					$f['c_state'] = $app['s_state'];
					$f['c_name'] = $app['s_city'];
					$city = db_insert('dh_city')->fields($f)->execute();
					$app['s_city'] = $city;
				}
			//unset($app['a_city']);

		}

	}
	else
		unset($app['s_city']);

    $app['s_f_name'] = ucwords(strtolower($app['s_f_name']) );
    $app['s_l_name'] = ucwords(strtolower($app['s_l_name']) );

    $app['s_updated_by'] = $user->uid;
	$app['s_updated'] = date('Y-m-d H:i:s');
    $app_r['r_updated_by'] = $user->uid;
	$app_r['r_updated'] = date('Y-m-d H:i:s');


	if ( arg(2) == 'edit')
	{
		$ref_id = arg(3);
		$student_id = db_query("select r_student from dh_referral where r_id=$ref_id")->fetchField();

		db_update('dh_student')->fields($app)->condition('s_id', $student_id )->execute();

		db_update('dh_student_course_input')->fields($app_s)->condition('sci_student', $student_id )->execute();

		db_update('dh_referral')->fields($app_r)->condition('r_id', $ref_id )->execute();
	}
	else 
	{
		$app['s_created_by'] = $user->uid;
	    $app['s_created'] = date('Y-m-d H:i:s');
	    $app_r['r_created_by'] = $user->uid;
		$app_r['r_created'] = date('Y-m-d H:i:s');

		$student_id = db_insert('dh_student')->fields($app)->execute();

		$app_s['sci_student'] = $student_id;
		db_insert('dh_student_course_input')->fields($app_s)->execute();

		$app_r['r_student'] = $student_id;
		$ref_id = db_insert('dh_referral')->fields($app_r)->execute();

	}

	/*update dh_applicant with a_referral for new/edit referral*/

	if ($ref_id)
    update_referral_flag($ref_id);

	drupal_set_message(t("Details updated successfully"), 'status');

	if (arg(0) == 'center-referral')
		drupal_goto ('center-referral/'.arg(1));
	else
		drupal_goto ('referral/'.arg(1));
}


function update_referral_flag($ref_id)
{
  $q = "select
      r_center,
      r_deleted,
      s_f_name,
      s_l_name,
      s_email,
      s_phone_mobile,
      s_pancard,
      s_aadhar,
      s_voter_id,
      s_passport
      from dh_referral r
      left join dh_student s on s.s_id = r.r_student
      where r_id=$ref_id";

  $ref = db_query($q)->fetchAssoc();

  $conditon = "";
  $conditon_clear ="";

  if ($ref['r_center'])
  {
    $conditon = " a.a_center='{$ref['r_center']}' and ";
    $conditon_clear = " a_center='{$ref['r_center']}' and ";
  }

  db_query("update dh_applicant set a_referral=0 where $conditon_clear a_referral=$ref_id");

  if ($ref["r_deleted"])
    return;

  $q = "update dh_applicant a
    left join dh_course co on a.a_course=co.c_id
    set a.a_referral = :a_referral
    where
    $conditon
    co.c_start>curdate() and
    a.a_f_name = :a_f_name and
    a.a_l_name = :a_l_name and
    (
      (a.a_email = :a_email and a.a_email <> '') or
      (a.a_phone_mobile = :a_phone_mobile and a.a_phone_mobile <> '')
    )
  ";

  $q_array = array (
    ':a_referral' => $ref_id,
    ':a_f_name' => $ref['s_f_name'],
    ':a_l_name' => $ref['s_l_name'],
    ':a_email' => $ref['s_email'],
    ':a_phone_mobile' => $ref['s_phone_mobile'],
  );

  db_query($q, $q_array);

  if ($ref['s_pancard'])
  {
    $q = "update dh_applicant a
      left join dh_course co on a.a_course=co.c_id
      set a.a_referral = :a_referral
      where
      $conditon
      co.c_start>curdate() and
      a.a_pancard = :a_pancard and a.a_pancard <> ''
    ";

    $q_array = array (
      ':a_referral' => $ref_id,
      ':a_pancard' => $ref['s_pancard'],
    );

    db_query($q, $q_array);
  }

  if ($ref['s_aadhar'])
  {
    $q = "update dh_applicant a
      left join dh_course co on a.a_course=co.c_id
      set a.a_referral = :a_referral
      where
      $conditon
      co.c_start>curdate() and
      a.a_aadhar = :a_aadhar and a.a_aadhar <> ''
    ";

    $q_array = array (
      ':a_referral' => $ref_id,
      ':a_aadhar' => $ref['s_aadhar'],
    );

    db_query($q, $q_array);
  }

  if ($ref['s_voter_id'])
  {
    $q = "update dh_applicant a
      left join dh_course co on a.a_course=co.c_id
      set a.a_referral = :a_referral
      where
      $conditon
      co.c_start>curdate() and
      a.a_voter_id = :a_voter_id and a.a_voter_id <> ''
    ";

    $q_array = array (
      ':a_referral' => $ref_id,
      ':a_voter_id' => $ref['s_voter_id'],
    );

    db_query($q, $q_array);
  }

  if ($ref['s_passport'])
  {
        $q = "update dh_applicant a
      left join dh_course co on a.a_course=co.c_id
      set a.a_referral = :a_referral
      where
      $conditon
      co.c_start>curdate() and
      a.a_passport = :a_passport and a.a_passport <> ''
    ";

    $q_array = array (
      ':a_referral' => $ref_id,
      ':a_passport' => $ref['s_passport'],
    );

    db_query($q, $q_array);
  }
}

function dh_addedit_referral()
{

	if ( arg(2) == 'add')
	{
		$new_row = array();
		if (isset($_REQUEST['aid']) && ($_REQUEST['aid'] <> '') )
		{
			$row = db_query("select * from dh_applicant where a_id='".$_REQUEST['aid']."'")->fetchAssoc();
			foreach( $row as $k => $v)
				$new_row[ str_replace("a_", "s_", $k) ] = $v;			
		} 

		return drupal_get_form('dh_referral_form', $new_row);
	}		
	elseif (arg(2) == 'edit') {
		$row = db_query("select * from dh_referral left join dh_student on r_student=s_id 
		  left join dh_student_course on r_student=sc_student left join dh_student_course_input on r_student = sci_student
			where r_id='".arg(3)."'")->fetchAssoc();
	}
		
	return drupal_get_form('dh_referral_form', $row);
	

}


function dh_referral_results()
{
	global $user;
  	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];


	if(arg(0) == 'center-referral')
		$center_id = arg(1);
	else
		$center_id = 0;
	
	if (user_access ('access at profile') && !arg(1))
	   $out = '<h2>'.l("Back to AT-Portal", "/at-portal")."&nbsp;&nbsp;";
    else
        $out = '<h2>'.l("Back to Dashboard", "centre/".arg(1))."&nbsp;&nbsp;";

 	if (user_access ('manage referral list') && $center_id == 0)
 		$out .= "|&nbsp;&nbsp;".l("Add Referral", "referral/".arg(1).'/add');

 	if (user_access ('manage center referral list') && $center_id <> 0)
 		$out .= "|&nbsp;&nbsp;".l("Add Center Referral", "center-referral/".arg(1).'/add');

 	$out .= '</h2>';


	$iPod    = stripos($_SERVER['HTTP_USER_AGENT'],"iPod");
	$iPhone  = stripos($_SERVER['HTTP_USER_AGENT'],"iPhone");
	$iPad    = stripos($_SERVER['HTTP_USER_AGENT'],"iPad");
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	if ( !($iPad || $iPhone || $iPod) )
	{
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.html5.min.js");
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.print.min.js");
	}

	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );


	$q = "select r_readonly, r_id, a.s_id, r_student, concat(s_f_name,' ',s_l_name) as 'Full Name', s_gender, s_city, ci.c_name as 'City', s.s_name as 'State', co.c_name as 'Country', s_address, s_zip, s_email, s_phone_home, s_phone_office, s_phone_mobile, s_passport, s_aadhar, s_voter_id, s_pancard, s_education, s_occupation, s_occupation_past, s_company, s_designation, s_department, s_photo, TIMESTAMPDIFF( YEAR, s_dob, CURDATE() ) as 'Age', sci_teen, sci_10d, sci_stp, sci_spl, sci_20d, sci_30d, sci_45d, sci_60d, sci_tsc, sci_teacher,sci_service, CONCAT( IFNULL(sci_first_year, ''),'-',IFNULL(sci_first_month, ''),'-',IFNULL(sci_first_day, ''), ', ', sci_first_location_str) as 'first_course', sc_desc_physical,  sc_desc_medication,  sc_desc_mental, sc_desc_other_technique, sc_addiction_current, sc_uri, r_listed_10d, r_listed_stp, r_listed_spl, r_listed_20d, r_listed_30d, r_listed_45d, r_listed_60d, r_listed_tsc, r_listed_10d_seva, r_listed_lc_seva, r_listed_longseva, r_listed_by, r_reason, r_start, r_end, r_deleted,
    concat(ifnull(t_f_name, ''), ' ', ifnull(t_l_name, ''), ' (', t_code, ')') as teacher, t_id as teacher_id
    from dh_student a
    left join dh_student_course sc on (a.s_id=sc_student)
    left join dh_student_course_input sci on (a.s_id=sci_student)
    left join dh_referral r on (a.s_id=r_student)
    left join dh_teacher t on (r.r_teacher=t.t_id)
    left join dh_country co on (s_country=co.c_code)
    left join dh_state s on (s_state = s.s_code and s.s_country = co.c_code)
    left join dh_city ci on (s_city=ci.c_id)
    where  (a.s_id=r_student) and
    (r_deleted = 0) and
    r_center={$center_id}
    group by a.s_id
    order by r.r_created";

	//c.c_name as 'last_course'
	//drupal_set_message($q);
//and s_status='Confirmed' 
	if ($user->uid == 1)
	{
		drupal_set_message($q);
	}
	$result = db_query($q);
	$rows = array();
	while( $r = $result->fetchAssoc() )
	{
		unset($rs);
		
		$rs['sid'] = $r['s_id'];
		$rs['ref_id'] = $r['r_id'];


		if (user_access('manage referral list') && !$center_id)
		  $rs['name'] = _ma_make_link($r['Full Name'],'referral/'.arg(1).'/edit/'.$rs['ref_id']);
		elseif (user_access('manage center referral list') && $center_id)
		  $rs['name'] = _ma_make_link($r['Full Name'],'center-referral/'.arg(1).'/edit/'.$rs['ref_id']);
		else
		  $rs['name'] = $r['Full Name'];

    $readonly = "";
    if($r['r_readonly'])
      $readonly = " checked=checked ";


		if (user_access('delete referral list') && !$center_id)
			$rs['del'] = l("Delete", "referral/".arg(1)."/".$rs['ref_id']."/delete", array("attributes" => array("class" => array("ref-del")))).' <label for="'.$rs['ref_id'].'">r-only</label> <input type="checkbox" class="referral-readonly" id="'.$rs['ref_id'].'" value="'.$rs['ref_id'].'" '.$readonly.'>' ;
		elseif (user_access('delete center referral list') && $center_id)
			$rs['del'] = l("Delete", "center-referral/".arg(1)."/".$rs['ref_id']."/delete", array("attributes" => array("class" => array("ref-del")))) ;
		else
			$rs['del'] = "No access"; 

    if($center_id)
    {
      $rs['ref_by_title'] = "Center Referral By:";
      $rs['ref_by'] = $r['r_listed_by'];
    }
    else
    {
      $rs['ref_by_title'] = "Referring Teacher:";
      $rs['ref_by'] = "";
      if($r['teacher_id'])
        $rs['ref_by'] = '<a href="#" id="'.$r['teacher_id'].'" class="get-at-info" >'.$r['teacher'].'</a>';
    }

    $rs['gender'] = (strtoupper($r['s_gender'])=='M')?'Male':'Female';
		//if ($r['City'] == '')
		//	$r['City'] = $r['s_city_str'];
		$rs['location'] = $r['Country'].', '.$r['State'].', '.$r['City'];
		$rs['city'] = $r['City'];
		$rs['state'] = $r['State'];
		$rs['country'] = $r['Country'];
		$rs['pin'] = $r['s_zip'];
		$rs['age'] = $r['Age'];
		$rs['address'] = $r['s_address'];//." - ".$r['s_zip'];			
		$rs['contact'] = '<label>H:&nbsp;</label>'.$r['s_phone_home']." <label>M:&nbsp;</label>".$r['s_phone_mobile']." <label>O:&nbsp;</label>".$r['s_phone_office']." <label>Email:&nbsp;</label>".$r['s_email'];
		$rs['contact_home'] = $r['s_phone_home'];
		$rs['contact_mobile'] = $r['s_phone_mobile'];
		$rs['contact_office'] = $r['s_phone_office'];
		$rs['contact_email'] = $r['s_email'];
		$rs['aadhar'] = $r['s_aadhar'];
		$rs['pancard'] = $r['s_pancard'];
		$rs['passport'] = $r['s_passport'];
		$rs['voter_id'] = $r['s_voter_id'];

		$rs['course_teen'] = $r['sci_teen']?$r['sci_teen']:"0";
		$rs['course_10d'] = $r['sci_10d']?$r['sci_10d']:"0";
		$rs['course_stp'] = $r['sci_stp']?$r['sci_stp']:"0";
		$rs['course_spl'] = $r['sci_spl']?$r['sci_spl']:"0";
		$rs['course_20d'] = $r['sci_20d']?$r['sci_20d']:"0";
		$rs['course_30d'] = $r['sci_30d']?$r['sci_30d']:"0";
		$rs['course_45d'] = $r['sci_45d']?$r['sci_45d']:"0";
		$rs['course_60d'] = $r['sci_60d']?$r['sci_60d']:"0";
		$rs['course_tsc'] = $r['sci_tsc']?$r['sci_tsc']:"0";
		$rs['course_seva'] = $r['sci_service']?$r['sci_service']:"0";

		$rs['first_course'] = $rs['last_course'] = '';

		$rs['ref_10d'] = $r['r_listed_10d']?"Yes":"No";
		$rs['ref_stp'] = $r['r_listed_stp']?"Yes":"No";
		$rs['ref_spl'] = $r['r_listed_spl']?"Yes":"No";
		$rs['ref_20d'] = $r['r_listed_20d']?"Yes":"No";
		$rs['ref_30d'] = $r['r_listed_30d']?"Yes":"No";
		$rs['ref_45d'] = $r['r_listed_45d']?"Yes":"No";
		$rs['ref_60d'] = $r['r_listed_60d']?"Yes":"No";
		$rs['ref_tsc'] = $r['r_listed_tsc']?"Yes":"No";
		$rs['ref_10dseva'] = $r['r_listed_10d_seva']?"Yes":"No";
		$rs['ref_lcseva'] = $r['r_listed_lc_seva']?"Yes":"No";
		$rs['ref_longseva'] = $r['r_listed_longseva']?"Yes":"No";
		$rs['ref_reason'] = $r['r_reason'];
		$rs['ref_start'] = $r['r_start'];
		$rs['ref_end'] = $r['r_end'];

		$rs['Education'] = $r['s_education'];
		$rs['Company'] = $r['s_company'];
		$rs['Dept'] = $r['s_department'];
		$rs['Occ'] = $r['s_occupation'];
		if ($r['s_occupation'] == '')
			$rs['Occ'] = $r['s_occupation_past'];
		$rs['Designation'] = $r['s_designation'];
		$rs['othertechnique'] = $r['sc_desc_other_technique']?$r['sc_desc_other_technique']:"";
		$rs['physical'] = $r['sc_desc_physical']?$r['sc_desc_physical']:"";
		$rs['mental'] = $r['sc_desc_mental']?$r['sc_desc_mental']:"";
		$rs['medication'] = $r['sc_desc_medication']?$r['sc_desc_medication']:"";
		$rs['addiction'] = $r['sc_addiction_current']?$r['sc_addiction_current']:"";
		$rs['photo'] = '';
		if ( $r['s_photo'] <> '')
			$rs['photo'] = file_create_url($r['s_photo']);
		//$rows[] = array_values($rs);
		
		$rows[] = $rs;
	}
	//$header = array('Detail', 'Applicant Name', 'Course Name', 'Status', 'ConfNo' , 'Gender', 'Country/State/City', 'Age');
	$header = array();
	$attributes = array('id' => 'table-applicants');
	$out .= theme('table', array('header' => $header, 'rows' => array(), 'attributes' => $attributes));
	$data = json_encode($rows);


	$button_access = '';
	if (user_access('export data'))
		if ( !($iPad || $iPhone || $iPod) )
			$button_access = '                        
				$.extend( true, {}, buttonCommon, {
      			    extend: "excelHtml5",
	            	title: "Student Data"
				} )
			';

/*

	            $.extend( true, {}, buttonCommon, {
	                extend: "pdfHtml5",
	            	title: "A-List"
				} ),


 */
	//$out .= '<div id="replace-me"></div>';
	$js = '
		function format ( d ) {
			var pregnant = "", photo = "";
			if ( d.photo )
			{
				photo = "<td colspan=2><img width=\"150\" src=\""+d.photo+"\" /></td>";
			}
		    return "<div class=\"slider\">"+
		        "<table cellpadding=\"5\" cellspacing=\"0\" border=\"0\" style=\"padding-left:50px;\">"+	        
		            "<tr>"+
		                "<td><label>Address:</label></td>"+
		                "<td colspan=\"3\">"+d.address+", "+d.location+"</td>"+
		             "</tr>"+
		             "<tr>"+
		                "<td><label>Contact:</label></td>"+
		                "<td colspan=\"3\">"+d.contact+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Pancard:</label></td>"+
		                "<td colspan=\"3\">"+d.pancard+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Aadhar:</label></td>"+
		                "<td colspan=\"3\">"+d.aadhar+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>National ID:</label></td>"+
		                "<td colspan=\"3\">"+d.voter_id+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Passport:</label></td>"+
		                "<td colspan=\"3\">"+d.passport+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Occupation/Edu:</label></td>"+
		                "<td>"+d.Occ+"/"+d.Education +"</td>"+
		                "<td><label>Company:</label></td>"+
		                "<td>"+d.Company+"</td>"+
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Designation:</label></td>"+
		                "<td>"+d.Designation+"</td>"+
		                "<td><label>Department:</label></td>"+
		                "<td>"+d.Dept+"</td>"+photo+
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Referral listed for:</label></td>"+
		                "<td><table><tr><td>10d</td><td>STP</td><td>SPL</td><td>20d</td><td>30d</td><td>45d</td><td>60d</td><td>TSC</td><td>10-Day Seva</td><td>LC Seva</td><td>Long Term Seva</td></tr>"+
		                "<tr><td>"+d.ref_10d+"</td><td>"+d.ref_stp+"</td><td>"+d.ref_spl+"</td><td>"+d.ref_20d+"</td><td>"+d.ref_30d+"</td><td>"+d.ref_45d+"</td><td>"+d.ref_60d+"</td><td>"+d.ref_tsc+"</td><td>"+d.ref_10dseva+"</td><td>"+d.ref_lcseva+"</td><td>"+d.ref_longseva+"</td></tr></table></td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Reason/Remarks:</label></td>"+
		                "<td>"+d.ref_reason+"</td>"+
		                "<td><label>"+d.ref_by_title+"</label></td>"+
		                "<td>"+d.ref_by+"</td>"+			            	
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Course Information:</label></td>"+
		                "<td><table><tr><td>Teen</td><td>10d</td><td>STP</td><td>SPL</td><td>TSC</td><td>20d</td><td>30d</td><td>45d</td><td>60d</td><td>Seva</td></tr>"+
		                "<tr><td>"+d.course_teen+"</td><td>"+d.course_10d+"</td><td>"+d.course_stp+"</td><td>"+d.course_spl+"</td><td>"+d.course_tsc+"</td><td>"+d.course_20d+"</td><td>"+d.course_30d+"</td><td>"+d.course_45d+"</td><td>"+d.course_60d+"</td><td>"+d.course_seva+"</td></tr></table></td>"+
		            "</tr>"+
                "<tr>"+
                "<td colspan=4><div class=\"activity-log-"+d.ref_id+"\">Loading Activity Log...</div></td>"+
                "</tr>"+
		        "</table>"+
		    "</div>";
		}

		(function ($) {
			$(document).ready(function(){

				var dataset = '.$data.';
				var comment = "";			

			    var buttonCommon = {
			        exportOptions: {
			            format: {
			            	header: function( data, column, node ) {
			            		var h = ["Name", "Gender", "Age", "Courses", "PhoneMobile", "PhoneHome", "PhoneOffice", "Email", "Education", "Occupation", "Company", "Designation/Dept", "Address", "Pin", "City", "State","Country", ""]; 
			            		return h[column];
			            	},
			                body: function ( data, row, column, node ) {
			                	var tmp = table.rows().data();
			                	var a;
			                	if ( column == 1 )
			                	{
			                		return tmp[ row ].gender; 
			                	}
			                	else if ( column == 2 )
			                	{
			                		return tmp[ row ].age; 
			                	}
			                	else if ( column == 3 )
			                	{
			                		a = "S: "+(+tmp[ row ].course_10d + +tmp[ row ].course_stp)+" L: "+(+tmp[ row ].course_20d + +tmp[ row ].course_30d + +tmp[ row ].course_45d + +tmp[ row ].course_60d + +tmp[ row ].course_spl + +tmp[ row ].course_tsc )+ " Seva: "+ tmp[ row ].course_seva; 
			                		return a; 
			                	}
			                	else if ( column == 4 )
			                	{
			                		return tmp[ row ].contact_mobile; 
			                	}
			                	else if ( column == 5 )
			                	{
			                		return tmp[ row ].contact_home; 
			                	}
			                	else if ( column == 6 )
			                	{
			                		return tmp[ row ].contact_office; 
			                	}
			                	else if ( column == 7 )
			                	{
			                		return tmp[ row ].contact_email; 
			                	}
			                	else if ( column == 8 )
			                	{
			                		return tmp[ row ].Education; 
			                	}
			                	else if ( column == 9 )
			                	{
			                		return tmp[ row ].Occ; 
			                	}
			                	else if ( column == 10 )
			                	{
			                		return tmp[ row ].Company; 
			                	}
			                	else if ( column == 11 )
			                	{
			                		return tmp[ row ].Designation + " / " + tmp[ row ].Dept; 
			                	}
			                	else if ( column == 12 )
			                	{
			                		return tmp[ row ].address; 
			                	}
			                	else if ( column == 13 )
			                	{
			                		return tmp[ row ].pin; 
			                	}
			                	else if ( column == 14 )
			                	{
			                		return tmp[ row ].city; 
			                	}
			                	else if ( column == 15 )
			                	{
			                		return tmp[ row ].state; 
			                	}
			                	else if (column == 16) {
			                		return tmp[ row ].country; 
			                	}
			                	return data;
			                }
			            }
			        }
			    };


			    var iOS = !!navigator.platform && /iPad|iPhone|iPod/.test(navigator.platform);

				var table = $("#table-applicants").DataTable({
					data: dataset,					
			        columns: [
			            {
			                "class":          "details-control",
			                "orderable":      false,
			                "data":           null,
			                "title":		  "Detail",
			                "defaultContent": ""
			            },				        
			            { data: "name", "title" : "Applicant Name" },
			            { data: "gender", "title" : "Gender" },
				        { data: "age", "title" : "Age"},
			   			{ data: "contact", "title" : "Email" },
				        { data: "ref_by", "title" : "Referred By" },
				        { data: "ref_start", "title" : "Start Date" },
				        { data: "ref_end", "title" : "End Date" },
				        { data: "del", "title" : "Delete" },
		            ],   	
				});

			    // Add event listener for opening and closing details
			    $("#table-applicants tbody").on("click", "td.details-control", function () {
			        var tr = $(this).closest("tr");
			        var row = table.row( tr );
			 
			        if ( row.child.isShown() ) {
			            // This row is already open - close it
			            $("div.slider", row.child()).slideUp( function () {
			                row.child.hide();
			                tr.removeClass("shown");
			            });
			        }
			        else {
			            // Open this row
			            row.child( format(row.data()), "no-padding" ).show();
			            tr.addClass("shown");
			 
			            $("div.slider", row.child()).slideDown();
                  $(".activity-log-"+row.data().ref_id).load("/referral/get-activity/"+row.data().ref_id);
			        }
			    });

	                
				$("#table-applicants tbody").on("click", ".ref-del", function() {
					return confirm("Are you sure you want to delete record?");
				});

        $("#table-applicants tbody").on("change", ".referral-readonly", function() {
          ref_id = $(this).attr("id");
          check_box = $(this);

          if($(this).is(":checked")) {

           $.confirm({
              title: "Add Readonly Flag to Referral.",
              buttons: {
                  Ok: {
                      text: "Ok",
                      btnClass: "btn-blue",
                      action: function () {
                          $.getJSON( "/referral/read-only/"+ref_id, { readonly: "1" } )
                    .done(function( json ) {
                      if (json.status == "OK")
                      {
                        alert(json.msg)
                      }
                      else
                        alert("Update failed!\n"+json.msg)
                    })
                    .fail(function( jqxhr, textStatus, error ) {
                      var err = textStatus + ", " + error;
                      alert("Error updating "+err);
                  });


                      }
                  },
                  Cancel: function () {
                      check_box.attr("checked", false);
                  },
              },
              onContentReady: function () {
                  // bind to events
                  var jc = this;
                  this.$content.find("form").on("submit", function (e) {
                      // if the user submits the form by pressing enter in the field.
                      e.preventDefault();
                      jc.$$formSubmit.trigger("click"); // reference the button and click it
                  });
              }
          });


          }
          else {

           $.confirm({
              title: "Remove Readonly Flag from Referral.",
              buttons: {
                  Ok: {
                      text: "Ok",
                      btnClass: "btn-blue",
                      action: function () {
                          $.getJSON( "/referral/read-only/"+ref_id, { readonly: "0" } )
                    .done(function( json ) {
                      if (json.status == "OK")
                      {
                        alert(json.msg)
                      }
                      else
                        alert("Update failed!\n"+json.msg)
                    })
                    .fail(function( jqxhr, textStatus, error ) {
                      var err = textStatus + ", " + error;
                      alert("Error updating "+err);
                  });


                      }
                  },
                  Cancel: function () {
                      check_box.attr("checked", true);
                  },
              },
              onContentReady: function () {
                  // bind to events
                  var jc = this;
                  this.$content.find("form").on("submit", function (e) {
                      // if the user submits the form by pressing enter in the field.
                      e.preventDefault();
                      jc.$$formSubmit.trigger("click"); // reference the button and click it
                  });
              }
          });


          }





        });

        $("body").on("click", ".get-at-info", function() {
          teacher_id = $(this).attr("id");
          $.getJSON( "/at-schedule/get-at-info/"+teacher_id )
            .done(function( json ) {
              if (json.status == "OK")
              {
                $.alert(
                  "<b><u>"+json.at_name+" ( "+json.at_year+" )</b></u><br><br>"+
                  "<b>Status:</b> "+json.at_status+"<br>"+
                  "<b>Mobile:</b> "+json.at_phone+"<br>"+
                  "<b>Email:</b> "+json.at_email+"<br>"+
                  "<b>Address:</b> "+json.at_address+"<br>"
                );
              }
              else
                $.alert("Not able to get AT Info")
            })
            .fail(function( jqxhr, textStatus, error ) {
            var err = textStatus + ", " + error;
            $.alert("Not able to get AT Info "+err);
            });
          return false;
        });

			});
		})(jQuery);

	';
	drupal_add_js($js, 'inline');	
	return $out;

}


function referral_delete( $id )
{
	global $user;

	$fields['r_updated_by'] = $user->uid;
	$fields['r_updated'] = date('Y-m-d H:i:s');
	$fields['r_deleted'] = 1;

  $readonly = db_query("select r_readonly from dh_referral where r_id=:r_id", array(":r_id"=>$id))->fetchField();

  if($readonly)
  {
    drupal_set_message("Deletion failed, referral is flagged readonly.", 'error');
  }
  else
  {
    db_update('dh_referral')->fields($fields)->condition('r_id', $id )->execute();
    db_query("update dh_applicant set a_referral=0 where a_referral=$id");
    add_referral_activity($id, "Deleted", "Referral Deleted");
    drupal_set_message("Referral deleted successfully.");
  }


	
	if (arg(0) == 'center-referral')
		drupal_goto ('center-referral/'.arg(1));
	else
		drupal_goto ('referral/'.arg(1));
}


function referral_readonly($id)
{
  global $user;

  $out = array();
  $out['status'] = 'OK';
  $out['msg'] ="";

  $fields['r_updated_by'] = $user->uid;
  $fields['r_updated'] = date('Y-m-d H:i:s');
  $fields['r_readonly'] = $_REQUEST['readonly'];

  if($id > 0 && $fields['r_readonly'] == "1")
  {
    db_update('dh_referral')->fields($fields)->condition('r_id', $id )->execute();
    $out['msg'] ="Added readonly flag to referral.";
    drupal_json_output($out);
    exit;
  }
  elseif($id > 0 && $fields['r_readonly'] == "0")
  {
    db_update('dh_referral')->fields($fields)->condition('r_id', $id )->execute();
    $out['msg'] ="Removed readonly flag from referral.";
    drupal_json_output($out);
    exit;
  }
  else
  {
    $out['status'] = 'Failed';
    $out['msg'] ="Invalid Params.";
    drupal_json_output($out);
    exit;
  }


}


function get_referral_activity($ref_id)
{

  $q = "select DATE_FORMAT(rl_tstamp, '%r, %D %M %Y'), rl_event, rl_msg, name from dh_referral_log left join users on dh_referral_log.rl_user=users.uid where rl_referral=:rl_referral order by rl_id desc";
  $res = db_query($q, array(':rl_referral' => $ref_id));

  $data = array();

  while($row = $res->fetchAssoc())
  {
    $data[] = $row;
  }

  $headers = array('Timestamp', 'Activity', 'Message', 'User');
  $out = '<h4>Referral Activity Log</h4>';
  if (!empty($data))
    $out .= theme('table', array('header' => $headers, 'rows' => $data)) ;
  print $out;
  exit;

}

function add_referral_activity($ref_id, $ref_event, $ref_msg)
{
  global $user;

  $uid = $user->uid;
  if(!$uid)
  {
    $system_uids = db_query("select td_key, td_val1 from dh_type_detail where td_type = 'COURSE-APPLICANT'")->fetchAllKeyed();
    $uid = $system_uids['COURSE-SYSTEM-UID'];
  }

  $f['rl_referral'] = $ref_id;
  $f['rl_event'] = $ref_event;
  $f['rl_msg'] = $ref_msg;
  $f['rl_user'] = $uid;
  db_insert('dh_referral_log')->fields($f)->execute();
}
