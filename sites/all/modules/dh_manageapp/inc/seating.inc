<?php

function dh_generate_seating_plan2($centre, $course, $pdf = '')
{
	global $base_url;
	if (!is_array($course))
		$course = array($course);

	$out = '<html><head><script src="https://code.jquery.com/jquery-3.6.0.js"></script>
  <script src="https://code.jquery.com/ui/1.13.2/jquery-ui.js"></script>
  <script>
  $( function() {


  } );
  </script>
';

	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchField();
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);

	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/seating.css");</style></head><body>';
	$out .= '<div class="helptext no-print">
		<h2>Editing Seating Plan:</h2>
		<p>1- Check basic Hall Settings in <a href="/wiki-centre-settings">Center Settings</a>.</p>
		<p>2- Rearrange the plan by dragging and dropping student seats in grid.</p>
		<p>3- Create empty rows for a grid by clicking "Add Empty Row" button if needed.</p>
		<p>4- Print by clicking "Print Seating Plan" button or CTL-P.</p>
		<p>5- Adjust print settings(Layout, Scale, Paper Size) as needed and print the seating plan.</p>
		<p>Note: All help text and buttons will be ignored on print.</p>
		</div>
		<button class="no-print" onclick="window.print()">Print Seating Plan</button>';
	if ( is_array($course) )
	{
		$course_name = '';
		$h = db_query("select c_name from dh_course where c_id in (".implode(",", $course).")");
		while($r = $h->fetchAssoc())
			$course_name .= $r['c_name']." / ";
	}

	$out .= get_seating_layout($ini['MALE']);
	$out .= '<div class="teacher-seat">Seating Plan - <br>'.$course_name.'</div>';
	$out .= '</body></html>';

	print $out;
	exit;
}



function get_seating_layout($ini)
{
	$seats_per_row = $ini['SeatsPerRow'];
	$max_rows = $ini['MaxRows'];
	$max_seats_per_row = $seats_per_row;
	/* Calculate max seats per row based on config */
	$config = array();
	foreach ($ini as $key => $value) 
	{
		$config[strtoupper(trim($key))] = $value;
		if ( strtoupper(substr(trim($key),10)) == 'SEATS-ROW-' )
		{
			if ( $value > $max_seats_per_row )
				$max_seats_per_row = $value;
		}
	}
	$out = '';
	/* Calculate grid layout */
	for ($r=0; $r < $max_rows; $r++) 
	{ 
		for ($c=0; $c < $max_seats_per_row; $c++)
		{
			$out .= '<li></li>';
		}
	}
	return $out;
}
