<?php

define('LEFT_TO_RIGHT', 1);
define('RIGHT_TO_LEFT', 2);

function dh_manage_centre()
{
	if ( arg(1) == '' )
		return drupal_get_form('dh_zero_select_centre');
	drupal_add_css('
	  .multi-column {
	  	  columns: 1;
		  -webkit-columns: 1;
  		  -moz-columns: 1;
	  }
  	  h2 { margin-top: 0 !important; }

 	  @media only screen and (min-width: 768px) {
    	  .multi-column {
    	  	 columns: 3;
		     -webkit-columns: 3;
  		     -moz-columns: 3;
    	  }
    	  html { background : #ffffff; }
	  }    	

		', 'inline');

	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');

	$name = db_query("select c_name from dh_center where c_id=".arg(1))->fetchField();
	drupal_set_title("Manage $name");
	$out = '<div style="color:#ff0000; font-size:26px; margin-bottom:10px;">'.variable_get('important_notice', '').'</div>';
	$modules = array('Centre Settings' => 'centre/'.arg(1).'/edit', 'Manage Courses' => 'manage-course/'.arg(1) , 'Manage Letters' => 'letters/'.arg(1),  'Search' => 'search-app/'.arg(1), 'Daily Activity' => 'daily-activity/'.arg(1), 'AT Schedule' => 'at-schedule/'.arg(1), 'Referral List' => 'referral/'.arg(1)  ); // 'Zero Day' => 'zero-day/'.arg(1)
	$out .= '<ul class="multi-column">';
	foreach($modules as $key => $val )
	{
		if (drupal_valid_path($val))
			$out .= '<h2><li>'.l($key, $val).'</li></h2>';
	}
	$out .= '</ul>';
	$with_header = false;
	$temp = drupal_get_form('dh_zero_select_course', $with_header);
	$out .= drupal_render($temp);
	$out .= dashboard( arg(1) );

	return $out;

	//return $out;
}

function dh_addedit_centre()
{
	if ( arg(1) == 'add')
		return drupal_get_form('dh_ma_centre_form');

	if (arg(2) == 'edit')
	{
		$row = db_query("select * from dh_center c left join dh_center_setting cs on c.c_id=cs.cs_center where c.c_id=".arg(1))->fetchAssoc() ;
		$out = drupal_get_form('dh_ma_centre_form', $row);
		return $out;		
	}
}


function dh_ma_centre_form($form, &$form_state, $edit_data = array())
{
	include_once(drupal_get_path('module', 'dh_manageapp').'/inc/qrcode.inc');
	drupal_add_library('system', 'ui.core');
	drupal_add_library('system', 'ui.dialog');
	drupal_add_library('system', 'ui.datepicker');
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/location-common.js');

	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
	$add = 0;
	if (count($edit_data) == 0)
		$add = 1;
	$group = 'Centre Details';
	$form['#attributes'] = array('class' => array('container-inline'));
	$identifier = '';
	$city_name = ''; $qr_code_img = '';
	if (!$add)
	{
		$form['b'] = array('#markup' => '<h2>'.l("Back to Centre Dashboard", "centre/".arg(1)).'</h2>');
		$ID_CODE =  simple_crypt(arg(1));
		$identifier = 'Identifier: '.$ID_CODE;
		$qr_code = drupal_get_path('module', 'dh_manageapp').'/cache/centre-'.arg(1).'.png';
		QRcode::png($ID_CODE, $qr_code);
		$qr_code_img = '<img src="/'.$qr_code.'" />';
		if ($edit_data['c_city'] <> '')
			$city_name = db_query("select c_name from dh_city where c_id='".$edit_data['c_city']."'")->fetchField();
	}

	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['c_name'] = array('#title' => 'Centre Name', '#type' => 'textfield', '#size' => '50', '#weight' => 1, 
		'#default_value' => $add?'':$edit_data['c_name'], '#required' => 1, '#suffix' => $identifier.$qr_code_img  );
	if ($add)
	{	$form[$group]['c_subdomain'] = array('#title' => 'Dhamma.org Identifier', '#type' => 'textfield', '#size' => '20', '#weight' => 2,
			'#default_value' => $add?'':$edit_data['c_subdomain'], );
		$form[$group]['c_dana'] = array('#title' => '&nbsp;Enable Dana?', '#type' => 'checkbox',
      		'#default_value' => $add?'':$edit_data['c_dana'],  '#weight' => 3);}
	$form[$group]['c_trust'] = array('#title' => 'Trust Name', '#type' => 'textfield', '#size' => '50', '#weight' => 4, 
		'#default_value' => $add?'':$edit_data['c_trust'], '#required' => 0, );
	$form[$group]['c_pincode'] = array('#title' => 'PinCode', '#type' => 'textfield', '#size' => '15', '#weight' => 5, 
		'#default_value' => $add?'':$edit_data['c_pincode'], '#required' => 1, );

	$form[$group]['c_country'] = array('#title' => 'Country', '#type' => 'textfield', '#size' => '40', '#weight' => 6, 
		'#default_value' => $add?'':$edit_data['c_country'], '#required' => 1,
				'#attached' => array(
      				'library' => array(
        				array('system', 'ui.autocomplete'),
      		)),

		 );
	$form[$group]['c_state'] = array('#title' => 'State', '#type' => 'textfield', '#size' => '40', '#weight' => 7, 
		'#default_value' => $add?'':$edit_data['c_state'], '#required' => 1, );
	$form[$group]['c_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '40', '#weight' => 8, 
		'#default_value' => $add?'':$city_name, '#required' => 1, );

	$form[$group]['c_address'] = array('#title' => 'Address', '#type' => 'textarea', '#rows' => '2', '#weight' => 9, 
		'#default_value' => $add?'':$edit_data['c_address'], '#required' => 1, );

	$form[$group]['c_phone'] = array('#title' => 'Phone', '#type' => 'textfield', '#size' => '50', '#weight' => 10, 
		'#default_value' => $add?'':$edit_data['c_phone'], '#required' => 1, );
	$form[$group]['c_fax'] = array('#title' => 'Fax', '#type' => 'textfield', '#size' => '50', '#weight' => 11, 
		'#default_value' => $add?'':$edit_data['c_fax'], '#required' => 0, );

	$form[$group]['c_email'] = array('#title' => 'Email', '#type' => 'textfield', '#size' => '50', '#weight' => 12, 
		'#default_value' => $add?'':$edit_data['c_email'], '#required' => 1, );
	$form[$group]['c_url'] = array('#title' => 'Website', '#type' => 'textfield', '#size' => '50', '#weight' => 13, 
		'#default_value' => $add?'':$edit_data['c_url'], '#required' => 1, );
	
	$form[$group]['cs_email_from_name'] = array('#title' => 'Email From Name', '#type' => 'textfield', '#size' => '50', '#weight' => 14, 
		'#default_value' => $add?'':$edit_data['cs_email_from_name'], '#required' => 1, );
	$form[$group]['cs_email_reply_to'] = array('#title' => 'Email Reply to', '#type' => 'textfield', '#size' => '50', '#weight' => 15, 
		'#default_value' => $add?'':$edit_data['cs_email_reply_to'], '#required' => 1, );


	$group = 'Registration Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_preconf_enable'] = array('#title' => 'Enable AutoCancellation after Preconfirmation', '#type' => 'radios', '#weight' => 1,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'0':$edit_data['cs_preconf_enable'], '#required' => 1,  );
	$form[$group]['cs_preconf_days'] = array('#title' => 'Days after which to send AutoCancellation after Preconfirmation', '#type' => 'textfield', '#weight' => 2,
		'#default_value' => $add?'5':$edit_data['cs_preconf_days'], '#required' => 0, '#size' => 5 );

	$form[$group]['cs_reconf_enable'] = array('#title' => 'Enable AutoCancellation after Reconfirmation', '#type' => 'radios', '#weight' => 3,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'0':$edit_data['cs_preconf_enable'], '#required' => 1,  );
	$form[$group]['cs_reconf_days'] = array('#title' => 'No of days before course start date to send Reconfirmation', '#type' => 'textfield', '#weight' => 4,
		'#default_value' => $add?'3':$edit_data['cs_expected_days'], '#required' => 0, '#size' => 5 );
	$form[$group]['cs_reconf_cancel'] = array('#title' => 'Days after which to send AutoCancellation after Reconfirmation', '#type' => 'textfield', '#weight' => 5,
		'#default_value' => $add?'5':$edit_data['cs_preconf_days'], '#required' => 0, '#size' => 5 );

	$form[$group]['cs_expected_enable'] = array('#title' => 'Enable Reminder Mail (Change Status to "Expected")', '#type' => 'radios', '#weight' => 6,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'0':$edit_data['cs_expected_enable'], '#required' => 1,  );
	$form[$group]['cs_expected_days'] = array('#title' => 'No of days before course start date to send reminder', '#type' => 'textfield', '#weight' => 7,
		'#default_value' => $add?'3':$edit_data['cs_expected_days'], '#required' => 0, '#size' => 5 );


	$group = 'Search Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_pagination'] = array('#title' => 'Show Pagination on', '#type' => 'radios', '#weight' => 1,
		'#options' => array('top' => 'Top', 'bottom' => 'Bottom', 'both' => 'Top and Bottom'), 
		'#default_value' => $add?'bottom':$edit_data['cs_pagination'], '#required' => 0,  );
	
	$group = 'Course Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_course_config'] = array('#title' => 'Course Configuration', '#type' => 'textarea', '#rows' => '10', '#weight' => 3, 
		'#default_value' => $add?"[1-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[2-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[3-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[10-Day]\nMaxApps-Male = \nMaxApps-Female = \n[10-DayExecutive]\nMaxApps-Male = \nMaxApps-Female = \n[10-DaySpecial]\nMaxApps-Male = \nMaxApps-Female = \n[Satipatthana]\nMaxApps-Male = \nMaxApps-Female = \n[20-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[30-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[45-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[60-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n\n":$edit_data['cs_course_config'], '#required' => 1,
		'#description' => 'Keywords: MaxApps-Male, MaxApps-Female, Waitlist-Male, Waitlist-Female. ' );

	$form[$group]['cs_apps_waitlist'] = array('#title' => 'Move Applications to Waitlist if course status is WaitList?', '#weight' => 4,
		'#default_value' => $add?0:$edit_data['cs_apps_waitlist'], '#type' => 'radios', '#options' => array('1' => 'Yes', '0' => 'No') );

	$group = 'Hall Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_hall_combined'] = array('#title' => 'Male/Female Students in same Hall', '#type' => 'radios', '#weight' => 1,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'':$edit_data['cs_hall_combined'], '#required' => 1,  );

	/*$seat_direction = array(LEFT_TO_RIGHT => 'Left To Right', RIGHT_TO_LEFT => 'Right to Left');
	$form[$group]['cs_seating_dir_male'] = array('#title' => 'Male Seating Direction', '#type' => 'select', '#weight' => 2,
		'#options' => $seat_direction, 
		'#default_value' => $add?'1':$edit_data['cs_seating_dir_male'], '#required' => 1,  );
	$form[$group]['cs_seating_dir_female'] = array('#title' => 'Female Seating Direction', '#type' => 'select', '#weight' => 3,
		'#options' => $seat_direction, 
		'#default_value' => $add?'1':$edit_data['cs_seating_dir_female'], '#required' => 1,  );

	$form[$group]['cs_seat_per_row_male'] = array('#title' => 'No of seats per row (Male)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 4,
		'#default_value' => $add?'1':$edit_data['cs_seat_per_row_male'], '#required' => 1,  );
	$form[$group]['cs_seat_per_row_female'] = array('#title' => 'No of seats per row (Female)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 5,
		'#default_value' => $add?'1':$edit_data['cs_seat_per_row_female'], '#required' => 1,  );

	$form[$group]['cs_chowky_per_row_male'] = array('#title' => 'No of chowky per row (Male)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 6,
		'#default_value' => $add?'1':$edit_data['cs_chowky_per_row_male'], '#required' => 1,  );
	$form[$group]['cs_chowky_per_row_female'] = array('#title' => 'No of chowky per row (Female)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 7,
		'#default_value' => $add?'1':$edit_data['cs_chowky_per_row_female'], '#required' => 1,  );

	*/
	$form[$group]['cs_seat_config'] = array('#title' => 'Seating Configuration', '#type' => 'textarea', '#weight' => 8,
		'#default_value' => $add?"#Female\n[LEFT]\nSeatsPerRow = 4\nSeatsPerRowChowky = 1\n\n#Male\n[RIGHT]\nSeatsPerRow = 5\nSeatsPerRowChowky = 1\n":$edit_data['cs_seat_config'], '#required' => 1, '#rows'  => 10 );


	$group = 'Cell Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_cells_config'] = array('#title' => 'Cells Configuration', '#type' => 'textarea', '#rows' => '10', '#weight' => 3, 
		'#default_value' => $add?"[MALE]\nCells = \nReserved = \n\n[FEMALE]\nCells = \nReserved = \n":$edit_data['cs_cells_config'], '#required' => 1,
		'#description' => 'Keywords: Reserverd, Cells, Old, New. If "Cells" is defined then cells will fill up available space, If "Old" or "New" is defined then cells will be blocked as per allocation' );


	if ($add)
		$action = 'Add Centre';
	else
		$action = 'Update';
	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 100);


	$centre_id = arg(1);

	if ( !$add )
	{
		$js = '
			var editor;
			(function ($) {
				function load_editor()
				{
					editor = new $.fn.dataTable.Editor({ 
					    ajax: {url: "/centre/'.arg(1).'/acco-handler", type: "POST"},
						table: "#centre-acco-table",
					    fields: [
					        { name: "dh_center_setting_acco.csa_gender", label: "Gender", type: "select" },
					        { name: "dh_center_setting_acco.csa_section", label:  "Section", type: "text" }, 
					        { name: "dh_center_setting_acco.csa_room", label:  "Rooms", type: "textarea" },
					        { name: "dh_center_setting_acco.csa_deleted", type: "hidden" },

					    ]
					});


					var table = $("#centre-acco-table").DataTable( {
					    ajax: "/centre/'.arg(1).'/acco-handler",
					    dom: "Bfrtip",
						columns: [
					        { data: null, render: function(data, type, row) {
					        	//alert(row.csa_gender);
					        	if (data.dh_center_setting_acco.csa_gender == "F")
					        		return "Female";
					        	else
					        		return "Male";	
					        } },
							{ data: "dh_center_setting_acco.csa_section" },
							{ data: "dh_center_setting_acco.csa_room" },
						],
					    select: true,
					    buttons: [
					        { extend: "create", editor: editor },
					        { extend: "edit",   editor: editor },
				            {
				                extend: "selected",
				                text: "Delete",
				                action: function ( e, dt, node, config ) {
				                    var rows = table.rows( {selected: true} ).indexes();
				 
				                    editor
				                        .hide( editor.fields() )
				                        .one( "close", function () {
				                            setTimeout( function () { // Wait for animation
				                                editor.show( editor.fields() );
				                            }, 500 );
				                        } )
				                        .edit( rows, {
				                            title: "Delete",
				                            message: rows.length === 1 ?
				                                "Are you sure you wish to delete this row?" :
				                                "Are you sure you wish to delete these "+rows.length+" rows",
				                            buttons: "Delete"
				                        } )
				                        .val( "dh_center_setting_acco.csa_deleted", 1 );
				                }
				            }
					    ]
					});

				}



				$(document).ready(function(){
					load_editor();
					autocomplete_country("#edit-c-country");
					autocomplete_state("#edit-c-state", "#edit-c-country");
					autocomplete_city("#edit-c-city", "#edit-c-country", "#edit-c-state");
					$("#edit-c-pincode").focusout(function() {
						do_pincode( $(this).val(), "#edit-c-country", "#edit-c-state", "#edit-c-city" );
					});
				});
			})(jQuery);
		';
		drupal_add_js($js, 'inline');

		$out = '<h2>Accomodation Settings</h2>
				<table id="centre-acco-table">
					<thead>
						<tr>
							<th>Gender</th>
							<th>Section</th>
							<th>Rooms</th>
						</tr>
					</thead>

		</table>';
		$form['a'] = array('#markup' => $out, '#weight' => 1000);

	}
	else
	{
		$js = '
			(function ($) {
				$(document).ready(function(){
					autocomplete_country("#edit-c-country");
					autocomplete_state("#edit-c-state", "#edit-c-country");
					autocomplete_city("#edit-c-city", "#edit-c-country", "#edit-c-state");
					$("#edit-c-pincode").focusout(function() {
						do_pincode( $(this).val(), "#edit-c-country", "#edit-c-state", "#edit-c-city" );
					});
				});
			})(jQuery);
		';
		drupal_add_js($js, 'inline');

	}

	return $form;
}

