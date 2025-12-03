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
		$edit_at = function($d, $row) { return $d." (".l("Edit", "/at-portal/address-book/edit/".strtolower($row['t_id'])).')'; };
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
	    array( 'db' => 't_id',     'dt' => 13 ),
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
	 
	// drupal_json_output( SSP::simple( $_GET, $sql_details, $table, $primaryKey, $columns ) );
    if (user_access('add assistant teacher'))
        drupal_json_output( SSP::simple( $_GET, $sql_details, $table, $primaryKey, $columns ) );
    else
        drupal_json_output( SSP::complex( $_GET, $sql_details, $table, $primaryKey, $columns, null, " t_status <> 'Training' " ) );

	exit;
}


function dh_atportal_address_book_form($form, &$form_state)
{
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
    drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
    drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
    drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );

    drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
    drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

	if (user_access('add assistant teacher'))
		$form['at-portal'] = array('#markup' => '<h2>'.l("Add Assistant Teacher", "at-portal/address-book/add")." | ".l("Back to AT-Portal", "at-portal").'</h2>');
    else
        $form['at-portal'] = array('#markup' => '<h2>'.l("Back to AT-Portal", "at-portal").'</h2>');

	// $header = array('Code', 'Gender', 'FName', 'LName', 'Status', 'Email', 'Country', 'Mobile', 'SAT', 'CT', 'FullT', 'CAT', 'Bhante', 'ID');

    $country_codes = db_query("select c_code, CONCAT(c_name,' ','(',c_code,')') from dh_country order by c_name")->fetchAllKeyed();

    $state_codes = db_query("select s_id, CONCAT(s_name,' ','(',c_name,')') FROM dh_state left join dh_country on s_country=c_code")->fetchAllKeyed();

    $form['t_code'] = array('#title' => 'AT-Code', '#type' => 'textfield', '#size' => '30', '#weight' => 1,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );

    $form['t_f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#size' => '30', '#weight' => 1,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );


    $form['t_l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#size' => '30', '#weight' => 4,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );

    $form['t_country'] = array(
        '#title' => 'Country',
        '#type' => 'select',
        '#options' => $country_codes,
        '#weight' => 6,
        '#multiple' => "multiple",
        '#attributes' => array(
            'id' => 'edit-t-country',
        ),
    );

    $form['s_id'] = array(
        '#title' => 'State',
        '#type' => 'select',
        '#options' => $state_codes,
        '#weight' => 8,
        '#multiple' => "multiple",
        '#attributes' => array(
            'id' => 'edit-t-state',
        ),
    );

    $form['t_sat'] = array(
        '#title' => '&nbsp;Senior Assistant Teacher (SAT)',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 10
    );

    $form['t_full_t'] = array(
        '#title' => '&nbsp;Full Teacher (T)',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 12
    );

    $form['t_bhante'] = array(
        '#title' => '&nbsp;Bhante Teacher (BT)',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 14
    );

    $form['t_cat'] = array(
        '#title' => '&nbsp;Coordinating Area Teacher (CAT)',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 16
    );

    $form['t_ct'] = array(
        '#title' => '&nbsp;Center Teacher (CT)',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18
    );

    $form['sub'] = array(
        '#type' => 'submit',
        '#value' => 'Search',
        '#weight' => 25,
        '#prefix' => '<br>',
        '#suffix' => '<br><br><br>',
    );

$search_filters = "";

