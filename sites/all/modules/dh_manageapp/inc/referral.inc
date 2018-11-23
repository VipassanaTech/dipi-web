<?php

function dh_referral_form($form, &$form_state, $edit_data = array()) 
{	
  	drupal_add_library('system', 'ui.autocomplete');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/location-common.js');  	
    drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/manageapp.js');
    drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');

 	$form['b'] = array('#markup' => '<h3>'.l("Back to Dashboard", "centre/".arg(1))."&nbsp|&nbsp".l("Back to Referral List", "referral/".arg(1)).'</h3>');
 	
 	if ( !empty($edit_data) )
 	{
 		$q = "select CONCAT(t_code,'-',t_gender) from dh_teacher where t_id=".$edit_data['r_listed_by'];
		$referral = db_query($q)->fetchField();
 	}

	$add = 0;
 

	$group = 'Personal Information';
	$form['#attributes'] = array('class' => array('container-inline')); 
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['s_f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#size' => '30', '#weight' => 1, 
		'#default_value' => isset($edit_data['s_f_name'])?$edit_data['s_f_name']:'', '#required' => 1, '#maxlength' => 50  );
	$form[$group]['s_m_name'] = array('#title' => 'Middle Name', '#type' => 'textfield', '#size' => '30', '#weight' => 2,
		'#default_value' => isset($edit_data['s_m_name'])?$edit_data['s_m_name']:'', '#maxlength' => 50);
	$form[$group]['s_l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#size' => '30', '#weight' => 3, 
		'#default_value' => isset($edit_data['s_l_name'])?$edit_data['s_l_name']:'', '#required' => 1, '#maxlength' => 50);
	$form[$group]['s_gender'] = array('#title' => 'Gender', '#type' => 'radios', '#options' => array("M" => 'Male', 'F' => 'Female')  , 
		'#default_value' => isset($edit_data['s_gender'])?$edit_data['s_gender']:'M', '#weight' => 4, '#required' => 1 , 
		'#ajax' => array(
		      // most notably 'mousedown', 'blur', and 'submit'.
		      // 'event' => 'change',
		      'callback' => 'ajax_section_dropdown_callback',
		      'wrapper' => 'dropdown-section-replace',
		    ), '#suffix' => '',
		);

	$form[$group]['s_dob'] = array('#title' => 'Date of Birth', '#type' => 'date_popup', '#size' => '20', '#weight' => 5, 
		'#default_value' => isset($edit_data['s_dob'])?$edit_data['s_dob']:'', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'), '#suffix' => ''); /*'<div class="form-item">DOB Checked<input type="checkbox" id="edit-checked-dob" /></div>'); */

	$form[$group]['age'] = array('#title' => 'Age', '#type' => 'textfield', '#size' => '5', '#weight' => 11, 
		'#default_value' => '', '#required' => 0, '#maxlength' => 10 );

	$form[$group]['s_phone_home'] = array('#title' => 'Phone (Home)', '#type' => 'textfield', '#size' => '20', '#weight' => 12, 
		'#default_value' => isset($edit_data['s_phone_home'])?$edit_data['s_phone_home']:'', '#required' => 0, '#maxlength' => 20 );
	
	$form[$group]['s_phone_office'] = array('#title' => 'Phone (Office)', '#type' => 'textfield', '#size' => '20', '#weight' => 13, 
		'#default_value' => isset($edit_data['s_phone_office'])?$edit_data['s_phone_office']:'', '#required' => 0, '#maxlength' => 20 );

	$form[$group]['s_phone_mobile'] = array('#title' => 'Mobile', '#type' => 'textfield', '#size' => '20', '#weight' => 14, 
		'#default_value' => isset($edit_data['s_phone_mobile'])?$edit_data['s_phone_mobile']:'', '#required' => 0, '#maxlength' => 15 );


	$form[$group]['s_address'] = array('#title' => 'Address', '#type' => 'textarea',  
		'#default_value' => isset($edit_data['s_address'])?$edit_data['s_address']:'', '#rows' => 3,  '#weight' => 20, '#required' => 1, 
		'#suffix' => '', '#maxlength' => 200 ); 

	$form[$group]['s_zip'] = array('#title' => 'Zip', '#type' => 'textfield', '#size' => '20', '#weight' => 25, 
		'#default_value' => (isset($edit_data['s_zip']))?$edit_data['s_zip']:'', '#maxlength' => 20 );
	$form[$group]['s_country'] = array('#title' => 'Country', '#type' => 'textfield', '#size' => '20', '#weight' => 26, 
		'#default_value' => isset($edit_data['s_country'])?$edit_data['s_country']:'', '#required' => 1, '#maxlength' => 5,
				'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	))
		);
	$city_name = '';
	if (!empty($edit_data))
	{
		if ((isset($edit_data['s_city'])) && ($edit_data['s_city'] <> ''))
			$city_name = db_query("select c_name from dh_city where c_id='".$edit_data['s_city']."'")->fetchField();
	}
	$form[$group]['s_state'] = array('#title' => 'State', '#type' => 'textfield', '#size' => '20', '#weight' => 27, 
		'#default_value' => isset($edit_data['s_state'])?$edit_data['s_state']:'', '#required' => 1, '#maxlength' => 5 );
	$form[$group]['s_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '20', '#weight' => 28, 
		'#default_value' => $add?'':$city_name, '#description' => isset($edit_data['s_city'])?$edit_data['s_city']:'', '#maxlength' => 100);

	$doc_types = array('' => 'Choose', 's_passport' => 'Passport', 's_pancard' => 'Pancard', 's_aadhar' => 'Aadhar', 's_voter_id' => 'National ID');
	$default_doc_type = '';
	$default_doc_id = '';
	//if (!$add || (isset($edit_data['s_passport'])) )
	if ( !empty($edit_data) )
	{
		if ( $edit_data['s_passport'] <> '' ) $default_doc_type = 's_passport';
		if ( $edit_data['s_pancard'] <> '' ) $default_doc_type = 's_pancard';
		if ( $edit_data['s_aadhar'] <> '' ) $default_doc_type = 's_aadhar';
		if ( $edit_data['s_voter_id'] <> '' ) $default_doc_type = 's_voter_id';
		if ($default_doc_type <> '')
			$default_doc_id = $edit_data[$default_doc_type];		
	};

	$form[$group]['document_type'] = array('#title' => 'Photo ID', '#type' => 'select', '#multiple' => false,  '#weight' => 29, 
		'#default_value' => $default_doc_type, '#options' => $doc_types, '#required' => 0, );
	$form[$group]['document_id'] = array('#title' => 'Identifier', '#type' => 'textfield', '#size' => '20', '#weight' => 30, 
		'#default_value' => $default_doc_id, '#required' => 0, '#maxlength' => 20 );
  
  $group = 'Referral Information';
  $form[$group] = array('#type' => 'fieldset', '#title' => 'Referral Information', '#collapsible' => true, '#collapsed' => false);



  $form[$group]['r_listed_10d'] = array('#title' => '&nbsp;10-Day', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_10d'])?$edit_data['r_listed_10d']:'0',  '#weight' => 13);
	$form[$group]['r_listed_stp'] = array('#title' => '&nbsp;STP', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_stp'])?$edit_data['r_listed_stp']:'0',  '#weight' => 14);
	$form[$group]['r_listed_spl'] = array('#title' => '&nbsp;10D-SPL', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_stp'])?$edit_data['r_listed_spl']:'0',  '#weight' => 14.1);
    $form[$group]['r_listed_20d'] = array('#title' => '&nbsp;20-Day', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_20d'])?$edit_data['r_listed_20d']:'0',  '#weight' => 15);
    $form[$group]['r_listed_30d'] = array('#title' => '&nbsp;30-Day', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_30d'])?$edit_data['r_listed_30d']:'0',  '#weight' => 16);
    $form[$group]['r_listed_45d'] = array('#title' => '&nbsp;45-Day', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_45d'])?$edit_data['r_listed_45d']:'0',  '#weight' => 17);
    $form[$group]['r_listed_60d'] = array('#title' => '&nbsp;60-Day', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_60d'])?$edit_data['r_listed_60d']:'0',  '#weight' => 18);
    $form[$group]['r_listed_tsc'] = array('#title' => '&nbsp;TSC', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_tsc'])?$edit_data['r_listed_tsc']:'0',  '#weight' => 19);
    $form[$group]['r_listed_10d_seva'] = array('#title' => '&nbsp;10-day Seva', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_10d_seva'])?$edit_data['r_listed_10d_seva']:'0',  '#weight' => 20);
    $form[$group]['r_listed_lc_seva'] = array('#title' => '&nbsp;LC Seva', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_lc_seva'])?$edit_data['r_listed_lc_seva']:'0',  '#weight' => 21);
    $form[$group]['r_listed_longseva'] = array('#title' => '&nbsp;Long-term Seva', '#type' => 'checkbox',
      '#default_value' => isset($edit_data['r_listed_longseva'])?$edit_data['r_listed_longseva']:'0',  '#weight' => 22);

    $form[$group]['checkall'] = array('#type' => 'checkbox', '#title' => t('All'), '#attributes' => array('onclick' => 'checkUncheckAll(this);'), '#weight' => 23);

    $form[$group]['r_start'] = array('#title' => 'Start Date', '#type' => 'date_popup', '#size' => '20', '#weight' => 23.1, 
		'#default_value' => isset($edit_data['r_start'])?$edit_data['r_start']:'', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'), '#suffix' => '');

    $form[$group]['r_end'] = array('#title' => 'End Date', '#type' => 'date_popup', '#size' => '20', '#weight' => 23.2, 
		'#default_value' => isset($edit_data['r_end'])?$edit_data['r_end']:'', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'), '#suffix' => '');
  
    $form[$group]['r_reason'] = array('#title' => 'Referral Reason', '#type' => 'textarea',  
	  '#default_value' => isset($edit_data['r_reason'])?$edit_data['r_reason']:'', '#rows' => 3,  '#weight' => 24, '#required' => 1, 
		'#suffix' => '', '#maxlength' => 200 );
  
    $form[$group]['r_listed_by'] = array( '#title' => 'Referred By',  '#type' => 'textfield', 
		'#weight' => 1, '#size' => 25, '#required' => 0, '#weight' => 70,
		'#default_value' => isset($edit_data['r_listed_by'])?$referral:'',
 		'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),
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

				autocomplete_country("#edit-a-country");
				autocomplete_state("#edit-s-state", "#edit-s-country");
				autocomplete_city("#edit-s-city", "#edit-s-country", "#edit-s-state");

			  	$("#edit-s-zip").focusout(function(){ 
		//		  		alert("hi");
			  		do_pincode($(this).val(), "#edit-s-country", "#edit-s-state", "#edit-s-city" ); 
			  	});


				$("#edit-r-listed-by").autocomplete({
					source: function( request, response ) {
						$("#edit-r-listed-by").addClass( "throbbing" );
				        $.ajax({
				          url: "/autocomplete/get-teacher/" + request.term,
				          dataType: "json",
				          success: function( data ) {
				          	$("#edit-r-listed-by").removeClass( "throbbing" );
				            response( data );
				          }
				        });
					},
		  			minLength: 1,
		  			select: function( event, ui ) {
		        		//console.log( "Selected: " + ui.item.value + " aka " + ui.item.code );
		        		$( "#edit-r-listed-by" ).val( ui.item.code );
		        		return false;
		      		}
		  		});

			});
		})(jQuery);
	';



	drupal_add_js($js, 'inline');
	if (arg(2) == $add)
		$action = 'Add Referral';
	else
		$action = 'Update';
	//$form['return'] = array('#type' => 'hidden', '#value' => $_SERVER['REQUEST_URI']);
	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 50);
  
  return $form;
}


function dh_referral_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];
	
	$ajax = '';
	if (arg(3) == 'edit')
		$form_state['redirect'] = 'referral/'.arg(1);
	elseif ( arg(2) == 'add')
		$form_state['redirect'] = 'referral/'.arg(1);

	foreach ($form_state['input'] as $key => $value) 
	{
		if ( substr($key,0,2) == 's_')
			$app[$key] = $value;
		if ( substr($key,0,2) == 'r_')
			$app_r[$key] = $value;
	}
	
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

	if ($input['document_type'] <> '')
		$app[$input['document_type']] = $input['document_id'];


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
	$app['s_updated_by'] = $user->uid;
	$app['s_updated'] = date('Y-m-d H:i:s');
    $app['s_f_name'] = ucwords(strtolower($app['s_f_name']) );
    $app['s_l_name'] = ucwords(strtolower($app['s_l_name']) );
    //$app['s_r_listed'] = '1';
    if (arg(3) == 'edit')
	{
		$app_id = arg(1);
		db_update('dh_student')->fields($app)->condition('s_id', $app_id )->execute();

	}

	else 
	{
		$app['s_created_by'] = $user->uid;
	    $app['s_created'] = date('Y-m-d H:i:s');
		$app_id = db_insert('dh_student')->fields($app)->execute();
	}

	/*insert or update dh_referral*/
	$temp = explode("-", $app_r['r_listed_by']);
	$q = "select t_id from dh_teacher where t_code='".$temp[0]."' and t_gender='".$temp[1]."'";
	$referral = db_query($q)->fetchField();
	$app_r['r_listed_by'] = $referral; 

	$app['r_teacher_code'] = $app_r['r_listed_by'];

	if ( arg(3) == 'edit')
	{
		$app_r['r_student'] = $app_id;
		$app_r['r_updated_by'] = $user->uid;
		$app_r['r_updated'] = date('Y-m-d H:i:s');
		db_update('dh_referral')->fields($app_r)->condition('r_student', $app_id )->execute();
	}

	else 
	{
		$app_r['r_student'] = $app_id;
		$app_r['r_created_by'] = $user->uid;
		$app_r['r_created'] = date('Y-m-d H:i:s');
		db_insert('dh_referral')->fields($app_r)->execute();
	}

	drupal_set_message(t("Details updated successfully"), 'status');

}


function dh_addedit_referral()
{

	if ( arg(2) == 'add')
		
			return drupal_get_form('dh_referral_form');

	if (arg(3) == 'edit')
	{
	
		$row = db_query("select * from dh_referral left join dh_student on r_student=s_id 
		  left join dh_student_course on r_student=sc_student left join dh_teacher on r_listed_by=t_id
			where s_id='".arg(2)."'")->fetchAssoc();
			}

		return drupal_get_form('dh_referral_form', $row);

}


function dh_referral_results()
{
	global $user;
  	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];

  	$centre_id = '';
	if (arg(1) <> '')
		$centre_id = arg(1);
	
	$out = '<h2>'.l("Back to Dashboard", "centre/".arg(1))."&nbsp;&nbsp;";

 	if (user_access ('manage referral list'))
 		$out .= "|&nbsp;&nbsp;".l("Add Referral", "referral/".arg(1).'/add');
 	$out .= '</h2>';


	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/datatables.min.js' );
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/datatables.min.css' );
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


	$q = "select a.s_id, concat(s_f_name,' ',s_l_name) as 'Full Name', s_gender, s_city, ci.c_name as 'City', s.s_name as 'State', co.c_name as 'Country', s_address, s_zip, s_email, s_phone_home, s_phone_office, s_phone_mobile, s_passport, s_aadhar, s_voter_id, s_pancard, s_education, s_occupation, s_occupation_past, s_company, s_designation, s_department, s_photo, TIMESTAMPDIFF( YEAR, s_dob, CURDATE() ) as 'Age', sci_10d, sci_stp, sci_spl, sci_20d, sci_30d, sci_45d, sci_60d, sci_tsc, sci_teacher,sci_service, CONCAT( IFNULL(sci_first_year, ''),'-',IFNULL(sci_first_month, ''),'-',IFNULL(sci_first_day, ''), ', ', sci_first_location_str) as 'first_course', sc_desc_physical,  sc_desc_medication,  sc_desc_mental, sc_desc_other_technique, sc_addiction_current, sc_uri, r_listed_10d, r_listed_stp, r_listed_spl, r_listed_20d, r_listed_30d, r_listed_45d, r_listed_60d, r_listed_tsc, r_listed_10d_seva, r_listed_lc_seva, r_listed_longseva, concat(t_f_name,' ',t_l_name) as 'Referred By', r_reason, r_start, r_end, r_deleted from dh_student a left join dh_student_course sc on (a.s_id=sc_student) left join dh_student_course_input sci on (a.s_id=sci_student) left join dh_referral r on (a.s_id=r_student) left join dh_country co on (s_country=co.c_code) left join dh_state s on (s_state = s.s_code and s.s_country = co.c_code) left join dh_city ci on (s_city=ci.c_id) left join dh_teacher on (t_id=r_listed_by) where  (a.s_id=r_student) and (r_deleted = 0) group by a.s_id order by r.r_created";

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
		$pdf_link = '';
		if ( $r['sc_uri'] <> '' )
			$pdf_link = '&nbsp;('.l("PDF", file_create_url($r['sc_uri']), array('attributes' => array("target" => "_blank") ) ).')';
		
		$rs['sid'] = $r['s_id'];

		if (user_access('manage referral list'))
		  $rs['name'] = _ma_make_link($r['Full Name'],'referral/'.arg(1).'/'.$r['s_id'].'/edit').$pdf_link;
		else
		  $rs['name'] = $r['Full Name'];
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
		$rs['ref_10dseva'] = $r['r_listed_longseva']?"Yes":"No";
		$rs['ref_by'] = $r['Referred By'];
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

	if (user_access('delete referral list'))
		$del_action = 'return "<a href=\"#\" action=\"Del\" class=\"applicant-del\">Delete</a>";';
	else
		$del_action = 'return "No access"';

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
		                "<td colspan=\"4\">"+d.address+"</td>"+
		                "<td>"+d.location+"</td>"+
		             "</tr>"+
		             "<tr>"+
		                "<td><label>Contact:</label></td>"+
		                "<td colspan=\"3\">"+d.contact+"</td>"+
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
		                "<tr><td>"+d.ref_10d+"</td><td>"+d.ref_stp+"</td><td>"+d.ref_spl+"</td><td>"+d.ref_20d+"</td><td>"+d.ref_30d+"</td><td>"+d.ref_45d+"</td><td>"+d.ref_60d+"</td><td>"+d.ref_tsc+"</td><td>"+d.ref_10dseva+"</td><td>"+d.ref_lcdseva+"</td><td>"+d.ref_longseva+"</td></tr></table></td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Reason:</label></td>"+
		                "<td>"+d.ref_reason+"</td>"+
		                "<td><label>Referral By:</label></td>"+
		                "<td>"+d.ref_by+"</td>"+			            	
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Mental Heath:</label></td>"+
		                "<td>"+d.mental+"</td>"+			            	
		                "<td><label>Physical Health:</label></td>"+
		                "<td>"+d.physical+"</td>"+			            	
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Course Information:</label></td>"+
		                "<td><table><tr><td>10d</td><td>STP</td><td>SPL</td><td>TSC</td><td>20d</td><td>30d</td><td>45d</td><td>60d</td><td>Seva</td></tr>"+
		                "<tr><td>"+d.course_10d+"</td><td>"+d.course_stp+"</td><td>"+d.course_spl+"</td><td>"+d.course_tsc+"</td><td>"+d.course_20d+"</td><td>"+d.course_30d+"</td><td>"+d.course_45d+"</td><td>"+d.course_60d+"</td><td>"+d.course_seva+"</td></tr></table></td>"+
		                "<td><label>Course Details:</label></td>"+
		                "<td>First: "+d.first_course+"<br>Last: "+d.last_course+"</td>"+
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Remarks:</label></td>"+
			            "<td>"+d.ref_remarks+"</td>"+
 		            "</tr>"+		       
 		            "<tr>"+
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
			                	if ( column == 0 )
			                	{
			                		a = jQuery("<span>").html(tmp[ row ].name).text();
			                		a = a.replace("(PDF)", "");
			                		return a;
			                	}
			                	else if ( column == 1 )
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
					"columnDefs": [
					            {
					                "targets": [ 11, 12, 13, 14, 15, 16 ],
					                "visible": false,
					                "searchable": false
					            }
			        ],					
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
			   			{ data: "contact_email", "title" : "Email" },
			            { data: "city", "title" : "City" },
			            { data: "state", "title" : "State" },
			            { data: "country", "title" : "Country" },
				        { data: "ref_by", "title" : "Referral By" },
				        { data: "ref_start", "title" : "Referred On" },
				        { data: null, "title" : "Delete", render: function(data, type, row) {
				        	'.$del_action.'	
				        } },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" }
		            ],
                  	/*buttons: ["copy","csv","excel","pdf","print"], */ 
					buttons:   [  
						'.$button_access.'                        
			         ]                  	
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
			        }
			    });

				$("#table-applicants tbody").on("change", ".status-select", function() {
					var s = $(this);
					//alert($(this).val());
					var tmp = $(this).val();
			        var tr = s.closest("tr");
			        var row = table.row( tr );
	                
				$("#table-applicants tbody").on("click", ".applicant-del", function() {
			        var tr = $(this).closest("tr");
			        var row = table.row( tr );
			        var id = row.data().sid;
					var r = confirm("Are you sure you want to delete applicant "+$("<div/>").html(row.data().name).text()+" ?");
					if (r == true)
					{
       					$(tr).loading();
						$.getJSON( "/referral/"+id+"/delete", {} )
						  .done(function( json ) {
						  	if (json.status == "OK")
						  	{
						  		//row.delete();
								tr.css("background-color", "#ff0000");						  		
						  	}
						  	else
						  		alert("Update failed "+json.msg);
						  	$(tr).loading("stop");
						  	//row.remove();
						  })
						  .fail(function( jqxhr, textStatus, error ) {
						    var err = textStatus + ", " + error;
						    alert("Error updating "+err);
						    $(tr).loading("stop");
						});
						}
						});						
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
	$temp = db_query("select a_center, CONCAT(a_f_name, ' ', a_l_name) as 'name' from dh_studnet where s_id=$id");
	$row = $temp->fetchAssoc();
	$fields = array();
	$fields['l_center'] = $row['a_center'];
	$fields['l_module'] = 'Applicant';
	$fields['l_identifier'] = $id;
	$fields['l_event'] = 'Deleted';
	$fields['l_msg'] = 'Applicant '.$row['name']." ($id) Deleted";
	$fields['l_user'] = $user->uid;
	db_insert('dh_log')->fields( $fields )->execute();

	$q = "update dh_referral set r_deleted = 1 where r_student=$id";
	db_query($q);
	
}


?>