function dh_ma_centre_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];
	$centre_id = '';
	if (arg(1) <> 'add')
		$centre_id = arg(1);

	$centre = array(); $centre_setting = array();
	foreach ($form_state['input'] as $key => $value) 
	{
		if ( substr($key,0,2) == 'c_')
			$centre[$key] = $value;
		if ( substr($key,0,3) == 'cs_')
			$centre_setting[$key] = $value;
	}
	if ($centre['c_dana'] == '')
		$centre['c_dana'] = 0;

	if ($centre['c_city'] <> '')
	{	
		$q = "select c_id from dh_city where c_country='".$centre['c_country']."' and c_state='".$centre['c_state']."' and c_name='".$centre['c_city']."' limit 1";
		$city_id = db_query($q)->fetchField();
		//watchdog('lala - $city_id', $q);
		if ( $city_id <> '')
			$centre['c_city'] = $city_id;
	}
	if ( $centre_id <> '')
	{
		$centre['c_updated_by'] = $user->uid;
		$centre['c_updated'] = date('Y-m-d H:i:s');
		db_update('dh_center')->fields($centre)->condition('c_id', $centre_id )->execute();
		db_update('dh_center_setting')->fields($centre_setting)->condition('cs_center', $centre_id )->execute();				
	}
	else
	{
		$centre['c_created_by'] = $user->uid;
		$centre['c_created'] = date('Y-m-d H:i:s');
		$centre['c_updated_by'] = $user->uid;
		$centre['c_updated'] = date('Y-m-d H:i:s');
		$centre_id = db_insert('dh_center')->fields($centre)->execute();
		$centre_setting['cs_center'] = $centre_id;
		db_insert('dh_center_setting')->fields($centre_setting)->execute();		
		copy_letters_to_centre($centre_id);		
	}
	drupal_goto("centre/$centre_id");
}