if (isset($form_state['user_input']) && $form_state['user_input'])
{
    $values = $form_state['user_input'];

    if($values['t_code'])
        $search_filters .= " and t_code like '%{$values["t_code"]}%' ";

    if($values['t_f_name'])
        $search_filters .= " and t_f_name like '%{$values["t_f_name"]}%' ";

    if($values['t_l_name'])
        $search_filters .= " and t_l_name like '%{$values["t_l_name"]}%' ";

    if( is_array($values['t_country']) && count($values['t_country'])>0 )
        $search_filters .= " and t_country in ('".implode("','", $values['t_country'])."') ";

    if( is_array($values['s_id']) && count($values['s_id'])>0 )
        $search_filters .= " and s_id in ('".implode("','", $values['s_id'])."') ";

    if ( isset($values['t_sat']) && ($values['t_sat']) )
            $search_filters .= " and t_sat = 1 ";

    if ( isset($values['t_full_t']) && ($values['t_full_t']) )
            $search_filters .= " and t_full_t = 1 ";

    if ( isset($values['t_bhante']) && ($values['t_bhante']) )
            $search_filters .= " and t_bhante = 1 ";

    if ( isset($values['t_ct']) && ($values['t_ct']) )
            $search_filters .= " and t_ct = 1 ";

    if ( isset($values['t_cat']) && ($values['t_cat']) )
            $search_filters .= " and t_cat = 1 ";

}

    if (!user_access('add assistant teacher') || $search_filters)
    {
        $where = " where 1 ";


        if(!user_access('add assistant teacher'))
        {
            $where .= " and t_status <> 'Training' ";
        }

        if($search_filters)
        {
            $where .= $search_filters;
        }
    }

    $q = "select
        t_id,
        t_code,
        t_gender,
        t_f_name,
        t_l_name,
        t_city,
        s_name,
        t_country,
        t_mob_country,
        t_mob_phone,
        t_mob_country_alt,
        t_mob_phone_alt,
        t_email,
        t_email_alt,
        TIMESTAMPDIFF( YEAR, t_dob, CURDATE() ) as 'age',
        t_responsibility,
        t_area,
        t_year_appointed,
        t_at_appointed,
        t_sat_appointed,
        t_full_t_appointed,
        t_bt_appointed,
        t_status,
        t_sat,
        t_ct,
        t_full_t,
        t_cat,
        t_bhante
        from dh_teacher
        left join dh_state on (t_state = s_code and t_country = s_country)
        $where";



    $result = db_query($q);
    $data = array();

    while( $r = $result->fetchAssoc() )
    {
        $r["edit_at"] = $r['t_code']."<br>(".l("Edit", "/at-portal/address-book/edit/".strtolower($r['t_id'])).')';
        $r["t_sat"] = $r["t_sat"]?'Yes':'';
        $r["t_ct"] = $r["t_ct"]?'Yes':'';
        $r["t_full_t"] = $r["t_full_t"]?'Yes':'';
        $r["t_cat"] = $r["t_cat"]?'Yes':'';
        $r["t_bhante"] = $r["t_bhante"]?'Yes':'';

        $r["t_country"] = $country_codes[$r["t_country"]];
        $data[] = $r;
    }
    $data = json_encode($data);
	//$out = theme('table', array('header' => $header, 'rows' => $rows_pending, 'attributes' => $attributes));
	// $out .= theme('table', array('header' => $header, 'rows' => array(array('1','2','3','4','5','1','2','3')), 'attributes' => $attributes));
    $header = array();
    $attributes = array('id' => 'at-address-book');
  $out = theme('table', array('header' => $header, 'rows' => array(array()) ,'attributes' => $attributes));

  $form['a'] = array('#markup' => $out, '#weight' => 101,);

	$js = '
            function format ( d ) {

            var t_at_appointed = "";
            var t_sat_appointed = "";
            var t_full_t_appointed = "";
            var t_bt_appointed = "";
            var t_responsibility = "";
            var t_area = "";

            if (d.t_responsibility)
            {
                t_responsibility =   "<tr>"+
                        "<td><label>Other Responsibility:</label></td>"+
                        "<td colspan=\"3\">"+d.t_responsibility+"</td>"+
                     "</tr>";
            }

            if (d.t_cat)
            {
                t_area =   "<tr>"+
                        "<td><label>Area of Responsibility:</label></td>"+
                        "<td colspan=\"3\">"+d.t_area+"</td>"+
                     "</tr>";
            }

            if (d.t_at_appointed)
            {
                t_at_appointed =   "<tr>"+
                        "<td><label>AT Appointment Year:</label></td>"+
                        "<td colspan=\"3\">"+d.t_at_appointed+"</td>"+
                     "</tr>";
            }

            if (d.t_sat_appointed)
            {
                t_sat_appointed =   "<tr>"+
                        "<td><label>SAT Appointment Year:</label></td>"+
                        "<td colspan=\"3\">"+d.t_sat_appointed+"</td>"+
                    "</tr>";
            }

            if (d.t_full_t_appointed)
            {
                t_full_t_appointed =   "<tr>"+
                        "<td><label>Full-T Appointment Year:</label></td>"+
                        "<td colspan=\"3\">"+d.t_full_t_appointed+"</td>"+
                    "</tr>";
            }

            if (d.t_bt_appointed)
            {
                t_bt_appointed =   "<tr>"+
                        "<td><label>Bhante Teacher Appointment Year:</label></td>"+
                        "<td colspan=\"3\">"+d.t_bt_appointed+"</td>"+
                    "</tr>";
            }

            return "<div class=\"slider\">"+
                "<table cellpadding=\"4\" cellspacing=\"0\" border=\"0\" style=\"padding-left:50px;\">"+
                    t_at_appointed+
                    t_sat_appointed+
                    t_full_t_appointed+
                    t_bt_appointed+
                    "<tr>"+
                        "<td><label>Age:</label></td>"+
                        "<td colspan=\"3\">"+d.age+" years</td>"+
                    "</tr>"+
                    "<tr>"+
                        "<td><label>City:</label></td>"+
                        "<td colspan=\"3\">"+d.t_city+"</td>"+
                    "</tr>"+
                    "<tr>"+
                        "<td><label>State:</label></td>"+
                        "<td colspan=\"3\">"+d.s_name+"</td>"+
                    "</tr>"+
                    "<tr>"+
                        "<td><label>Mobile-1:</label></td>"+
                        "<td colspan=\"3\">\+"+d.t_mob_country+" "+d.t_mob_phone+"</td>"+
                    "</tr>"+
                    "<tr>"+
                        "<td><label>Mobile-2:</label></td>"+
                        "<td colspan=\"3\">\+"+d.t_mob_country_alt+" "+d.t_mob_phone_alt+"</td>"+
                    "</tr>"+
                    "<tr>"+
                        "<td><label>Email-1:</label></td>"+
                        "<td colspan=\"3\">"+d.t_email+"</td>"+
                    "</tr>"+
                    "<tr>"+
                        "<td><label>Email-2:</label></td>"+
                        "<td colspan=\"3\">"+d.t_email_alt+"</td>"+
                    "</tr>"+
                    t_responsibility+
                    t_area+
                "</table>"+
            "</div>";
        }

		(function ($) {
			function do_datatable( tid, dataset )
			{
				var table = $(tid).DataTable({
                    "bDestroy": true,
					data: dataset,
                    columns: [
                        {
                            "class":          "details-control",
                            "orderable":      false,
                            "data":           null,
                            "title":          "Detail",
                            "defaultContent": ""
                        },
                        { data: "edit_at", "title" : "Code" },
                        { data: "t_gender", "title" : "Gender" },
                        { data: "t_f_name", "title" : "FName"},
                        { data: "t_l_name", "title" : "LName" },
                        { data: "t_status", "title" : "Status" },
                        // { data: "t_email", "title" : "Email" },
                        { data: "t_country", "title" : "Country" },
                        // { data: "t_mob_phone", "title" : "Mobile" },
                        { data: "t_sat", "title" : "SAT" },
                        { data: "t_ct", "title" : "CT" },
                        { data: "t_full_t", "title" : "FullT" },
                        { data: "t_cat", "title" : "CAT" },
                        { data: "t_bhante", "title" : "Bhante" },
                    ],
                });

                // Add event listener for opening and closing details
                $("#at-address-book tbody").on("click", "td.details-control", function () {
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


			}
			$(document).ready(function(){
                $("#edit-t-country").select2();
                $("#edit-t-state").select2();

                var dataset = '.$data.';
				do_datatable("#at-address-book", dataset);

			});
		})(jQuery);

	';
	drupal_add_js($js, 'inline');

	return $form;
}

function dh_atportal_address_book_form_submit($form, &$form_state)
{
    $form_state['user_input'] = $form_state['input'];
    $form_state['rebuild'] = TRUE;
}


function dh_atportal_profile($form, &$form_state, $edit_data = array())
{
	global $user;
	$add = 0;
	if (count($edit_data) == 0)
		$add = 1;

    if(!$add)
        $at_id = arg(3);

    $at_code_year = "";

    if($edit_data['t_status'] == 'Active' && $edit_data['t_bhante'])
      $at_code_year = "BT-{$edit_data['t_year_appointed']}";
    elseif($edit_data['t_status'] == 'Active' && $edit_data['t_full_t'])
      $at_code_year = "FullT-{$edit_data['t_year_appointed']}";
    elseif($edit_data['t_status'] == 'Active' && $edit_data['t_sat'])
      $at_code_year = "SAT-{$edit_data['t_year_appointed']}";
    elseif($edit_data['t_status'] == 'Active')
      $at_code_year = "AT-{$edit_data['t_year_appointed']}";

	$form['a'] = array('#markup' => '<h2>'.l("Back to AT Address Book", "at-portal/address-book").'<br><br>Edit AT Profile</h2>');


	$group = 'Personal Information';
	$form['#attributes'] = array('class' => array('container-inline'));
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false);
  if (user_access('add assistant teacher'))
  {
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
  }
  else
  {

    $form[$group]['at_info'] = array(
      '#markup' => "
        <p><strong>Name: </strong>{$edit_data['t_f_name']} {$edit_data['t_l_name']} ({$at_code_year})</p>
        <p><strong>Gender: </strong>{$edit_data['t_gender']}</p>
        <p><strong>AT-Code: </strong>{$edit_data['t_code']}</p>
        ",
      '#weight' => 1,
    );
  }


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

    $country_codes = db_query("select c_isd_code, CONCAT(c_name,' ','(+',c_isd_code,')') from dh_country order by c_name")->fetchAllKeyed();
    $country_codes['0'] = 'Choose';

    $form[$group]['t_mob_country'] = array('#title' => 'Country Code 1', '#type' => 'select', '#options' =>  $country_codes,  '#weight' => 6.3,
        '#default_value' => isset($edit_data['t_mob_country'])?$edit_data['t_mob_country']:'0', '#required' => 0, );
	$form[$group]['t_mob_phone'] = array('#title' => 'Mobile 1', '#type' => 'textfield', '#size' => '20', '#weight' => 6.4,
		'#default_value' => isset($edit_data['t_mob_phone'])?$edit_data['t_mob_phone']:'', '#required' => 0, '#maxlength' => 50 );

    $form[$group]['t_mob_country_alt'] = array('#title' => 'Country Code 2', '#type' => 'select', '#options' =>  $country_codes,  '#weight' => 6.5,
        '#default_value' => isset($edit_data['t_mob_country_alt'])?$edit_data['t_mob_country_alt']:'0', '#required' => 0, );
    $form[$group]['t_mob_phone_alt'] = array('#title' => 'Mobile 2', '#type' => 'textfield', '#size' => '20', '#weight' => 6.6,
        '#default_value' => isset($edit_data['t_mob_phone_alt'])?$edit_data['t_mob_phone_alt']:'', '#required' => 0, '#maxlength' => 50 );

	$form[$group]['t_email'] = array('#title' => 'Email 1', '#type' => 'textfield', '#size' => '25', '#weight' => 8,
		'#default_value' => isset($edit_data['t_email'])?$edit_data['t_email']:'', '#required' => 0, '#maxlength' => 50 );
    $form[$group]['t_email_alt'] = array('#title' => 'Email 2', '#type' => 'textfield', '#size' => '25', '#weight' => 8.1,
        '#default_value' => isset($edit_data['t_email_alt'])?$edit_data['t_email_alt']:'', '#required' => 0, '#maxlength' => 50 );

	if (user_access('add assistant teacher'))
	{
		$group = 'AT Role Information';
		$form[$group] = array(
            '#type' => 'fieldset',
            '#title' => 'Role Information',
            '#collapsible' => true,
            '#collapsed' => false,
            '#description' => "<b>In case of direct appointment to SAT, please use AT appointment date same as SAT</b><br><b>In case of direct appointment to Full-T, please use AT and SAT appointment date same as Full-T</b><br><b>Similarly in case of direct appointment from AT to Full-T, please use SAT appointment date same as Full-T</b><br><br>",
            );
		$statuses = array(
            'Active' => 'Active',
            'Inactive' => 'Inactive',
            // 'Deprecated' => 'Deprecated',
            'Training' => 'Training',
            'Deceased' => 'Deceased',
        );

		$default_status = '';
		if ( !empty($edit_data) )
		{
			if ( $edit_data['t_status'] == 'Active' ) $default_status = 'Active';
			if ( $edit_data['t_status'] == 'Inactive' ) $default_status = 'Inactive';
            if ( $edit_data['t_status'] == 'Deceased' ) $default_status = 'Deceased';
			// if ( $edit_data['t_status'] == 'Deprecated' ) $default_status = 'Deprecated';
			if ( $edit_data['t_status'] == 'Training' ) $default_status = 'Training';
            if ( $edit_data['t_status'] != 'Training' ) unset($statuses['Training']);
		};	

        if($add)
            unset($statuses['Training']);

		$form[$group]['t_status'] = array('#title' => 'Status', '#type' => 'select', '#options' =>  $statuses,
			'#default_value' => $default_status, '#weight' => 9, '#empty_option' => 'Choose', '#required' => 1);
		$form[$group]['t_code'] = array('#title' => 'AT Code', '#type' => 'textfield', '#size' => '10', '#weight' => 10, 
			'#required' => 1,
			'#default_value' => isset($edit_data['t_code'])?$edit_data['t_code']:'', '#maxlength' => 10  );
		$form[$group]['t_year_appointed'] = array(
            '#title' => 'Appointment Status',
            '#type' => 'textfield',
            '#size' => '10',
            '#weight' => 10.1,
			'#default_value' => $at_code_year,
            '#maxlength' => 10,
            '#disabled' => true,
        );

        $form[$group]['t_at_appointed'] = array(
            '#title' => 'Date of AT Appointment',
            '#type' => 'date_popup',
            '#size' => '20',
            '#weight' => 10.2,
            '#default_value' => isset($edit_data['t_at_appointed'])?$edit_data['t_at_appointed']:'',
            '#date_format' => 'Y-m-d',
            '#datepicker_options' => array(
                'maxDate' => 0,
                '#required' => 1,
                'dateFormat' => date_popup_format_to_popup('Y-m-d'),
                ),
            '#date_year_range' => '-100:1',
            '#date_label_position' => 'above',
            '#theme_wrappers' => array('date_popup'),
            '#suffix' => '',
        );

		$form[$group]['t_sat'] = array('#title' => '&nbsp;SAT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_sat'])?$edit_data['t_sat']:'0',  '#weight' => 13);

        $form[$group]['t_sat_appointed'] = array(
            '#title' => 'Date of SAT Appointment',
            '#type' => 'date_popup',
            '#size' => '20',
            '#weight' => 13.1,
            '#default_value' => isset($edit_data['t_sat_appointed'])?$edit_data['t_sat_appointed']:'',
            '#date_format' => 'Y-m-d',
            '#datepicker_options' => array(
                'maxDate' => 0,
                '#required' => 1,
                'dateFormat' => date_popup_format_to_popup('Y-m-d'),
                ),
            '#date_year_range' => '-100:1',
            '#date_label_position' => 'above',
            '#theme_wrappers' => array('date_popup'),
            '#suffix' => '',
        );

		$form[$group]['t_full_t'] = array('#title' => '&nbsp;T', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_full_t'])?$edit_data['t_full_t']:'0',  '#weight' => 14);

        $form[$group]['t_full_t_appointed'] = array(
            '#title' => 'Date of Full-T Appointment',
            '#type' => 'date_popup',
            '#size' => '20',
            '#weight' => 14.1,
            '#default_value' => isset($edit_data['t_full_t_appointed'])?$edit_data['t_full_t_appointed']:'',
            '#date_format' => 'Y-m-d',
            '#datepicker_options' => array(
                'maxDate' => 0,
                '#required' => 1,
                'dateFormat' => date_popup_format_to_popup('Y-m-d'),
                ),
            '#date_year_range' => '-100:1',
            '#date_label_position' => 'above',
            '#theme_wrappers' => array('date_popup'),
            '#suffix' => '',
        );

        $form[$group]['t_bhante'] = array('#title' => '&nbsp;Bhikkhu/Bhikkhuni Teacher', '#type' => 'checkbox',
          '#default_value' => isset($edit_data['t_bhante'])?$edit_data['t_bhante']:'0',  '#weight' => 15);

        $form[$group]['t_bt_appointed'] = array(
            '#title' => 'Date of BT Appointment',
            '#type' => 'date_popup',
            '#size' => '20',
            '#weight' => 15.1,
            '#default_value' => isset($edit_data['t_bt_appointed'])?$edit_data['t_bt_appointed']:'',
            '#date_format' => 'Y-m-d',
            '#datepicker_options' => array(
                'maxDate' => 0,
                '#required' => 1,
                'dateFormat' => date_popup_format_to_popup('Y-m-d'),
                ),
            '#date_year_range' => '-100:1',
            '#date_label_position' => 'above',
            '#theme_wrappers' => array('date_popup'),
            '#suffix' => '',
        );

		$form[$group]['t_ct'] = array('#title' => '&nbsp;CT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_ct'])?$edit_data['t_ct']:'0',  '#weight' => 16);
	    $form[$group]['t_cat'] = array('#title' => '&nbsp;CAT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_cat'])?$edit_data['t_cat']:'0',  '#weight' => 16.1);

	    $form[$group]['t_responsibility'] = array('#title' => 'Additional Responsibility', '#type' => 'textarea', '#size' => '100', '#weight' => 17, 
			'#default_value' => isset($edit_data['t_responsibility'])?$edit_data['t_responsibility']:'', '#maxlength' => 200  );
	    $form[$group]['t_area'] = array('#title' => 'Area of Responsibility', '#type' => 'textarea', '#size' => '100', '#weight' => 18, 
			'#default_value' => isset($edit_data['t_area'])?$edit_data['t_area']:'', '#maxlength' => 255  );
        $form[$group]['t_lc_area'] = array('#title' => '&nbsp;Allow LC 2nd Step Review', '#type' => 'checkbox',
          '#default_value' => isset($edit_data['t_lc_area'])?$edit_data['t_lc_area']:'0',  '#weight' => 19);
        $form[$group]['t_allow_cat_emails'] = array('#title' => '&nbsp;Receive CAT emails', '#type' => 'checkbox',
          '#default_value' => isset($edit_data['t_allow_cat_emails'])?$edit_data['t_allow_cat_emails']:'0',  '#weight' => 20);
	}	

	if ($add)
		$action = 'Add';
	else
		$action = 'Update';

	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 110);

    if(!$add)
    {
        $q = "select tl_tstamp, tl_msg, name FROM dh_teacher_log left join users on tl_user=uid where tl_event='Teacher Updated' and tl_teacher=".$at_id." order by tl_tstamp desc;";
        $res = db_query($q);
        $rows = array();
        while( $row = $res->fetchAssoc())
        {
            $msg_o ="";
            $msg_n ="";
            $msg_o = (array)(json_decode($row['tl_msg'])->old);
            $msg_n = (array)(json_decode($row['tl_msg'])->new);
            foreach(array_keys($msg_o) as $k)
            {
                $l = array();
                $l[] = $row['tl_tstamp'];
                $l[] = $k;
                $l[] = $msg_o[$k];
                $l[] = $msg_n[$k];
                $l[] = $row['name'];
                $rows[] = $l;
            }

        }
        $header = array('DateTime', 'Field', 'OldValue', 'NewValue', 'User');
        $out = '';
        if (!empty($rows))
            $out .= theme('table', array('header' => $header, 'rows' => $rows)) ;

        $form['teacher_changelog'] = array('#type' => 'fieldset','#title' => '<h2>Change Log - Click to open</h2>' , '#collapsible' => true, '#collapsed' => true, '#weight' => 112,);
        $form['teacher_changelog']['a'] = array('#markup' => $out);
    }


    $js ='
    (function ($) {
        function check_appointments()
        {
            if($("input[name=t_sat]:checked").val() == "1" || $("input[name=t_full_t]:checked").val() == "1")
            {
                $("#edit-t-sat-appointed :input").prop("disabled", false);
            }
            else
            {
                $("#edit-t-sat-appointed :input").prop("disabled", true);
            }

            if($("input[name=t_full_t]:checked").val() == "1")
            {
                $("#edit-t-full-t-appointed :input").prop("disabled", false);
            }
            else
            {
                $("#edit-t-full-t-appointed :input").prop("disabled", true);
            }

            if($("input[name=t_bhante]:checked").val() == "1")
            {
                $("#edit-t-bt-appointed :input").prop("disabled", false);
            }
            else
            {
                $("#edit-t-bt-appointed :input").prop("disabled", true);
            }
        }

      $(document).ready(function(){

        check_appointments();
        autocomplete_state("#edit-t-state", "#edit-t-country");

        $(document).on("change", "input[type=checkbox][name=t_sat]", function(){
            check_appointments();
        });
        $(document).on("change", "input[type=checkbox][name=t_full_t]", function(){
            check_appointments();
        });
        $(document).on("change", "input[type=checkbox][name=t_bhante]", function(){
            check_appointments();
        });
        $("#edit-t-mob-country").select2();
        $("#edit-t-mob-country-alt").select2();

      });
    })(jQuery);
  ';
  drupal_add_js($js, 'inline');

    drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
  drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
  $form['#attached']['js'][] = drupal_get_path('module', 'dh_manageapp').'/js/location-common.js';

	return $form;		
}

