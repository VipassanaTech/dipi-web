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
    drupal_add_css(drupal_get_path('module', 'dh_atportal').'/css/bio.css');   // the photo in an opened row

    drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
    drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

    if (user_access('add assistant teacher') && user_access('access area cluster'))
        $form['at-portal'] = array('#markup' => '<h2>'.l("Add Assistant Teacher", "at-portal/address-book/add")." | ".l("Back to AT-Portal", "at-portal")." | ".l("CT-List", "at-portal/ct-list")." | ".l("CAT-List", "at-portal/cat-list")." | ".l("Area Cluster List", "at-portal/area-cluster-list").'</h2>');
	elseif (user_access('add assistant teacher'))
		$form['at-portal'] = array('#markup' => '<h2>'.l("Add Assistant Teacher", "at-portal/address-book/add")." | ".l("Back to AT-Portal", "at-portal")." | ".l("CT-List", "at-portal/ct-list")." | ".l("CAT-List", "at-portal/cat-list").'</h2>');
    else
        $form['at-portal'] = array('#markup' => '<h2>'.l("Back to AT-Portal", "at-portal")." | ".l("CT-List", "at-portal/ct-list")." | ".l("CAT-List", "at-portal/cat-list").'</h2>');

	// $header = array('Code', 'Gender', 'FName', 'LName', 'Status', 'Email', 'Country', 'Mobile', 'SAT', 'CT', 'FullT', 'CAT', 'Bhante', 'ID');

    $country_codes = db_query("select c_code, CONCAT(c_name,' ','(',c_code,')') from dh_country order by c_name")->fetchAllKeyed();

    $state_codes = db_query("select s_id, CONCAT(s_name,' ','(',c_name,')') FROM dh_state left join dh_country on s_country=c_code")->fetchAllKeyed();

    $ct_info = db_query("select tr_teacher, GROUP_CONCAT(tc_name)  from dh_teacher_role left join dh_teacher_center on tr_identifier=tc_id where tr_deleted=0 and tr_role='CT' group by tr_teacher")->fetchAllKeyed();

    $act_info = db_query("select tr_teacher, GROUP_CONCAT(tc_name)  from dh_teacher_role left join dh_teacher_center on tr_identifier=tc_id where tr_deleted=0 and tr_role='ACT' group by tr_teacher")->fetchAllKeyed();

    $cat_info = db_query("select tr_teacher, GROUP_CONCAT(tac_name)  from dh_teacher_role left join dh_teacher_area_cluster on tr_identifier=tac_id where tr_deleted=0 and tr_role='CAT' group by tr_teacher")->fetchAllKeyed();

    $acat_info = db_query("select tr_teacher, GROUP_CONCAT(tac_name)  from dh_teacher_role left join dh_teacher_area_cluster on tr_identifier=tac_id where tr_deleted=0 and tr_role='ACAT' group by tr_teacher")->fetchAllKeyed();

    $form['t_code'] = array('#title' => 'AT-Code', '#type' => 'textfield', '#size' => '30', '#weight' => 1,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );

    $form['t_f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#size' => '30', '#weight' => 2,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );


    $form['t_l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#size' => '30', '#weight' => 4,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );

    $form['t_email'] = array('#title' => 'Email', '#type' => 'textfield', '#size' => '30', '#weight' => 5,
        '#default_value' => '', '#required' => 0, '#maxlength' => 50  );

    $form['t_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '30', '#weight' => 5.1,
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

    // AT: an Active teacher who is not SAT, Full-T or Bhante Teacher (see the filter below)
    $form['t_at'] = array(
        '#title' => '&nbsp;Assistant Teacher (AT)',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 9
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

    $form['t_status_active'] = array(
        '#title' => '&nbsp;Active',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#prefix' => '<br>',
        '#weight' => 18.1
    );

    $form['t_status_inactive'] = array(
        '#title' => '&nbsp;Inactive',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18.2
    );

    $form['t_status_deceased'] = array(
        '#title' => '&nbsp;Deceased',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18.3
    );

    $form['t_status_steppeddown'] = array(
        '#title' => '&nbsp;SteppedDown',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18.31
    );

    $form['t_status_redacted'] = array(
        '#title' => '&nbsp;Redacted',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18.32
    );

    $form['t_status_training'] = array(
        '#title' => '&nbsp;Training',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18.4
    );

    $form['t_gender_m'] = array(
        '#title' => '&nbsp;Male',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#prefix' => '<br>',
        '#weight' => 18.5
    );

    $form['t_gender_f'] = array(
        '#title' => '&nbsp;Female',
        '#type' => 'checkbox',
        '#default_value' => '0',
        '#weight' => 18.6
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

    foreach ($values as $key => $value) {
        if(is_string($value))
            $values[$key] = addslashes($value);
    }

    if($values['t_code'])
        $search_filters .= " and t_code like '%{$values["t_code"]}%' ";

    if($values['t_f_name'])
        $search_filters .= " and t_f_name like '%{$values["t_f_name"]}%' ";

    if($values['t_l_name'])
        $search_filters .= " and t_l_name like '%{$values["t_l_name"]}%' ";

    if($values['t_email'])
        $search_filters .= " and (t_email like '%{$values["t_email"]}%' or t_email_alt like '%{$values["t_email"]}%') ";

    if($values['t_city'])
        $search_filters .= " and t_city like '%{$values["t_city"]}%' ";

    if( is_array($values['t_country']) && count($values['t_country'])>0 )
        $search_filters .= " and t_country in ('".implode("','", $values['t_country'])."') ";

    if( is_array($values['s_id']) && count($values['s_id'])>0 )
        $search_filters .= " and s_id in ('".implode("','", $values['s_id'])."') ";

    // if ( isset($values['t_sat']) && ($values['t_sat']) )
    //         $search_filters .= " and t_sat = 1 ";

    // if ( isset($values['t_full_t']) && ($values['t_full_t']) )
    //         $search_filters .= " and t_full_t = 1 ";

    // if ( isset($values['t_bhante']) && ($values['t_bhante']) )
    //         $search_filters .= " and t_bhante = 1 ";

    // if ( isset($values['t_ct']) && ($values['t_ct']) )
    //         $search_filters .= " and t_ct = 1 ";

    // if ( isset($values['t_cat']) && ($values['t_cat']) )
    //         $search_filters .= " and t_cat = 1 ";

    $or_desigation = array();

    if (!empty($values['t_at']))
        $or_desigation[] = " (t_status = 'Active' and coalesce(t_sat, 0) = 0 and coalesce(t_full_t, 0) = 0 and coalesce(t_bhante, 0) = 0) ";

    if (!empty($values['t_sat']))
        $or_desigation[] = " t_sat = 1 ";

    if (!empty($values['t_full_t']))
        $or_desigation[] = " t_full_t = 1 ";

    if (!empty($values['t_bhante']))
        $or_desigation[] = " t_bhante = 1 ";

    if (!empty($values['t_ct']))
        $or_desigation[] = " t_ct = 1 ";

    if (!empty($values['t_cat']))
        $or_desigation[] = " t_cat = 1 ";

    if (!empty($or_desigation))
        $search_filters .= " AND (" . implode(" OR ", $or_desigation) . ") ";

    $or_status = array();

    if (!empty($values['t_status_active']))
        $or_status[] = " t_status = 'Active' ";

    if (!empty($values['t_status_inactive']))
        $or_status[] = " t_status = 'Inactive' ";

    if (!empty($values['t_status_deceased']))
        $or_status[] = " t_status = 'Deceased' ";

    if (!empty($values['t_status_steppeddown']))
        $or_status[] = " t_status = 'SteppedDown' ";

    if (!empty($values['t_status_redacted']))
        $or_status[] = " t_status = 'Redacted' ";

    if (!empty($values['t_status_training']))
        $or_status[] = " t_status = 'Training' ";

    if (!empty($or_status))
        $search_filters .= " AND (" . implode(" OR ", $or_status) . ") ";

    $or_gender = array();

    if (!empty($values['t_gender_m']))
        $or_gender[] = " t_gender = 'M' ";

    if (!empty($values['t_gender_f']))
        $or_gender[] = " t_gender = 'F' ";

    if (!empty($or_gender))
        $search_filters .= " AND (" . implode(" OR ", $or_gender) . ") ";


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
        t_bhante,
        (coalesce(t_photo, '') <> '') as has_photo,
        t_updated
        from dh_teacher
        left join dh_state on (t_state = s_code and t_country = s_country)
        $where";

    $result = db_query($q);
    $data = array();

    while( $r = $result->fetchAssoc() )
    {
        $r["edit_at"] = $r['t_code'];
        if(user_access('add assistant teacher'))
            $r["edit_at"] = $r['t_code']."<br>(".l("Edit", "/at-portal/address-book/edit/".strtolower($r['t_id'])).')';
        $r["t_sat"] = $r["t_sat"]?'Yes':'';
        $r["t_ct"] = $r["t_ct"]?'Yes':'';
        $r["t_full_t"] = $r["t_full_t"]?'Yes':'';
        $r["t_cat"] = $r["t_cat"]?'Yes':'';
        $r["t_bhante"] = $r["t_bhante"]?'Yes':'';
        $r["t_country"] = $country_codes[$r["t_country"]];

        $r["t_ct_info"] = $ct_info[$r["t_id"]]?$ct_info[$r["t_id"]]:'';
        $r["t_act_info"] = $act_info[$r["t_id"]]?$act_info[$r["t_id"]]:'';
        $r["t_cat_info"] = $cat_info[$r["t_id"]]?$cat_info[$r["t_id"]]:'';
        $r["t_acat_info"] = $acat_info[$r["t_id"]]?$acat_info[$r["t_id"]]:'';
        // the teacher's photo (bio.inc), shown on the right when the row is opened
        $r["photo_url"] = $r["has_photo"] ? url('at-portal/teacher-photo/'.$r['t_id'], array('query' => array('v' => substr(md5((string) $r['t_updated']), 0, 8)))) : '';
        unset($r["has_photo"], $r["t_updated"]);


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
            // a value for the opened row: empty instead of "null", and safe to put in the page
            function ab_v ( x ) {
                return (x === null || x === undefined) ? "" : String(x).replace(/[&<>"]/g, function (c) {
                    return {"&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;"}[c];
                });
            }
            function ab_phone ( cc, ph ) {
                if (!ph) return "";
                if (String(ph).charAt(0) == "+") return ab_v(ph);   // already the full international number
                return ((cc && cc != "0") ? "+" + ab_v(cc) + " " : "") + ab_v(ph);
            }
            // one label / value line of the opened row (an empty value stays as an empty line)
            function ab_row ( label, value ) {
                return "<tr><td><label>" + label + ":</label></td><td colspan=\"3\">" + value + "</td></tr>";
            }
            function format ( d ) {

            var t_at_appointed = "";
            var t_sat_appointed = "";
            var t_full_t_appointed = "";
            var t_bt_appointed = "";
            var t_responsibility = "";
            var t_area = "";
            var t_ct_info = "";
            var t_act_info = "";
            var t_cat_info = "";
            var t_acat_info = "";

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

            if (d.t_ct_info)
            {
                t_ct_info =   "<tr>"+
                        "<td><label>Center Teacher:</label></td>"+
                        "<td colspan=\"3\">"+d.t_ct_info+"</td>"+
                    "</tr>";
            }

            if (d.t_act_info)
            {
                t_act_info =   "<tr>"+
                        "<td><label>Assistant to Center Teacher:</label></td>"+
                        "<td colspan=\"3\">"+d.t_act_info+"</td>"+
                    "</tr>";
            }

            if (d.t_cat_info)
            {
                t_cat_info =   "<tr>"+
                        "<td><label>Coordinating Area Teacher:</label></td>"+
                        "<td colspan=\"3\">"+d.t_cat_info+"</td>"+
                    "</tr>";
            }

            if (d.t_acat_info)
            {
                t_acat_info =   "<tr>"+
                        "<td><label>Assistant to Coordinating Area Teacher:</label></td>"+
                        "<td colspan=\"3\">"+d.t_acat_info+"</td>"+
                    "</tr>";
            }

            // the photo on the left, the details beside it (option B, 2026-10-15); it is only fetched when a row is opened
            var photo = d.photo_url
                ? "<img class=\"dh-ab-photo\" alt=\"\" src=\"" + d.photo_url + "\">"
                : "<div class=\"dh-ab-nophoto\">No photo</div>";

            return "<div class=\"slider\"><div class=\"dh-ab-detail\">"+
                photo+
                "<table cellpadding=\"4\" cellspacing=\"0\" border=\"0\">"+
                    t_at_appointed+
                    t_sat_appointed+
                    t_full_t_appointed+
                    t_bt_appointed+
                    ab_row("Age", (d.age !== null && d.age !== "") ? ab_v(d.age) + " years" : "")+
                    ab_row("City", ab_v(d.t_city))+
                    ab_row("State", ab_v(d.s_name))+
                    ab_row("Mobile-1", ab_phone(d.t_mob_country, d.t_mob_phone))+
                    ab_row("Mobile-2", ab_phone(d.t_mob_country_alt, d.t_mob_phone_alt))+
                    ab_row("Email-1", ab_v(d.t_email))+
                    ab_row("Email-2", ab_v(d.t_email_alt))+
                    t_ct_info+
                    t_act_info+
                    t_cat_info+
                    t_acat_info+
                    t_responsibility+
                "</table>"+
            "</div></div>";
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
                        { data: "t_full_t", "title" : "FullT" },
                        { data: "t_ct", "title" : "CT" },
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
    }

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
	$form['#attributes'] = array('class' => array('dh-at-profile'));
	$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false, '#weight' => 1);
	$form['Contact'] = array('#type' => 'fieldset', '#title' => 'Contact', '#weight' => 2);
	$form['Address'] = array('#type' => 'fieldset', '#title' => 'Address', '#weight' => 3);
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


	$form[$group]['t_dob'] = dh_teacher_dob_field(isset($edit_data['t_dob']) ? $edit_data['t_dob'] : '', 4);

	$group = 'Address';
	$form[$group]['t_address'] = array('#title' => 'Address', '#type' => 'textarea', '#rows' => 2, '#weight' => 5, 
		'#default_value' => isset($edit_data['t_address'])?$edit_data['t_address']:'', '#required' => 1, '#maxlength' => 200);	
	$form[$group]['t_pincode'] = array('#title' => 'PIN / Zip code', '#type' => 'textfield', '#size' => '10', '#weight' => 6.3, 
		'#default_value' => (isset($edit_data['t_pincode']))?$edit_data['t_pincode']:'', '#maxlength' => 10 );

	$country_codes = db_query("select c_code, CONCAT(c_name,' ','(',c_code,')') from dh_country order by c_name")->fetchAllKeyed();	
	$country_codes['0'] = 'Choose';
	$form[$group]['t_country'] = array('#title' => 'Country', '#type' => 'select', '#options' =>  $country_codes,  '#weight' => 6, 
		'#default_value' => isset($edit_data['t_country'])?$edit_data['t_country']:'0', '#required' => 0, );

	$form[$group]['t_state'] = array('#title' => 'State', '#type' => 'textfield', '#size' => '20', '#weight' => 6.1, 
		'#default_value' => isset($edit_data['t_state'])?$edit_data['t_state']:'', '#maxlength' => 20,
    '#attached' => array(
          'library' => array(
            array('system', 'ui.autocomplete'),
        )),
     );

	$form[$group]['t_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '20', '#weight' => 6.2, 
		'#default_value' => isset($edit_data['t_city'])?$edit_data['t_city']:'', '#maxlength' => 50 );

    // Mobiles: one field each with a country flag and dial code (as on the schedule site's forms).
    // The country code is worked out from the number on save (dh_atportal_phone_parse).
    $group = 'Contact';
    $home = isset($edit_data['t_country']) ? $edit_data['t_country'] : '';
    foreach (array('t_mob_phone' => array('Mobile 1', 't_mob_country', 6.4), 't_mob_phone_alt' => array('Mobile 2', 't_mob_country_alt', 6.6)) as $key => $m)
    {
        $phone = isset($edit_data[$key]) ? $edit_data[$key] : '';
        $region = dh_atportal_phone_region($phone, isset($edit_data[$m[1]]) ? $edit_data[$m[1]] : '', $home);
        $form[$group][$key] = array('#title' => $m[0], '#type' => 'textfield', '#size' => '20', '#weight' => $m[2],
            '#default_value' => $phone, '#required' => 0, '#maxlength' => 30,
            '#attributes' => array('class' => array('dh-intl-tel'), 'inputmode' => 'tel', 'data-region' => strtolower($region),
                'data-region-field' => drupal_html_class('edit-'.$key.'-region')));
        // the country chosen in the widget, so a number typed without +code is read for that country
        $form[$group][$key.'_region'] = array('#type' => 'hidden', '#default_value' => $region,
            '#attributes' => array('id' => drupal_html_class('edit-'.$key.'-region')));
    }
    $itl = libraries_get_path('intl-tel-input');
    drupal_add_css($itl.'/build/css/intlTelInput.min.css');
    drupal_add_js($itl.'/build/js/intlTelInput.min.js');
    drupal_add_js(drupal_get_path('module', 'dh_atportal').'/js/phone.js');
    drupal_add_js(array('dhIntlTel' => array('utils' => base_path().$itl.'/build/js/utils.js')), 'setting');

	$form[$group]['t_email'] = array('#title' => 'Email 1', '#type' => 'textfield', '#size' => '25', '#weight' => 8,
		'#default_value' => isset($edit_data['t_email'])?$edit_data['t_email']:'', '#required' => 0, '#maxlength' => 100 );
    $form[$group]['t_email_alt'] = array('#title' => 'Email 2', '#type' => 'textfield', '#size' => '25', '#weight' => 8.1,
        '#default_value' => isset($edit_data['t_email_alt'])?$edit_data['t_email_alt']:'', '#required' => 0, '#maxlength' => 100 );

	// Bio data from the AT acceptance letter (bio.inc): marital status, place of birth,
	// nationality, photo, first course, family, background and service.
	$bio_id = $add ? 0 : (int) $at_id;
	dh_teacher_bio_form($form, $bio_id, $edit_data, !$add && (arg(3) == 'me' || $bio_id == dh_teacher_own_id()));

	if (user_access('add assistant teacher'))
	{
		$group = 'AT Role Information';
		$form[$group] = array(
            '#type' => 'fieldset',
            '#title' => 'Role Information',
            '#weight' => 40,
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
            'SteppedDown' => 'SteppedDown',
            'Redacted' => 'Redacted',
        );

		$default_status = '';
		if ( !empty($edit_data) )
		{
			if ( $edit_data['t_status'] == 'Active' ) $default_status = 'Active';
			if ( $edit_data['t_status'] == 'Inactive' ) $default_status = 'Inactive';
            if ( $edit_data['t_status'] == 'Deceased' ) $default_status = 'Deceased';
            if ( $edit_data['t_status'] == 'SteppedDown' ) $default_status = 'SteppedDown';
            if ( $edit_data['t_status'] == 'Redacted' ) $default_status = 'Redacted';
			// if ( $edit_data['t_status'] == 'Deprecated' ) $default_status = 'Deprecated';
			if ( $edit_data['t_status'] == 'Training' ) $default_status = 'Training';
            // if ( $edit_data['t_status'] != 'Training' ) unset($statuses['Training']);
		};	

        // if($add)
        //     unset($statuses['Training']);

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
	      '#default_value' => isset($edit_data['t_ct'])?$edit_data['t_ct']:'0',  '#weight' => 16, '#disabled' => true,);
	    $form[$group]['t_cat'] = array('#title' => '&nbsp;CAT', '#type' => 'checkbox',
	      '#default_value' => isset($edit_data['t_cat'])?$edit_data['t_cat']:'0',  '#weight' => 16.1, '#disabled' => true,);

	    $form[$group]['t_responsibility'] = array('#title' => 'Additional Responsibility', '#type' => 'textarea', '#size' => '100', '#weight' => 17, 
			'#default_value' => isset($edit_data['t_responsibility'])?$edit_data['t_responsibility']:'', '#maxlength' => 200  );
	    $form[$group]['t_area'] = array('#title' => 'CAT Area', '#type' => 'textarea', '#size' => '100', '#weight' => 18,
			'#default_value' => isset($edit_data['t_area'])?$edit_data['t_area']:'', '#maxlength' => 255, '#disabled' => true, );
        $form[$group]['t_comments'] = array('#title' => 'Internal Comments', '#type' => 'textarea', '#size' => '100', '#weight' => 18.1,
            '#default_value' => isset($edit_data['t_comments'])?$edit_data['t_comments']:'',  );

        $form[$group]['t_lc_area'] = array('#title' => '&nbsp;Allow LC 2nd Step Review', '#type' => 'checkbox',
          '#default_value' => isset($edit_data['t_lc_area'])?$edit_data['t_lc_area']:'0',  '#weight' => 19);
        $form[$group]['t_allow_cat_emails'] = array('#title' => '&nbsp;Receive CAT emails', '#type' => 'checkbox',
          '#default_value' => isset($edit_data['t_allow_cat_emails'])?$edit_data['t_allow_cat_emails']:'0',  '#weight' => 20);
        $form[$group]['t_disable_group_emails'] = array('#title' => '&nbsp;Disable Group Emails', '#type' => 'checkbox',
          '#default_value' => isset($edit_data['t_disable_group_emails'])?$edit_data['t_disable_group_emails']:'0',  '#weight' => 20);

        $ct_centers = db_query("select tc_id, CONCAT(tc_name,' ','(',c_name,')') from dh_teacher_center left join dh_country on tc_country=c_code order by tc_name")->fetchAllKeyed();

        $cat_areas = db_query("select tac_id, tac_name from dh_teacher_area_cluster")->fetchAllKeyed();

        if(!$add)
        {
            $edit_data['edit_ct_centers'] = db_query("select tr_identifier, tr_identifier from dh_teacher_role where tr_role='CT' and tr_teacher=:tr_teacher and tr_deleted=0", array(':tr_teacher'=>$at_id))->fetchAllKeyed();


            $edit_data['edit_act_centers'] = db_query("select tr_identifier, tr_identifier from dh_teacher_role where tr_role='ACT' and tr_teacher=:tr_teacher and tr_deleted=0", array(':tr_teacher'=>$at_id))->fetchAllKeyed();

            $edit_data['edit_cat_areas'] = db_query("select tr_identifier, tr_identifier from dh_teacher_role where tr_role='CAT' and tr_teacher=:tr_teacher and tr_deleted=0", array(':tr_teacher'=>$at_id))->fetchAllKeyed();

            $edit_data['edit_acat_areas'] = db_query("select tr_identifier, tr_identifier from dh_teacher_role where tr_role='ACAT' and tr_teacher=:tr_teacher and tr_deleted=0", array(':tr_teacher'=>$at_id))->fetchAllKeyed();

        }

        $form[$group]['t_ct_centers'] = array(
            '#title' => '<br>Center Teacher for:',
            '#type' => 'select',
            '#default_value' => isset($edit_data['edit_ct_centers'])?$edit_data['edit_ct_centers']:'',
            '#options' => $ct_centers,
            '#weight' => 21,
            '#multiple' => "multiple",
            '#attributes' => array(
                'id' => 'edit-t-ct-centers',
            ),
        );

        $form[$group]['t_act_centers'] = array(
            '#title' => 'Assistant to Center Teacher for:',
            '#type' => 'select',
            '#default_value' => isset($edit_data['edit_act_centers'])?$edit_data['edit_act_centers']:'',
            '#options' => $ct_centers,
            '#weight' => 22,
            '#multiple' => "multiple",
            '#attributes' => array(
                'id' => 'edit-t-act-centers',
            ),
        );

    $form[$group]['t_cat_areas'] = array(
            '#title' => 'Coordinating Area Teacher for:',
            '#type' => 'select',
            '#default_value' => isset($edit_data['edit_cat_areas'])?$edit_data['edit_cat_areas']:'',
            '#options' => $cat_areas,
            '#weight' => 23,
            '#multiple' => "multiple",
            '#attributes' => array(
                'id' => 'edit-t-cat-areas',
            ),
        );

    $form[$group]['t_acat_areas'] = array(
            '#title' => 'Assistant to Coordinating Area Teacher for:',
            '#type' => 'select',
            '#default_value' => isset($edit_data['edit_acat_areas'])?$edit_data['edit_acat_areas']:'',
            '#options' => $cat_areas,
            '#weight' => 24,
            '#multiple' => "multiple",
            '#attributes' => array(
                'id' => 'edit-t-acat-areas',
            ),
        );

        dh_atportal_role_layout($form[$group]);

	}	

	if ($add)
		$action = 'Add';
	else
		$action = 'Update';

	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 110);

    if(!$add && user_access('add assistant teacher'))
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



        $q = "select tr_updated, tr_role,
        CASE WHEN tac_id IS NOT NULL THEN tac_name ELSE tc_name END AS location,
        tr_deleted, name
        FROM dh_teacher_role
        left join dh_teacher_area_cluster on (tr_identifier=tac_id and tr_role in ('CAT', 'ACAT'))
        left join dh_teacher_center on (tr_identifier=tc_id and tr_role in ('CT', 'ACT'))
        left join users on tr_updated_by=uid
        where tr_teacher=$at_id
        order by tr_updated desc";

        $res = db_query($q);
        $rows = array();
        while( $row = $res->fetchAssoc())
        {
                $l = array();
                $l[] = $row['tr_updated'];
                $l[] = $row['tr_role'];
                $l[] = $row['location'];
                $l[] = ($row['tr_deleted']) ? "Deleted" : "Added";
                $l[] = $row['name'];
                $rows[] = $l;
        }
        $header = array('DateTime', 'Role', 'Location', 'Status', 'User');
        $out = '';
        if (!empty($rows))
            $out .= theme('table', array('header' => $header, 'rows' => $rows)) ;

        $form['teacher_role_log'] = array('#type' => 'fieldset','#title' => '<h2>Role Log - Click to open</h2>' , '#collapsible' => true, '#collapsed' => true, '#weight' => 113,);
        $form['teacher_role_log']['a'] = array('#markup' => $out);
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
        $("#edit-t-ct-centers").select2({ width: "100%" });
        $("#edit-t-act-centers").select2({ width: "100%" });
        $("#edit-t-cat-areas").select2({ width: "100%" });
        $("#edit-t-acat-areas").select2({ width: "100%" });

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
    $in = $form_state['input'];
    foreach (array('t_email' => 'Email 1', 't_email_alt' => 'Email 2') as $k => $label)
        if (isset($in[$k]) && trim($in[$k]) !== '' && !valid_email_address(trim($in[$k])))
            form_set_error($k, t('@f: please enter a valid email address.', array('@f' => $label)));
    foreach (array('t_mob_phone' => 'Mobile 1', 't_mob_phone_alt' => 'Mobile 2') as $k => $label)
        if (isset($in[$k]) && trim($in[$k]) !== '')
        {
            $p = dh_atportal_phone_parse($in[$k], isset($in[$k.'_region']) ? $in[$k.'_region'] : '');
            if ($p)
                $form_state['storage']['phones'][$k] = $p;
            else
                form_set_error($k, t('@f: this is not a valid mobile number for the chosen country.', array('@f' => $label)));
        }
    if (isset($in['t_dob']) && dh_teacher_dob_value($in['t_dob']) === FALSE)
        form_set_error('t_dob][day', t('Date of Birth: please choose a day, month and year that make a real date (or leave all three empty).'));
    // The photo (already cropped by the photo editor in the browser), saved on submit.
    $photo = file_save_upload('upload_photo', array('file_validate_extensions' => array('png jpg jpeg'), 'file_validate_size' => array(10 * 1024 * 1024)));
    if ($photo)
        $form_state['storage']['teacher_photo'] = $photo;

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

	// Only the fields this person may change. This used to copy every posted t_* key into
	// dh_teacher, so a crafted POST could set its own t_status, t_code or role flags.
	$t = array();
	foreach (dh_atportal_profile_fields(user_access('add assistant teacher')) as $key)
		if (array_key_exists($key, $form_state['input']))
			$t[$key] = $form_state['input'][$key];
	if (user_access('add assistant teacher'))
		foreach (array('t_sat', 't_full_t', 't_bhante', 't_lc_area', 't_allow_cat_emails', 't_disable_group_emails', 't_ct_centers', 't_act_centers', 't_cat_areas', 't_acat_areas') as $key)
			if (!isset($t[$key]))
				$t[$key] = '';

	if ($t['t_email'])
		$t['t_email'] = trim($t['t_email']);

	// mobiles are kept as the full international number (+9198…), with the country code beside it
	foreach (array('t_mob_phone' => 't_mob_country', 't_mob_phone_alt' => 't_mob_country_alt') as $k => $cc)
		if (array_key_exists($k, $t))
		{
			$t[$k] = trim((string) $t[$k]);
			if ($t[$k] !== '' && isset($form_state['storage']['phones'][$k]))
				list($t[$k], $t[$cc]) = $form_state['storage']['phones'][$k];
		}

	if (array_key_exists('t_dob', $t))
		$t['t_dob'] = dh_teacher_dob_value($t['t_dob']);

	if (user_access('add assistant teacher'))
	{
        $new_t_role['CT'] = $t['t_ct_centers'];
        unset($t['t_ct_centers']);

        $new_t_role['ACT'] = $t['t_act_centers'];
        unset($t['t_act_centers']);

        $new_t_role['CAT'] = $t['t_cat_areas'];
        unset($t['t_cat_areas']);

        $new_t_role['ACAT'] = $t['t_acat_areas'];
        unset($t['t_acat_areas']);

        if ($t['t_at_appointed']['date'] == '')
            $t['t_at_appointed'] = NULL;

        if ($t['t_sat_appointed']['date'] == '')
            $t['t_sat_appointed'] = NULL;

        if ($t['t_full_t_appointed']['date'] == '')
            $t['t_full_t_appointed'] = NULL;

        if ($t['t_bt_appointed']['date'] == '')
            $t['t_bt_appointed'] = NULL;

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
        if ($t['t_disable_group_emails'] == '')
            $t['t_disable_group_emails'] = '0';

        if($t['t_bhante'])
          $t['t_year_appointed'] = $t['t_bt_appointed'];
        elseif($t['t_full_t'])
          $t['t_year_appointed'] = $t['t_full_t_appointed'];
        elseif($t['t_sat'])
          $t['t_year_appointed'] = $t['t_sat_appointed'];
        else
          $t['t_year_appointed'] = $t['t_at_appointed'];

        if($t['t_year_appointed'])
        {
            $t['t_year_appointed'] = date("Y", strtotime($t['t_year_appointed']['date']));
        }
        else
        {
            $t['t_year_appointed'] = null;
        }

        unset($t['t_ct']);
        unset($t['t_cat']);
        unset($t['t_area']);
	}




	// date_popup fields post array('date' => 'Y-m-d'): store the date text, or NULL when empty
	foreach (array('t_at_appointed', 't_sat_appointed', 't_full_t_appointed', 't_bt_appointed') as $key)
		if (isset($t[$key]) && is_array($t[$key]))
			$t[$key] = (isset($t[$key]['date']) && $t[$key]['date'] !== '') ? $t[$key]['date'] : NULL;

    $at_id = "";
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
		// a field that was never filled and is still empty stays NULL, so the Change Log shows no false change
		$old = db_query("select * from dh_teacher where t_id = :t", array(':t' => $at_id))->fetchAssoc();
		foreach ($t as $k => $v)
			if ($v === '' && $old && array_key_exists($k, $old) && $old[$k] === NULL)
				$t[$k] = NULL;
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
		$at_id = db_insert('dh_teacher')->fields($t)->execute();
	}

    if (user_access('add assistant teacher'))
    {
        foreach($new_t_role as $role_type => $new_values)
        {
            if(!$new_values)
                $new_values = array();

            $old_values = array_keys(db_query("select tr_identifier from dh_teacher_role where tr_role=:tr_role and tr_teacher=:tr_teacher and tr_deleted=0", array(':tr_role'=>$role_type, ':tr_teacher'=>$at_id))->fetchAllAssoc('tr_identifier'));

            $added_values = array_diff($new_values, $old_values);
            $removed_values = array_diff($old_values, $new_values);

            if(is_array($added_values) && count($added_values))
            {
                foreach($added_values as $added_value)
                {
                    $tr = array();
                    $tr['tr_teacher'] = $at_id;
                    $tr['tr_identifier'] = $added_value;
                    $tr['tr_role'] = $role_type;
                    $tr['tr_created_by'] = $user->uid;
                    $tr['tr_updated_by'] = $user->uid;
                    db_insert('dh_teacher_role')->fields($tr)->execute();
                }
            }

            if(is_array($removed_values) && count($removed_values))
            {
                    $tr = array();
                    $tr['tr_updated'] = date('Y-m-d H:i:s');
                    $tr['tr_updated_by'] = $user->uid;
                    $tr['tr_deleted'] = 1;

                    db_update('dh_teacher_role')
                        ->fields($tr)
                        ->condition('tr_identifier', $removed_values, 'IN')
                        ->condition('tr_teacher', $at_id)
                        ->condition('tr_role', $role_type)
                        ->execute();
            }

        }

        $cat_areas = db_query("select
                GROUP_CONCAT(tac_name SEPARATOR ', ') AS areas
                FROM dh_teacher
                LEFT JOIN dh_teacher_role ON t_id = tr_teacher
                LEFT JOIN dh_teacher_area_cluster ON tr_identifier = tac_id
                where
                t_id=:at_id
                AND tr_deleted = 0
                AND tr_role = 'CAT'
                GROUP BY t_id", array(':at_id' => $at_id))->fetchField();

        if($cat_areas)
        {
            db_query("update dh_teacher set t_area=:t_area, t_cat=1 where t_id=:t_id",
            array(':t_area' => $cat_areas, 't_id' => $at_id));
        }
        else
        {
            db_query("update dh_teacher set t_area='', t_cat=0 where t_id=:t_id",
            array('t_id' => $at_id));
        }

        $ct_flag = db_query("select
            count(t_id) AS count
            FROM dh_teacher
            LEFT JOIN dh_teacher_role ON t_id = tr_teacher
            where
            t_id=:at_id
            AND tr_deleted = 0
            AND tr_role = 'CT'
            GROUP BY t_id", array(':at_id' => $at_id))->fetchField();

        if($ct_flag)
        {
            db_query("update dh_teacher set t_ct=1 where t_id=:t_id",
            array('t_id' => $at_id));
        }
        else
        {
                db_query("update dh_teacher set t_ct=0 where t_id=:t_id",
            array('t_id' => $at_id));
        }

    }


	// Bio data, family members and photo (bio.inc).
	if ($at_id && dh_teacher_bio_ready())
	{
		dh_teacher_bio_save($at_id, $form_state['input']);
		if (isset($form_state['input']['family_json']))
			dh_teacher_family_save($at_id, dh_teacher_family_clean($form_state['input']['family_json']));
	}
	if ($at_id && !empty($form_state['storage']['teacher_photo']))
	{
		if (!dh_teacher_photo_save($at_id, $form_state['storage']['teacher_photo']))
			drupal_set_message(t('The photo could not be saved. Please try again.'), 'error');
		unset($form_state['storage']['teacher_photo']);
	}

	drupal_set_message(t("Details added/updated successfully"), 'status');

}

/**
 * The dh_teacher fields the profile form may change: the teacher's own details, plus the
 * role fields for the AT Book team ('add assistant teacher').
 */
function dh_atportal_profile_fields($admin)
{
	$own = array('t_dob', 't_address', 't_pincode', 't_country', 't_state', 't_city', 't_mob_country', 't_mob_phone',
		't_mob_country_alt', 't_mob_phone_alt', 't_email', 't_email_alt', 't_martial_status');
	if (!$admin)
		return $own;
	return array_merge($own, array('t_f_name', 't_l_name', 't_gender', 't_status', 't_code', 't_at_appointed', 't_sat', 't_sat_appointed',
		't_full_t', 't_full_t_appointed', 't_bhante', 't_bt_appointed', 't_responsibility', 't_comments', 't_lc_area',
		't_allow_cat_emails', 't_disable_group_emails', 't_ct_centers', 't_act_centers', 't_cat_areas', 't_acat_areas'));
}

/**
 * The country a stored mobile belongs to, for the phone widget: from a number written with +code,
 * else from its saved country code (the teacher's own country when several share that code),
 * else the teacher's country, else India. An empty field starts on India.
 */
function dh_atportal_phone_region($phone, $cc, $home)
{
	$util = \libphonenumber\PhoneNumberUtil::getInstance();
	$phone = trim((string) $phone);
	$home = strtoupper(trim((string) $home));
	if ($phone === '')
		return 'IN';
	if ($phone[0] == '+')
	{
		try
		{
			$r = $util->getRegionCodeForNumber($util->parse($phone, NULL));
			if ($r)
				return $r;
		}
		catch (\Exception $e) {}
	}
	if ((int) $cc > 0)
	{
		$regions = $util->getRegionCodesForCountryCode((int) $cc);
		if ($regions)
			return in_array($home, $regions) ? $home : $regions[0];
	}
	return in_array($home, $util->getSupportedRegions()) ? $home : 'IN';
}

/**
 * A mobile number as typed, read for the chosen country: array(E.164, country code), or FALSE.
 * Accepted when libphonenumber calls it valid, or when its length fits a mobile in that country
 * (new number ranges are not in the library's list yet), the same rule as the schedule site.
 */
function dh_atportal_phone_parse($value, $region)
{
	$util = \libphonenumber\PhoneNumberUtil::getInstance();
	$region = preg_match('/^[A-Za-z]{2}$/', (string) $region) ? strtoupper($region) : 'IN';
	try
	{
		$p = $util->parse(trim((string) $value), $region);
	}
	catch (\Exception $e)
	{
		return FALSE;
	}
	if (!$util->isValidNumber($p) && !$util->isPossibleNumberForType($p, \libphonenumber\PhoneNumberType::MOBILE))
		return FALSE;
	return array($util->format($p, \libphonenumber\PhoneNumberFormat::E164), (string) $p->getCountryCode());
}

/**
 * Lays out the Role Information section (AT Book team) in groups, top to bottom: status and code;
 * appointments, one row per role with its tick box beside its date; centres and areas; notes;
 * settings. Field names and ids stay the same, so validation, saving and the page script that
 * switches the dates on and off are unchanged. The CT and CAT tick boxes and the CAT Area box
 * are left out: they could not be edited, and saving works them out from the centre and area lists.
 */
function dh_atportal_role_layout(&$g)
{
	$take = function ($keys) use (&$g) {
		$out = array();
		foreach ($keys as $k)
			if (isset($g[$k]))
			{
				$out[$k] = $g[$k];
				unset($g[$k]);
			}
		return $out;
	};
	$box = function ($weight, $classes, $heading = '') {
		$b = array('#type' => 'container', '#weight' => $weight, '#attributes' => array('class' => $classes));
		if ($heading)
			$b['head'] = array('#markup' => '<h4>'.$heading.'</h4>', '#weight' => -100);
		return $b;
	};
	unset($g['#description'], $g['t_ct'], $g['t_cat'], $g['t_area']);
	$g['t_full_t']['#title'] = '&nbsp;Full-T';
	$g['t_ct_centers']['#title'] = 'Center Teacher for';
	$g['t_act_centers']['#title'] = 'Assistant to Center Teacher for';
	$g['t_cat_areas']['#title'] = 'Coordinating Area Teacher for';
	$g['t_acat_areas']['#title'] = 'Assistant to Coordinating Area Teacher for';
	$g['t_responsibility']['#rows'] = 3;
	$g['t_comments']['#rows'] = 3;

	$g['role_main'] = $box(1, array('dh-role-grid', 'dh-role-3')) + $take(array('t_status', 't_code', 't_year_appointed'));

	$g['role_appoint'] = $box(2, array('dh-role-appoint'), 'Appointments');
	$g['role_appoint']['cols'] = array('#markup' => '<div class="dh-role-row dh-role-cols"><span>Role</span><span>Date appointed</span></div>', '#weight' => -99);
	$w = 1;
	foreach (array('t_at_appointed' => 'AT', 't_sat_appointed' => 't_sat', 't_full_t_appointed' => 't_full_t', 't_bt_appointed' => 't_bhante') as $date => $tick)
	{
		$row = $box($w++, array('dh-role-row'));
		if (isset($g[$tick]))
			$row += $take(array($tick));
		else
			$row['name'] = array('#markup' => '<span class="dh-role-name">'.$tick.'</span>', '#weight' => -1);
		$g['role_appoint']['row'.$w] = $row + $take(array($date));
	}
	$g['role_appoint']['note'] = array('#weight' => 10, '#markup' => '<ul class="dh-role-note">'.
		'<li>In case of direct appointment to SAT, please use AT appointment date same as SAT</li>'.
		'<li>In case of direct appointment to Full-T, please use AT and SAT appointment date same as Full-T</li>'.
		'<li>Similarly in case of direct appointment from AT to Full-T, please use SAT appointment date same as Full-T</li></ul>');

	$g['role_places'] = $box(3, array('dh-role-grid'), 'Centres and areas') + $take(array('t_ct_centers', 't_act_centers', 't_cat_areas', 't_acat_areas'));
	$g['role_notes'] = $box(4, array('dh-role-grid'), 'Notes') + $take(array('t_responsibility', 't_comments'));
	$g['role_flags'] = $box(5, array('dh-role-flags'), 'Settings') + $take(array('t_lc_area', 't_allow_cat_emails', 't_disable_group_emails'));
}


function dh_atportal_ctlist()
{
    drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
    drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

    $out = '<h2>'.l("Back to AT-Portal", "at-portal")." | ".l("Back to AT Address Book", "at-portal/address-book").'</h2>';


    $q = "select
    tc_name,
    concat(tc_city, ', ' ,s_name, ', ', c_name) as tc_address,
    GROUP_CONCAT(IF(tr_role = 'CT', concat(t_f_name, ' ', t_l_name, ' (', t_code, ')'), NULL) SEPARATOR '<br>') as tc_ct,
    GROUP_CONCAT(IF(tr_role = 'ACT', concat(t_f_name, ' ', t_l_name, ' (', t_code, ')'), NULL) SEPARATOR '<br>') as tc_act
    FROM dh_teacher_role
    left join dh_teacher_center on tr_identifier=tc_id
    left join dh_teacher on tr_teacher=t_id
    left join dh_country on tc_country=c_code
    left join dh_state on (tc_country=s_country and tc_state=s_code)
    where
    tr_role in ('CT', 'ACT')
    and tr_deleted=0
    and t_status='active'
    group by tc_id;
        ";

    $result = db_query($q);
    $data = array();

    while( $r = $result->fetchAssoc() )
    {
        $data[] = $r;
    }

    $data = json_encode($data);

    $header = array();
    $attributes = array('id' => 'ct-list');
  $out .= theme('table', array('header' => $header, 'rows' => array(array()) ,'attributes' => $attributes));
    $js = '
        (function ($) {
            function do_datatable( tid, dataset )
            {
                var table = $(tid).DataTable({
                    "bDestroy": true,
                    data: dataset,
                    columns: [
                        { data: "tc_name", "title" : "Center Name" },
                        { data: "tc_address", "title" : "Address"},
                        { data: "tc_ct", "title" : "Center Teacher" },
                        { data: "tc_act", "title" : "Assistant to Center Teacher" },
                    ],
                });

            }
            $(document).ready(function(){
                var dataset = '.$data.';
                do_datatable("#ct-list", dataset);
            });
        })(jQuery);

    ';
    drupal_add_js($js, 'inline');

        $css = '#ct-list td:nth-child(1) { width: 25%; text-align: left }
                #ct-list td:nth-child(2) { width: 25%; text-align: left }
                #ct-list td:nth-child(3) { width: 25%; text-align: left }
                #ct-list td:nth-child(4) { width: 25%; text-align: left }
        ';
    drupal_add_css($css, 'inline');

    return $out;
}



function dh_atportal_catlist()
{
    drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
    drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

    $out = '<h2>'.l("Back to AT-Portal", "at-portal")." | ".l("Back to AT Address Book", "at-portal/address-book").'</h2>';

    $q = "select
        tac_name,
        GROUP_CONCAT(IF(tr_role = 'CAT', concat(t_f_name, ' ', t_l_name, ' (', t_code, ')'), '') SEPARATOR '<br>') as ta_cat,
        GROUP_CONCAT(IF(tr_role = 'ACAT', concat(t_f_name, ' ', t_l_name, ' (', t_code, ')'), '') SEPARATOR '<br>') as ta_acat
        from dh_teacher_role
        left join dh_teacher_area_cluster on tr_identifier=tac_id
        left join dh_teacher on tr_teacher=t_id
        where
        tr_role in ('CAT', 'ACAT')
        and tr_deleted=0
        and t_status='active'
        group by tac_id;
        ";

    $result = db_query($q);
    $data = array();

    while( $r = $result->fetchAssoc() )
    {
        $data[] = $r;
    }

    $data = json_encode($data);

    $header = array();
    $attributes = array('id' => 'cat-list');
  $out .= theme('table', array('header' => $header, 'rows' => array(array()) ,'attributes' => $attributes));
    $js = '
        (function ($) {
            function do_datatable( tid, dataset )
            {
                var table = $(tid).DataTable({
                    "bDestroy": true,
                    data: dataset,
                    columns: [
                        { data: "tac_name", "title" : "Area" },
                        { data: "ta_cat", "title" : "CAT"},
                        { data: "ta_acat", "title" : "Assistant to CAT" },
                    ],
                });

            }
            $(document).ready(function(){
                var dataset = '.$data.';
                do_datatable("#cat-list", dataset);
            });
        })(jQuery);

    ';
    drupal_add_js($js, 'inline');

    $css = '#cat-list td:nth-child(1) { width: 50%; text-align: left }
            #cat-list td:nth-child(2) { width: 25%; text-align: left }
            #cat-list td:nth-child(3) { width: 25%; text-align: left }
        ';
    drupal_add_css($css, 'inline');

    return $out;
}


function dh_atportal_manage_area_cluster()
{
    switch( arg(2) )
    {
        case 'add':
            return drupal_get_form('dh_atportal_area_cluster_form');
            break;
        case 'edit':
            $tac_id = arg(3);
            $row = db_query("select * from dh_teacher_area_cluster where tac_id=$tac_id")->fetchAssoc();
            return drupal_get_form('dh_atportal_area_cluster_form', $row);
            break;
    }
}




function dh_atportal_area_cluster_form($form, &$form_state, $edit_data = array())
{
    $add = 0;
    if (count($edit_data) == 0)
        $add = 1;

    if(!$add)
        $tac_id = arg(3);

    $form['at-portal'] = array('#markup' => '<h2>'.l("Back to AT-Portal", "at-portal")." | ".l("Back to AT Address Book", "at-portal/address-book")." | ".l("Back to Area Cluster List", "at-portal/area-cluster-list").'</h2>'.'</h2>');

    $form['tac_name'] = array(
        '#title' => 'Area Cluster Name',
        '#type' => 'textarea',
        '#size' => '100',
        '#weight' => 1,
        '#default_value' => isset($edit_data['tac_name'])?$edit_data['tac_name']:'',
        '#maxlength' => 255
    );


    $tac_areas = db_query("select ta_id, CASE WHEN ta_type = 'country' THEN ta_name WHEN ta_type ='state' THEN CONCAT(ta_name,' ','(',ta_country,')') WHEN ta_type ='zip' THEN CONCAT(ta_name,' ','(',ta_state, ', ',ta_country,')')  END  from dh_teacher_area;")->fetchAllKeyed();

    if(!$add)
    {
        $edit_data['edit_tac_areas'] = db_query("select ta_id, ta_id from dh_teacher_area where ta_cluster=:ta_cluster", array(':ta_cluster'=>$tac_id))->fetchAllKeyed();

    }

    $form['tac_areas'] = array(
            '#title' => 'Containing Areas',
            '#type' => 'select',
            '#default_value' => isset($edit_data['edit_tac_areas'])?$edit_data['edit_tac_areas']:'',
            '#options' => $tac_areas,
            '#weight' => 2,
            '#multiple' => "multiple",
            '#attributes' => array(
                'id' => 'edit-tac-areas',
            ),
        );


    if ($add)
        $action = 'Add';
    else
        $action = 'Update';

    $form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 110);



    $js ='
    (function ($) {
      $(document).ready(function(){
        $("#edit-tac-areas").select2({ width: "100%" });
      });
    })(jQuery);
  ';
    drupal_add_js($js, 'inline');

    drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
    drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

    return $form;
}