function dh_acco_handler()
{
	global $user;
	$centre_id = arg(1);
	include_once "sites/all/libraries/editor/php/DataTables.php";

	$editor = \DataTables\Editor::inst( $db, 'dh_center_setting_acco', 'csa_id' ); 
	 $editor->fields( \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_gender' )
	 		->options( function() { 
				$gender = array();
				$gender[] = array("value" => "M", "label" => "Male");
				$gender[] = array("value" => "F", "label" => "Female");
	 			return $gender; } )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Gender is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_section' )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Section is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_room' )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Room is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_deleted' ),	 
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_center' )
	    	->setValue( $centre_id )
	    )
    ->pkey('csa_id')
    ->on( 'preRemove', function() { return false; })
    ->where( 'csa_deleted', 0)
    ->where( 'csa_center', $centre_id)
    ->process( $_POST )
    ->json();
    exit;

}

function upcoming_courses( $centre_id )
{
	$now_minus_1month = date('Y-m-d',strtotime("-1 month"));
	$now = date('Y-m-d');
	$now_plus_1month = date('Y-m-d', strtotime("+1 month"));

	$q = "select td_key,td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statuses = db_query($q)->fetchAllKeyed();
	unset($system_statuses['STATUS-LEFT']);
	unset($system_statuses['STATUS-CUSTOM']);
	unset($system_statuses['STATUS-ATTENDED']);

	$statuses = $system_statuses;
	//$statuses = array($system_statuses['STATUS-RECEIVED'] , $system_statuses['STATUS-CLARIFICATIONRESPONSE'], $system_statuses['STATUS-CONFIRMED'], $system_statuses['STATUS-CANCELLED']);
	$q = "select td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$s_res = db_query($q);
	while ($r = $s_res->fetchAssoc()) $statuses[] = $r['td_key'];

	//$where = " where a_center=$centre_id and  c.c_start >= '".$now."' and c.c_start <= '".$now_plus_1month."'";
	$c_hand = db_query("select c_id from dh_course where c_center=$centre_id  and c_start >= '$now' and c_deleted=0 order by c_start limit 4");
	$courses = array();
	while($r = $c_hand->fetchAssoc())
		$courses[$r['c_id']] = $r['c_id'];
	if (count($courses) > 0)
		$where = " where a_center=$centre_id and  c.c_id in (".implode(",", array_keys($courses) ).")";
	else
		$where = " where a_center=$centre_id and c.c_id = -1";
	$out = '<h2>Upcoming Courses</h2>';
	$out .= course_summary($centre_id, $where, $statuses);

	return $out;	
}




