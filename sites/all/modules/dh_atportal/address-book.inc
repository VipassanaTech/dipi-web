<?php

function address_book_search()
{
	global $databases;
	// DB table to use
	$table = 'dh_teacher';
	 
	// Table's primary key
	$primaryKey = 't_id';
	 
	// Array of database columns which should be read and sent back to DataTables.
	// The `db` parameter represents the column name in the database, while the `dt`
	// parameter represents the DataTables column identifier. In this case simple
	// indexes
	$edit_at = function($d, $row) { return $d; };
	if (user_access('add assistant teacher'))
		$edit_at = function($d, $row) { return $d." (".l("Edit", "/at-portal/address-book/edit/".strtolower($d.".".$row['t_gender'])).')'; };
	$columns = array(
	    array( 'db' => 't_code', 'dt' => 0, 'formatter' => $edit_at),
	    array( 'db' => 't_gender',     'dt' => 1 ),
	    array( 'db' => 't_f_name', 'dt' => 2 ),
	    array( 'db' => 't_l_name',  'dt' => 3 ),
	    array( 'db' => 't_status',     'dt' => 4 ),
	    array( 'db' => 't_email',     'dt' => 5 ),
	    array( 'db' => 't_country',   'dt' => 6 ),
	    array( 'db' => 't_mob_phone',     'dt' => 7 ),
	    array( 'db' => 't_sat',     'dt' => 8, 'formatter' => function($d, $row) { return ($d?'Yes':''); } ),
	    array( 'db' => 't_ct',     'dt' => 9, 'formatter' => function($d, $row) { return ($d?'Yes':''); } ),
	    array( 'db' => 't_full_t',     'dt' => 10, 'formatter' => function($d, $row) { return ($d?'Yes':''); }),
	    array( 'db' => 't_cat',     'dt' => 11, 'formatter' => function($d, $row) { return ($d?'Yes':''); } ),
	    array( 'db' => 't_bhante',     'dt' => 12 , 'formatter' => function($d, $row) { return ($d?'Yes':''); }),
	);
	// SQL server connection information
	$sql_details = array(
	    'user' => $databases['default']['default']['username'],
	    'pass' => $databases['default']['default']['password'],
	    'db'   => $databases['default']['default']['database'],
	    'host' => $databases['default']['default']['host']
	);
	 
	 
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	 * If you just want to use the basic configuration for DataTables with PHP
	 * server-side, there is no need to edit below this line.
	 */
	 
	require( dirname(__FILE__).'/ssp.class.php' );
	 
	drupal_json_output( SSP::simple( $_GET, $sql_details, $table, $primaryKey, $columns ) );
	exit;
}


function dh_atportal_address_book()
{
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

	$out = '';
	if (user_access('add assistant teacher'))
		$out = '<h2>'.l("Add Assistant Teacher", "at-portal/address-book/add").'</h2>';
	$header = array('Code', 'Gender', 'FName', 'LName', 'Status', 'Email', 'Country', 'Mobile', 'SAT', 'CT', 'FullT', 'CAT', 'Bhante'); 
	$attributes = array('id' => 'at-address-book');

	//$out = theme('table', array('header' => $header, 'rows' => $rows_pending, 'attributes' => $attributes));
	// $out .= theme('table', array('header' => $header, 'rows' => array(array('1','2','3','4','5','1','2','3')), 'attributes' => $attributes));
  $out .= theme('table', array('header' => $header, 'rows' => array(array()) ,'attributes' => $attributes));

	$js = '
		(function ($) {
			function do_datatable( tid )
			{
				var table = $(tid).DataTable({
					dom: "lfr<\"clear\">tip",
			        "ajax": "/at-portal/address-book/ajax"
			    });			
			}
			$(document).ready(function(){
				do_datatable("#at-address-book");
			});
		})(jQuery);

	';
	drupal_add_js($js, 'inline');

	return $out;
}