function dh_manage_at()
{
	global $user;
    $at_id = 0;
	if (arg(3) == 'me')
	{		
		$at = explode(".", $user->name);
		$at_code = $at[0];
		$gender = $at[1];
		$at_id = db_query("select t_id from dh_teacher where t_code='".$at_code."' and t_gender='".$gender."'")->fetchField();
	}
	else
	{
		$at_id = arg(3);
	}
	switch( arg(2) )
	{
		case 'add':
			return drupal_get_form('dh_atportal_profile');
			break;
		case 'edit':
            $row = db_query("select * from dh_teacher where t_id=$at_id")->fetchAssoc();
			return drupal_get_form('dh_atportal_profile', $row);
			break;
	}
}


function dh_atportal_profile_validate($form, &$form_state)
{
    if ($form_state['input']['t_full_t'])
    {
        if($form_state['input']['t_full_t_appointed']['date'] == '')
            form_set_error('t_full_t_appointed', t('Please enter Date of Full-T Appointment'));
        if($form_state['input']['t_sat'])
            form_set_error('t_sat', t('For Full-T SAT checkbox must be unchecked'));
        if($form_state['input']['t_sat_appointed']['date'] == '')
            form_set_error('t_sat_appointed', t('Please enter Date of SAT Appointment'));
        if($form_state['input']['t_at_appointed']['date'] == '')
            form_set_error('t_at_appointed', t('Please enter Date of AT Appointment'));

        if( !( $form_state['input']['t_full_t_appointed']['date'] >= $form_state['input']['t_sat_appointed']['date'] ) )
            form_set_error('t_full_t_appointed', t('Date of Full-T appointment must be greater then equal to date of SAT appointment'));

        if( !( $form_state['input']['t_sat_appointed']['date'] >= $form_state['input']['t_at_appointed']['date'] ) )
            form_set_error('t_sat_appointed', t('Date of SAT appointment must be greater then equal to date of AT appointment'));
    }
    elseif ($form_state['input']['t_sat'])
    {
        if($form_state['input']['t_sat_appointed']['date'] == '')
            form_set_error('t_sat_appointed', t('Please enter Date of SAT Appointment'));
        if($form_state['input']['t_at_appointed']['date'] == '')
            form_set_error('t_at_appointed', t('Please enter Date of AT Appointment'));

        if( !( $form_state['input']['t_sat_appointed']['date'] >= $form_state['input']['t_at_appointed']['date'] ) )
            form_set_error('t_sat_appointed', t('Date of SAT appointment must be greater then equal to date of AT appointment'));
    }

    if ($form_state['input']['t_bhante'])
    {
        if($form_state['input']['t_bt_appointed']['date'] == '')
            form_set_error('t_bt_appointed', t('Please enter Date of BT Appointment'));
    }
}