function received_applications( $centre_id )
{
	//$now_minus_1month = date('Y-m-d',strtotime("-1 month"));
	$now = date('Y-m-d');
	$SYS_R = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key in ('STATUS-RECEIVED', 'STATUS-CLARIFICATIONRESPONSE') ");
	$SYSTEM_RECEIVED = $SYS_R->fetchAssoc()['td_val1'];
	$SYSTEM_CLARIFICATIONRESPONSE = $SYS_R->fetchAssoc()['td_val1'];
	$out = '<div style="clear: left; display: block;"><br /><h2>Received/Response Applications</h2>';
	$q = "select c.c_id,c.c_name,a_gender,a_old, count(a_id) as 'tot' from dh_applicant a left join dh_course c on (a_course=c.c_id) where a_center=$centre_id and  c.c_start >= '".$now."' and a_status IN ('".$SYSTEM_RECEIVED."', '".$SYSTEM_CLARIFICATIONRESPONSE."') group by c.c_id,c.c_name, a_gender, a_old";
	$received = db_query($q);
	$received_courses = array();
	while( $r = $received->fetchAssoc())
	{
		$received_courses[$r['c_name'].'||'.$r['c_id']][$r['a_gender']][$r['a_old']] = $r['tot'];
	}
	$header = array('Course','New Male','Old Male', 'Total Male', '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;', 'New Female','Old Female', 'Total Female');
	$gender = array('M', 'F');
	$rows = array();
	foreach( $received_courses as $course => $gender_data )
	{
		$temp = explode("||", $course);
		$course_id = $temp[1];
		//$out .= 
		$r = array();
		$r[] = l($temp[0], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '','g' => '')));
		foreach( $gender as $v )
		{
			$r[] = isset($gender_data[$v][0])?l($gender_data[$v][0],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '0','g' => $v))):"";
			$r[] = isset($gender_data[$v][1])?l($gender_data[$v][1],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '1','g' => $v))):"";
			$new = 0; $old = 0;
			if (isset($gender_data[$v][0]))
				$new = $gender_data[$v][0];
			if (isset($gender_data[$v][1]))
				$old = $gender_data[$v][1];
			if ( ($new + $old) )
				$r[] = "<b>".l($new + $old, "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '','g' => $v)))."</b>";
			else
				$r[] = "";
			$r[] = ' ';
		}
		$rows[] = $r;
	}
	$out .= theme('table', array('header' => $header, 'rows' => $rows));
	$out .= '</div>';
	return $out;
}