function dh_atportal_profile($form, &$form_state, $edit_data = array())
{
	global $user;
	$at_code = arg(3);
	$add = 0;
	if (count($edit_data) == 0)
		$add = 1;

	$form['a'] = array('#markup' => '<h2>Edit Profile</h2>');

	$group = 'Personal Information';
	$form['#attributes'] = array('class' => array('container-inline'));
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['t_f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#size' => '30', '#weight' => 1.1, 
		'#default_value' => isset($edit_data['t_f_name'])?$edit_data['t_f_name']:'', '#required' => 1, '#maxlength' => 50  );	
	$form[$group]['t_l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#size' => '30', '#weight' => 2, 
		'#default_value' => isset($edit_data['t_l_name'])?$edit_data['t_l_name']:'', '#required' => 1, '#maxlength' => 50);
	$form[$group]['t_gender'] = array('#title' => 'Gender', '#type' => 'radios', '#options' => array("M" => 'Male', 'F' => 'Female')  , 
		'#default_value' => isset($edit_data['t_gender'])?$edit_data['t_gender']:'M', '#weight' => 3, '#required' => 1 , 
		/*'#ajax' => array(
		      // most notably 'mousedown', 'blur', and 'submit'.
		      // 'event' => 'change',
		      'callback' => 'ajax_section_dropdown_callback',
		      'wrapper' => 'dropdown-section-replace',
		    ),*/
		);

	$form[$group]['t_dob'] = array('#title' => 'Date of Birth', '#type' => 'date_popup', '#size' => '20', '#weight' => 4, 
		'#default_value' => isset($edit_data['t_dob'])?$edit_data['t_dob']:'', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
    		'maxDate' => 0, '#required' => 1, 
    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'), '#suffix' => '');

	$form[$group]['t_address'] = array('#title' => 'Address', '#type' => 'textarea', '#size' => '150', '#weight' => 5, 
		'#default_value' => isset($edit_data['t_address'])?$edit_data['t_address']:'', '#required' => 1, '#maxlength' => 200);	
	$form[$group]['t_pincode'] = array('#title' => 'Zip', '#type' => 'textfield', '#size' => '10', '#weight' => 5.2, 
		'#default_value' => (isset($edit_data['t_pincode']))?$edit_data['t_pincode']:'', '#maxlength' => 10 );

	$country_codes = db_query("select c_code, CONCAT(c_name,' ','(',c_code,')') from dh_country order by c_name")->fetchAllKeyed();	
	$country_codes['0'] = 'Choose';
	$form[$group]['t_country'] = array('#title' => 'Country Code', '#type' => 'select', '#options' =>  $country_codes,  '#weight' => 6, 
		'#default_value' => isset($edit_data['t_country'])?$edit_data['t_country']:'0', '#required' => 0, );

	$form[$group]['t_state'] = array('#title' => 'State', '#type' => 'textfield', '#size' => '20', '#weight' => 6.1, 
		'#default_value' => isset($edit_data['t_state'])?$edit_data['t_state']:'', '#maxlength' => 20,
    '#attached' => array(
          'library' => array(
            array('system', 'ui.autocomplete'),
        )),
     );

	$form[$group]['t_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '20', '#weight' => 6.2, 
		'#default_value' => isset($edit_data['t_city'])?$edit_data['t_city']:'', '#maxlength' => 20 );
	
	$form[$group]['t_mob_phone'] = array('#title' => 'Mobile', '#type' => 'textfield', '#size' => '20', '#weight' => 7, 
		'#default_value' => isset($edit_data['t_mob_phone'])?$edit_data['t_mob_phone']:'', '#required' => 0, '#maxlength' => 50 );
	$form[$group]['t_email'] = array('#title' => 'Email', '#type' => 'textfield', '#size' => '25', '#weight' => 8, 
		'#default_value' => isset($edit_data['t_email'])?$edit_data['t_email']:'', '#required' => 0, '#maxlength' => 50 );

	if (user_access('add assistant teacher'))
	{
		$group = 'AT Role Information';
		$form[$group] = array('#type' => 'fieldset', '#title' => 'Role Information',  '#collapsible' => true, '#collapsed' => false);
		$statuses = array('Active' => 'Active', 'Inactive' => 'Inactive', 'Deprecated' => 'Deprecated', 'Training' => 'Training',);
		$default_status = '';
		if ( !empty($edit_data) )
		{
			if ( $edit_data['t_status'] == 'Active' ) $default_status = 'Active';
			if ( $edit_data['t_status'] == 'Inactive' ) $default_status = 'Inactive';		
			if ( $edit_data['t_status'] == 'Deprecated' ) $default_status = 'Deprecated';
			if ( $edit_data['t_status'] == 'Training' ) $default_status = 'Training';
		};	
		$form[$group]['t_status'] = array('#title' => 'Status', '#type' => 'select', '#options' =>  $statuses,
			'#default_value' => $default_status, '#weight' => 9, '#empty_option' => 'Choose', '#required' => 1);
		$form[$group]['t_code'] = array('#title' => 'AT Code', '#type' => 'textfield', '#size' => '10', '#weight' => 10, 
			'#required' => 1,
			'#default_value' => isset($edit_data['t_code'])?$edit_data['t_code']:'', '#maxlength' => 10  );
		$form[$group]['t_year_appointed'] = array('#title' => 'Year of Appointment', '#type' => 'textfield', '#size' => '10', '#weight' => 10.1, 
			'#default_value' => isset($edit_data['t_year_appointed'])?$edit_data['t_year_appointed']:'', '#maxlength' => 10  );
		$form[$group]['t_sat'] = array('#title' => '&nbsp;SAT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_sat'])?$edit_data['t_sat']:'0',  '#weight' => 13);
		$form[$group]['t_full_t'] = array('#title' => '&nbsp;T', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_full_t'])?$edit_data['t_full_t']:'0',  '#weight' => 14);
		$form[$group]['t_ct'] = array('#title' => '&nbsp;CT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_ct'])?$edit_data['t_ct']:'0',  '#weight' => 14.1);
	    $form[$group]['t_cat'] = array('#title' => '&nbsp;CAT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_cat'])?$edit_data['t_cat']:'0',  '#weight' => 15);
	    $form[$group]['t_bhante'] = array('#title' => '&nbsp;Bhikkhu', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_bhante'])?$edit_data['t_bhante']:'0',  '#weight' => 16);
	    $form[$group]['t_responsibility'] = array('#title' => 'Additional Responsibility', '#type' => 'textarea', '#size' => '100', '#weight' => 17, 
			'#default_value' => isset($edit_data['t_responsibility'])?$edit_data['t_responsibility']:'', '#maxlength' => 200  );
	    $form[$group]['t_area'] = array('#title' => 'Area of Responsibility', '#type' => 'textarea', '#size' => '100', '#weight' => 18, 
			'#default_value' => isset($edit_data['t_area'])?$edit_data['t_area']:'', '#maxlength' => 255  );
	}	

	if ($add)
		$action = 'Add';
	else
		$action = 'Update';
	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 110);

    $js ='
    (function ($) {
      $(document).ready(function(){
        autocomplete_state("#edit-t-state", "#edit-t-country");
      });
    })(jQuery);
  ';
  drupal_add_js($js, 'inline');

	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
  $form['#attached']['js'][] = drupal_get_path('module', 'dh_manageapp').'/js/location-common.js';

	return $form;		
}

