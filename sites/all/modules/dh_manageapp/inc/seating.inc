<?php

function dh_generate_seating_plan2($centre, $course, $pdf = '')
{
	global $base_url;
	if (!is_array($course))
		$course = array($course);

	$out = '<html><head>
  <script src="https://unpkg.com/sortablejs-make/Sortable.min.js"></script>
  <script src="https://code.jquery.com/jquery-2.2.4.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/jquery-sortablejs@latest/jquery-sortable.js"></script>
  <script>
  (function ($) { 

  	 function assign_seat( seat, data )
  	 {
  	 	$("div[seat-no="+seat+"]").find(".s-acco").text(data.acco);
  	 	$("div[seat-no="+seat+"]").find(".s-name").text(data.name);
  	 	$("div[seat-no="+seat+"]").find(".s-old-student").text(data.os);
  	 	$("div[seat-no="+seat+"]").find(".s-age").text("Age: "+data.age);
  	 	if (data.backrest)
  	 		$("div[seat-no="+seat+"]").find(".s-backrest").addClass("s-backrest-yes");
  	 	else
  	 		$("div[seat-no="+seat+"]").find(".s-backrest").removeClass("s-backrest-yes");
  	 }

  	 $(document).ready(function() {
  	 	assign_seat("m-R8C3", {"acco": "B-101", "name": "Vinod Kharat", "os": "(O) s:10 L:2", "age": 30, "backrest": true});
  	 	assign_seat("m-R8C4", {"acco": "B-102", "name": "Anand Sri", "os": "(O) s:10 L:2", "age": 30, "backrest": false});

  	 	//var el = document.getElementById("sortable-m");
  	 	$("#sortable-m").sortable({group: "list", animation: 200, ghostClass: "ghost", "filter": ".ui-state-disabled", 
  	 		onStart: function() {
			        $(".ui-state-disabled", this).each(function(){
			            var $this = $(this);
			            $this.data("pos", $this.index());
			        });
  	 			},
  	 		onChange: function() {
  	 			
		        $sortable = $(this);
		        $statics = $(".ui-state-disabled", this).detach();
		        $helper = $("<div class=\"ui-state-default\"></div>").prependTo(this);
		        $statics.each(function(){
		            var $this = $(this);
		            var target = $this.data("pos");
		            
		            $this.insertAfter($("div", $sortable).eq(target));
		        });
		        $helper.remove();
		        
  	 		}
  	 	});

    	$("#sortable-m").disableSelection();
  	 });

  })(jQuery);
  </script>
';

	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchField();
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);

	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/seating2.css");</style></head><body>';
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
	if ( strtoupper ($ini['MALE-CHOWKY']['Direction']) == 'LEFT')
	{
		$tmp = get_seating_layout($ini['MALE-CHOWKY'], "mc");
		$out .= $tmp['out'];
	}
	$tmp = get_seating_layout($ini['MALE'], "m");
	$out .= $tmp['out'];
	if ( strtoupper ($ini['MALE-CHOWKY']['Direction']) == 'RIGHT')
	{
		$tmp = get_seating_layout($ini['MALE-CHOWKY'], "mc");
		$out .= $tmp['out'];
	}

	$out .= '<div class="teacher-seat">Seating Plan - <br>'.$course_name.'</div>';
	$out .= '</body></html>';

	print $out;
	exit;
}



function get_seating_layout($ini, $prefix)
{
	$seat_width = 140;
	$buffer = 20;
	$seats_per_row = $ini['SeatsPerRow'];
	$max_rows = $ini['MaxRows'];
	$max_seats_per_row = $seats_per_row;
	$direction = strtoupper($ini['Direction']);
	/* Calculate max seats per row based on config */
	$config = array();
	foreach ($ini as $key => $value) 
	{
		$config[strtoupper(trim($key))] = $value;
		if ( strtoupper(substr(trim($key),0,10)) == 'SEATS-ROW-' )
		{
			if ( $value > $max_seats_per_row )
				$max_seats_per_row = $value;
		}
	}
	$width = ($seat_width * $max_seats_per_row) + $buffer;
	if ($direction == 'LEFT')
	{
		$col_start = -$max_seats_per_row + 1;
		$col_end = 1;
	}
	else
	{
		$col_start = 0;
		$col_end = $max_seats_per_row;
	}
	$suffix = "m";
	$out = '<div class="seating-plan"><div id="sortable-'.$prefix.'" class="sortable-'.$suffix.' ui-sortable list-group" style="width: '.$width.'px">';
	$seat_classes = array( 's-acco', 's-name', 's-old-student', 's-age', 's-backrest');
	/* Calculate grid layout */
	for ($r=($max_rows - 1); $r >= 0; $r--) 
	{ 
		$total_seats = $seats_per_row;
		$row_no = $r + 1;
		if ( isset( $config['SEATS-ROW-'.$row_no]) )
			$total_seats = $config['SEATS-ROW-'.$row_no];
		$disabled_seats = array();
		for ($i = $total_seats; $i < $max_seats_per_row; $i++ )
			$disabled_seats[] = $i;
		if (isset($config['DISABLED-SEATS-ROW-'.$row_no]))
		{
			$tmp = explode(",", $config['DISABLED-SEATS-ROW-'.$row_no]);
			foreach($tmp as $k)
				$disabled_seats[] = $k - 1;
		}
		//print("R - ".$row_no.print_r($disabled_seats, true));
		for ($c=$col_start; $c < $col_end; $c++)
		{
			$class = "";
			$class_disabled = "";
			if (in_array(abs($c), $disabled_seats))
			{
				$class = "disabled";
				$class_disabled = "ui-state-disabled";
			}
			$seat_no = 'R'.$row_no.'C'.(abs($c)+1);
			$out .= '<div class="ui-state-default '.$class_disabled.'"><div class="seat '.$class.'" seat-no="'.$prefix.'-'.$seat_no.'" >';
			$out .= '<div class="s-seat-no">'.$seat_no.'</div>';
			foreach ($seat_classes as $value) 
			{
				$out .= '<div class="'.$value.'"></div>';
			}
			$out .= '</div></div>';
			//<div class="s-seat-no">R'.$row_no.'C'.(abs($c)+1).'</div></div></li>';
		}
	}

	$out .= '</div></div>';
	return array('max rows' => $max_seats_per_row, 'out' => $out );
}