function dashboard( $centre_id )
{

	/*$mg = Mailgun::create('key-1dec8567ba03bd362ea0d719f554c1cb');

	$mg->messages()->send('nl.vipassana.co', [
	  'from'    => 'vinay@neuralit.com',
	  'to'      => 'vinay@vinay.in',
	  'subject' => 'My First test from MG',
	  'text'    => 'It is so simple to send a message.'
	]);
	*/
	$now_minus_1month = date('Y-m-d',strtotime("-1 month"));
	$now_plus_1month = date('Y-m-d', strtotime("+1 month"));

	$out = '';
	if (user_access('received applications block'))
		$out .= received_applications( $centre_id );

	if (user_access('upcoming courses block'))
		$out .= upcoming_courses($centre_id);
	return $out; 
}


function copy_letters_to_centre( $centre_id )
{
	global $user;
	$q = "insert into dh_letter (l_center, l_event, l_course_type, l_name, l_subject, l_body, l_sms, l_created_by, l_updated_by, l_updated) select '$centre_id', l_event, l_course_type, l_name, l_subject, l_body, l_sms, '".$user->uid."', '".$user->uid."', '".date("Y-m-d H:i:s")."' from dh_letter where l_center = 0";
	db_query($q);

	$q = "insert into dh_letter_fields (lf_center, lf_name, lf_value ) select '$centre_id', lf_name, lf_value from dh_letter_fields where lf_center = 0";
	db_query($q);


}