function dh_manage_at()
{
	global $user;
	if (arg(3) == 'me')
	{		
		$at = explode(".", $user->name);
		$at_code = $at[0];
		$gender = $at[1];
	}
	else
	{
		$at_code = substr(arg(3), 0, strpos(arg(3), "."));
		$gender = substr(arg(3), -1);
	}
	switch( arg(2) )
	{
		case 'add':
			return drupal_get_form('dh_atportal_profile');
			break;
		case 'edit':						
			$row = db_query("select * from dh_teacher where t_code='".$at_code."' and t_gender='".$gender."'")->fetchAssoc();
			return drupal_get_form('dh_atportal_profile', $row);
			break;
	}
}

function dh_atportal_profile_submit($form, &$form_state)
{
	global $user;	
	if (arg(3) == 'me')
	{		
		$at = explode(".", $user->name);
		$at_code = $at[0];
		$gender = $at[1];
	}
	else
	{
		$at_code = substr(arg(3), 0, strpos(arg(3), "."));
		$gender = substr(arg(3), -1);
	}

	$input = $form_state['input'];
	if (arg(2) == 'edit')
		$form_state['redirect'] = 'at-portal';
	elseif ( arg(2) == 'add')
		$form_state['redirect'] = 'at-portal/address-book';

	foreach ($form_state['input'] as $key => $value) 
	{
		if ( substr($key,0,2) == 't_')
			$t[$key] = $value;
	}

	if ($t['t_email'])
		$t['t_email'] = trim($t['t_email']);

	if ($t['t_mob_phone'])
		$t['t_mob_phone'] = trim($t['t_mob_phone']);

	if ($t['t_dob']['date'] == '')
		$t['t_dob'] = NULL;

	if (user_access('add assistant teacher'))
	{
			if ($t['t_sat'] == '')
				$t['t_sat'] = '0';
			if ($t['t_full_t'] == '')
				$t['t_full_t'] = '0';
			if ($t['t_ct'] == '')
				$t['t_ct'] = '0';
			if ($t['t_cat'] == '')
				$t['t_cat'] = '0';
			if ($t['t_bhante'] == '')
				$t['t_bhante'] = '0';
	}


	if ( arg(2) == 'edit')
	{
		//drupal_set_message(print_r($t, true));
		$t['t_updated_by'] = $user->uid;
		$t['t_updated'] = date('Y-m-d H:i:s');
		db_update('dh_teacher')->fields($t)->condition('t_code', $at_code )->condition('t_gender', $gender )->execute();
	}

	else 
	{
		$t['t_created_by'] = $user->uid;
		$t['t_created'] = date('Y-m-d H:i:s');
		$t['t_updated_by'] = $user->uid;
		$t['t_updated'] = date('Y-m-d H:i:s');
		db_insert('dh_teacher')->fields($t)->execute();
	}

	drupal_set_message(t("Details updated successfully"), 'status');

}