function dh_atportal_area_cluster_form_submit($form, &$form_state)
{
    global $user;
    $input = $form_state['input'];

    $new_tac_areas = $input['tac_areas'];
    unset($input['tac_areas']);

    $tac_id = '';
    if ( arg(2) == 'edit')
    {
        $tac_id = arg(3);
        $tac['tac_name'] = $input['tac_name'];
        $tac['tac_updated_by'] = $user->uid;
        $tac['tac_updated'] = date('Y-m-d H:i:s');
        db_update('dh_teacher_area_cluster')->fields($tac)->condition('tac_id', $tac_id )->execute();
    }
    else
    {
        $tac['tac_name'] = $input['tac_name'];
        $tac['tac_created_by'] = $user->uid;
        $tac['tac_updated_by'] = $user->uid;
        $tac_id = db_insert('dh_teacher_area_cluster')->fields($tac)->execute();
    }

    if(!$new_tac_areas)
        $new_tac_areas = array();

    $old_tac_areas = array_keys(db_query("select ta_id from dh_teacher_area where ta_cluster=:ta_cluster", array(':ta_cluster'=>$tac_id))->fetchAllAssoc('ta_id'));

    $added_tac_areas = array_diff($new_tac_areas, $old_tac_areas);
    $removed_tac_areas = array_diff($old_tac_areas, $new_tac_areas);

    $ta = array();
    $ta['ta_updated_by'] = $user->uid;
    $ta['ta_updated'] = date('Y-m-d H:i:s');

    if(is_array($added_tac_areas) && count($added_tac_areas))
    {
            $ta['ta_cluster'] = $tac_id;
            db_update('dh_teacher_area')
                ->fields($ta)
                ->condition('ta_id', $added_tac_areas, 'IN')
                ->execute();
    }

    if(is_array($removed_tac_areas) && count($removed_tac_areas))
    {
            $ta['ta_cluster'] = 0;
            db_update('dh_teacher_area')
                ->fields($ta)
                ->condition('ta_id', $removed_tac_areas, 'IN')
                ->execute();
    }

    drupal_set_message(t("Details added/updated successfully"), 'status');

}