function cron_auto_cancel()
{
	$status_cancelled = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-CANCELLED'")->fetchField();
	$q = "select cs_center, cs_reconf_cancel, cs_preconf_days from dh_center_setting where (cs_preconf_enable=1 and cs_preconf_days > 0) or (cs_reconf_enable=1 and cs_reconf_cancel > 0)";
	$UID_SYSTEM = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();

	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$send_date = date('Y-m-d', strtotime("-".$r['cs_preconf_days']." day"));
		$send_rc_date = date('Y-m-d', strtotime("-".$r['cs_reconf_cancel']." day"));
		$q = "select a_id from dh_applicant left join dh_course on (a_course=c_id) where a_center=".$r['cs_center']." and (a_status='PreConfirmation' and date_format(c_start, '%Y%m%d') > '".date("Ymd")."' and a_pc_date <= '$send_date') or (a_status = 'ReConfirmation' and date_format(c_start, '%Y%m%d') > '".date("Ymd")."' and a_rc_date <= '$send_rc_date')";
		$res = db_query($q);
		while($row = $res->fetchAssoc())
		{
			_update_status($row['a_id'], $status_cancelled, $UID_SYSTEM);
			dh_send_letter('applicant', $row['a_id'], $status_cancelled );
		}
	}
}


function cron_rc_expected($centre_id=0, $course_id=0, $expected_or_rc = 'expected')
{
	$status_expected = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-EXPECTED'")->fetchField();
	$status_reconf = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-RECONFIRMATION'")->fetchField();
	$UID_SYSTEM = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();
	$q = "select cs_center, cs_reconf_days, cs_expected_days from dh_center_setting where (cs_expected_enable=1 and cs_expected_days > 0) or (cs_reconf_enable=1 and cs_reconf_days > 0) ";
	if ($centre_id > 0 )
		$q .=  " and cs_center = '$centre_id'";
	$result = db_query($q);
	$new_status = ($expected_or_rc=='expected')?$status_expected:$status_reconf;
	while($r = $result->fetchAssoc())
	{
		$end_date = date('Y-m-d', strtotime("+".$r[ ($expected_or_rc=='expected')?'cs_expected_days':'cs_reconf_days']." day"));
		//$end_rc_date = date('Y-m-d', strtotime("+".$r['cs_reconf_days']." day"));

		$q = "select c_id from dh_course where c_center='".$r['cs_center']."' and c_deleted=0";
		if ($course_id > 0 )
			$q .=  " and c_id = '$course_id'";
		else
			$q .= " and c_start='$end_date'";
		$res_c = db_query($q);
		while($r_c = $res_c->fetchAssoc())
		{
			logit($r['cs_center'], 'CRON-RC-EXPECTED', $r_c['c_id'], 'Running cron for course start date '.$end_date);
			$q = "select a_id from dh_applicant where a_course=".$r_c['c_id']." and a_status = 'Confirmed' )";
			$res = db_query($q);
			while($row = $res->fetchAssoc())
			{

				_update_status($row['a_id'], $new_status, $UID_SYSTEM);
				dh_send_letter('applicant', $row['a_id'], $new_status );
			}			
		}
	}	
}