function dh_atportal_profile_submit($form, &$form_state)
{
	global $user;	

	$input = $form_state['input'];
	// if (arg(2) == 'edit')
	// 	$form_state['redirect'] = 'at-portal';
	// elseif ( arg(2) == 'add')
	// 	$form_state['redirect'] = 'at-portal/address-book';

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

    if ($t['t_at_appointed']['date'] == '')
        $t['t_at_appointed'] = NULL;

    if ($t['t_sat_appointed']['date'] == '')
        $t['t_sat_appointed'] = NULL;

    if ($t['t_full_t_appointed']['date'] == '')
        $t['t_full_t_appointed'] = NULL;

    if ($t['t_bt_appointed']['date'] == '')
        $t['t_bt_appointed'] = NULL;

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
            if ($t['t_lc_area'] == '')
                $t['t_lc_area'] = '0';
            if ($t['t_allow_cat_emails'] == '')
                $t['t_allow_cat_emails'] = '0';
	}


    if($t['t_bhante'])
      $t['t_year_appointed'] = $t['t_bt_appointed'];
    elseif($t['t_full_t'])
      $t['t_year_appointed'] = $t['t_full_t_appointed'];
    elseif($t['t_sat'])
      $t['t_year_appointed'] = $t['t_sat_appointed'];
    else
      $t['t_year_appointed'] = $t['t_at_appointed'];

    $t['t_year_appointed'] = date("Y", strtotime($t['t_year_appointed']['date']));


	if ( arg(2) == 'edit')
	{
		if (arg(3) == 'me')
		{
			$at = explode(".", $user->name);
			$at_code = $at[0];
			$gender = $at[1];
			$at_id = db_query("select t_id from dh_teacher where t_code='".$at_code."' and t_gender='".$gender."'")->fetchField();
		}
		else
		{
			$at_id = arg(3);
		}
		//drupal_set_message(print_r($t, true));
		$t['t_updated_by'] = $user->uid;
		$t['t_updated'] = date('Y-m-d H:i:s');
		db_update('dh_teacher')->fields($t)->condition('t_id', $at_id )->execute();
	}

	else 
	{
		$t['t_created_by'] = $user->uid;
		$t['t_created'] = date('Y-m-d H:i:s');
		$t['t_updated_by'] = $user->uid;
		$t['t_updated'] = date('Y-m-d H:i:s');
		db_insert('dh_teacher')->fields($t)->execute();
	}

	drupal_set_message(t("Details added/updated successfully"), 'status');

}