function dh_atportal_area_cluster_list()
{
    drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
    drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

    $out = '<h2>'.l("Add Area Cluster", "at-portal/area-cluster/add")." | ".l("Back to AT Portal", "at-portal")." | ".l("Back to AT Address Book", "at-portal/address-book").'</h2>';

    $q = "select
        tac_id,
        tac_name,
        GROUP_CONCAT(CASE WHEN ta_type = 'country' THEN ta_name WHEN ta_type ='state' THEN CONCAT(ta_name,' ','(',ta_country,')') WHEN ta_type ='zip' THEN CONCAT(ta_name,' ','(',ta_state, ', ',ta_country,')')  END SEPARATOR '<br>') as tac_areas
        from dh_teacher_area_cluster
        left join dh_teacher_area on tac_id=ta_cluster
        group by tac_id;
        ";

    $result = db_query($q);
    $data = array();

    while( $r = $result->fetchAssoc() )
    {
        $r["tac_id"] = l("Edit", "/at-portal/area-cluster/edit/".strtolower($r['tac_id']));
        $data[] = $r;
    }

    $data = json_encode($data);

    $header = array();
    $attributes = array('id' => 'tac-list');
  $out .= theme('table', array('header' => $header, 'rows' => array(array()) ,'attributes' => $attributes));
    $js = '
        (function ($) {
            function do_datatable( tid, dataset )
            {
                var table = $(tid).DataTable({
                    "bDestroy": true,
                    data: dataset,
                    columns: [
                        { data: "tac_id", "title" : "Edit" },
                        { data: "tac_name", "title" : "Cluster Name"},
                        { data: "tac_areas", "title" : "Containing Areas" },
                    ],
                });

            }
            $(document).ready(function(){
                var dataset = '.$data.';
                do_datatable("#tac-list", dataset);
            });
        })(jQuery);

    ';
    drupal_add_js($js, 'inline');

    $css = '#tac-list td:nth-child(1) { width: 10%; text-align: left }
            #tac-list td:nth-child(2) { width: 45%; text-align: left }
            #tac-list td:nth-child(3) { width: 45%; text-align: left }
        ';
    drupal_add_css($css, 'inline');

    return $out;
}
