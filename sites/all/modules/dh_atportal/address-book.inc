<?php

function address_book_search()
{
	// DB table to use
	$table = 'dh_teacher';
	 
	// Table's primary key
	$primaryKey = 't_id';
	 
	// Array of database columns which should be read and sent back to DataTables.
	// The `db` parameter represents the column name in the database, while the `dt`
	// parameter represents the DataTables column identifier. In this case simple
	// indexes
	$columns = array(
	    array( 'db' => 't_code', 'dt' => 0 ),
	    array( 'db' => 't_gender',     'dt' => 1 ),
	    array( 'db' => 't_f_name', 'dt' => 2 ),
	    array( 'db' => 't_l_name',  'dt' => 3 ),
	    array( 'db' => 't_current_status',     'dt' => 4 ),
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
	    'user' => 'dipi',
	    'pass' => 'sdfkj38443iufnkdfjkn',
	    'db'   => 'dipi',
	    'host' => 'localhost'
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
	$header = array('Code', 'Gender', 'FName', 'LName', 'CurrentStatus', 'Email', 'Country', 'Mobile', 'SAT', 'CT', 'FullT', 'CAT', 'Bhante'); 
	$attributes = array('id' => 'at-address-book');

	//$out = theme('table', array('header' => $header, 'rows' => $rows_pending, 'attributes' => $attributes));
	$out .= theme('table', array('header' => $header, 'rows' => array(array('1','2','3','4','5','1','2','3')), 'attributes' => $attributes));

	$js = '
		(function ($) {
			function do_datatable( tid )
			{
				var table = $(tid).DataTable({
					dom: "lfr<\"clear\">tip", 
			        "processing": true,
			        "serverSide": true,
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

function dh_atportal_profile($form, &$form_state, $at_code)
{
	global $user;

	$form['a'] = array('#markup' => '<h2>Profile for '.$at_code.'</h2>');

	return $form;
}


