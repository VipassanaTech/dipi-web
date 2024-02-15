<?php

require_once 'vendor/autoload.php';
use PhpOffice\PhpSpreadsheet\Spreadsheet;
use PhpOffice\PhpSpreadsheet\Writer\Xlsx;
use Mailgun\Mailgun;


function dh_manage_courses_main()
{
	if (arg(2) <> '')
		return dh_course_dashboard();
	else
		return dh_manage_courses();
}

function dh_course_dashboard()
{
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
	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];
	$temp = db_query("select c_name, c_finalized from dh_course where c_id=".arg(2))->fetchAssoc();
	$course_name = $temp['c_name'];
	$finalized = $temp['c_finalized'];
	$type = 'a';
	//if ($finalized) $type = 's';
	$out = "<h2>$course_name</h2>";
	$out .= "<h3>".l("Back to Dashboard", "centre/".arg(1))."</h3>";
	$modules = array( 'View Applications' => 'search-course/'.arg(1).'/'.arg(2), "Add Application" =>  'app/add/'.arg(1).'/'.arg(2), 'Assign Teacher' => 'assign-teacher/'.arg(1).'/'.arg(2),  'Day 0 List' => 'day0-list/'.arg(1).'/'.arg(2), 'Zero Day' => 'zero-day/'.arg(1).'/'.arg(2), 'Teachers List' => 'teacher-list/'.arg(1).'/'.arg(2), 'Manager List'  => 'manager-list/'.arg(1).'/'.arg(2), 'Cell List' => 'cell-list/'.arg(1).'/'.arg(2), 'Seating Plan' => 'seating/'.arg(1).'/'.arg(2), 'Laundry List' => 'laundry-list/'.arg(1).'/'.arg(2), 'Valuable List' => 'valuable-list/'.arg(1).'/'.arg(2), 'Student Chit' => 'student-chit/'.arg(1).'/'.arg(2) , 'Checking Slip' => 'checking-slip/'.arg(1).'/'.arg(2) , 'Course Summary Report' => 'report-day11/'.arg(1).'/'.arg(2), 'Generate Male PDF' => 'course-pdf-m/'.arg(1).'/'.arg(2), 'Generate Female PDF' => 'course-pdf-f/'.arg(1).'/'.arg(2)  );
	$out .= '<ul class="multi-column">';
	foreach($modules as $key => $val )
	{
		if ( $key == 'View Applications')
			$out .= '<h2><li>'.l($key, $val, array("query" => array("s" => '', 't' => '','g' => '', 'd' => $type))).'</li></h2>';
		elseif ( in_array($key, array('Cell List', 'Seating Plan'))  )
			$out .= '<h2><li>'.l($key, $val).' ('.l('Regenerate', $val, array("query" => array("r" => 1))).') </li></h2>';
		else
			if (drupal_valid_path($val))
				$out .= '<h2><li>'.l($key, $val).'</li></h2>';
	}
	$out .= '</ul><br />';
	$out .= '<h2>Course Summary</h2>';
	$finalized = db_query("select c_finalized from dh_course where c_id=".arg(2))->fetchField();
//	if ($finalized)
//		$out .= course_summary_finalized( arg(2) );
//	else
		$out .= course_summary( arg(1), "where c_id=".arg(2), array() );
	return $out;
}


function dh_manage_courses()
{
	$c_type = db_query("select c_subdomain, c_vri from dh_center where c_id=".arg(1))->fetchAssoc();
	drupal_add_library('system', 'ui.core');
	drupal_add_library('system', 'ui.dialog');
	drupal_add_library('system', 'ui.datepicker');
	/*
	  drupal_add_css("https://cdn.datatables.net/1.10.16/css/jquery.dataTables.min.css", array('type' => 'external'));
	  drupal_add_css("https://cdn.datatables.net/buttons/1.5.1/css/buttons.dataTables.min.css", array("type" => 'external'));
	  drupal_add_css("https://cdn.datatables.net/select/1.2.4/css/select.dataTables.min.css", array("type" => 'external'));
	
      drupal_add_css("/sites/all/libraries/editor/css/editor.dataTables.min.css");

	  drupal_add_js("https://cdn.datatables.net/1.10.16/js/jquery.dataTables.min.js", "external");
	  drupal_add_js("https://cdn.datatables.net/buttons/1.5.1/js/dataTables.buttons.min.js", "external");
	  drupal_add_js("https://cdn.datatables.net/select/1.2.4/js/dataTables.select.min.js", "external");
	
	  drupal_add_js("/sites/all/libraries/editor/js/dataTables.editor.min.js");
	*/

	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	drupal_add_js( libraries_get_path("editor").  "/js/moment.min.js");
	drupal_add_js( libraries_get_path("editor").  "/js/datetime-moment.js");
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/editor.select2.js');

	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');

	$js = '
		var editor;
		var ctypes = [];
		(function ($) {
			function load_editor()
			{
				editor = new $.fn.dataTable.Editor({ 
				    ajax: {url: "/course/handler/'.arg(1).'", type: "POST"},
					table: "#course-table",
				    fields: [
				        { name: "dh_course.c_course_type", label: "Course Type", type: "select", def: "6" },
				        { name: "dh_course.c_start", label:  "Start Date", type: "date", dateFormat:"dd M yy"  }, //dateFormat: $.datepicker.ISO_8601 
				        { name: "dh_course.c_end", label: "End Date", type: "date", dateFormat:"dd M yy" }, 
				        { name: "dh_course.c_cancelled", label: "Cancelled", type:"radio", options: [{label: "No", value: 0},  {label: "Yes", value:1}] },
				        { name: "dh_course.c_enrol_date", label: "Enrollment Date", type: "date", dateFormat: "dd M yy" },  
				        { name: "dh_course.c_status", label: "Course Status", type: "select", def: "Open" },
				        { name: "dh_course.c_status_nm", label: "Status New Male", type: "select", def: "Open" },
				        { name: "dh_course.c_status_om", label: "Status Old Male", type: "select", def: "Open" },
				        { name: "dh_course.c_status_nf", label: "Status New Female", type: "select", def: "Open" },
				        { name: "dh_course.c_status_of", label: "Status Old Female", type: "select", def: "Open" },
				        { name: "dh_course.c_status_svr_m", label: "Status Male Server", type: "select", def: "Open" },
				        { name: "dh_course.c_status_svr_f", label: "Status Female Server", type: "select", def: "Open" },
				        { name: "dh_course.c_list_only", label: "List Only", type: "radio", options: [{label: "No", value: 0},  {label: "Yes", value:1}] },
				        { name: "dh_course.c_date_change", label: "Date Change", type: "radio", options: [{label: "No", value: 0},  {label: "Yes", value:1}] },
				        { name: "dh_course.c_comments", label: "Comments", type: "textarea" },
				        { name: "dh_course.c_description", label: "Description", type: "textarea", fieldInfo: "If Description field is set, it will be displayed instead of default course-type (overrides course-type).<br>Please do not use it as comment." },
				        { name: "dh_course.c_at_m_count", label: "Male Teacher Count", type: "text", attr: {type: "number"}, def: "1" },
						{ name: "dh_course.c_at_f_count", label: "Female Teacher Count", type: "text", attr: {type: "number"}, def: "1" },
				        { name: "dh_course.c_combined_seat_course", label: "Combine Seating with Course", type: "select2", def: "0" },
                { name: "dh_course.c_form_langs", label: "Course Form Langs", type: "select2", "opts": {"multiple": true, "placeholder": "Select Form Langguages"}, fieldInfo: "Apply button in schedule.vridhamma.org will show multiple language (entered here) options for application forms (<b>in case application form type has translation present for entered languages</b>)<br>English will be present by default and need not to be selected.<br>This field is only applicable for schedule.vridhamma.org and will not have any impact on dhamma.org schedule/search."},
				        { name: "dh_course.c_deleted", type: "hidden" },
				        { name: "dh_course.c_finalized", type: "hidden" },
				    ],	
				 } );

				var table = $("#course-table").DataTable( {
				    ajax: "/course/handler/'.arg(1).'",
				    dom: "Bfrtip",
					columns: [
						{ data: "dh_course.c_id"  },
						{ data: "ctype.td_val1"  },
						{ data: "dh_course.c_start" },
						{ data: "dh_course.c_end" },
						{ data: null, render: function ( data, type, row ) {
							if (data.dh_course.c_cancelled == "1")
							  return "Yes"
							else
								return "No";
						} },
						{ data: "dh_course.c_status" },
						{ data: null, render: function ( data, type, row ) {
								//alert(JSON.stringify(data, null, 4));
								if ( data.dh_course.c_status_nm == null ) data.dh_course.c_status_nm = ""; 
								if ( data.dh_course.c_status_om == null ) data.dh_course.c_status_om = ""; 
								return data.dh_course.c_status_nm + "/" + data.dh_course.c_status_om;
						} },
						{ data: null, render: function ( data, type, row ) {
								if ( data.dh_course.c_status_nf == null ) data.dh_course.c_status_nf = ""; 
								if ( data.dh_course.c_status_of == null ) data.dh_course.c_status_of = ""; 
								return data.dh_course.c_status_nf + "/" + data.dh_course.c_status_of;
						} },
						{ data: "dh_course.c_comments" },
						{ data: "dh_course.c_description" },
						{ data: null, render: function ( data, type, row ) {
							if (data.dh_course.c_finalized == "1")
							    return "Yes (<a href=\"/course/'.arg(1).'/"+data.dh_course.c_id+"\">View</a>)"
							else
								return "No (<a href=\"/course/'.arg(1).'/"+data.dh_course.c_id+"\">View</a>)";
						} },
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
			 					var rowData = table.rows(".selected").data();
			 						
			 						var blkstr = [];
									$.each(rowData, function(key,val) {                    
									  var course_id=val.dh_course.c_id;
									  blkstr.push(course_id);
									});
									blkstr.join(",");


									var $URL="/app-student-count-cancel/"+blkstr;
							
				                	var request = $.ajax({
	            						url:$URL,
	            						type: "GET"
	        						});
	        						request.done(function (responseString) {
	        							console.log(responseString);
	        					
	           							var obj = responseString;
	           							if(obj.ApplicantFound == "Yes")
	           							{
											alert("Course has applications. Cannot be deleted");
			 								return;
	           							}
	           							else
	           							{
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
					                        .val( "dh_course.c_deleted", 1 );
					                    }
		           							
	           						});

							
			                }
			            },
    			        {
			                extend: "selected",
			                text: "Finalize Course",
			                action: function ( e, dt, node, config ) {
			                	if (table.rows(".selected").data().length > 1)
			                	{
			                		alert("Please select only one row");
			                		return;
			                	}
			                	else
			                	{
			                		var row = table.rows(".selected").data();
			                		var course_id=row[0].dh_course.c_id;
			                		var AllowFinalizedCourseFlag=false;
			                		
				                	var $URL="/app-student-count-finalize/"+course_id;


				                	var request = $.ajax({
	            						url:$URL,
	            						type: "GET"
	        						});
	        						request.done(function (responseString) {
	        							
	           							var obj = responseString;
	           							
	           							var r = confirm("PLEASE MAKE SURE COURSE IS COMPLETE AND LEFT STUDENTS ARE UPDATED BEFORE FINALIZING.\nAre you sure you want to finalize this course?");
			                			if ( r == true)
			                			{	
			                				if(obj.Count > 0)
			                				{

			                				editor.edit( table.row( { selected: true } ).index(), false )
			                					.set( "dh_course.c_finalized", "1" )
					                        	.submit();
			                				}
			                				else
			                				{
			                					alert("Course is not yet finished or Students who have attended course have not been updated. Please update via Zero Day section");
			                					return;
			                				}
			                			}
			                			
	           						});
			                	}
			              
			                }

			            }
				    ],
		            "order": [[ 2, "asc" ]]

				} );

			}

			$(document).ready(function(){
			    $.fn.dataTable.moment( "DD MMM YYYY" );	

				load_editor();
			});
		})(jQuery);

';
	

	drupal_add_js($js, 'inline');
	$code = "sch".$c_type['c_subdomain'];
	if ( strstr($c_type['c_subdomain'], '.in'))
		$code = "noncenter/".$c_type['c_subdomain'];
	

	$q = "select c_id, c_name as 'Name', c_finalized from dh_course where c_center = ".arg(1)." and c_finalized = 1 order by c_start";
	$res = db_query($q);
	$rows = array();
	while($r = $res->fetchAssoc())
	{
		$r['c_finalized'] = l("View", "course/".arg(1)."/".$r['c_id']);
		$rows[] = $r;
	}
	$header = array("Id", "Course Name", "Action");
	

	$out  = '<h2>'.l("Back to Dashboard", "centre/".arg(1));
	if ($c_type['c_vri'])
		$out .= '&nbsp;&nbsp;&nbsp;&nbsp;('.l("Course Schedule", "https://schedule.vridhamma.org/courses/".$c_type['c_subdomain'], array('attributes' => array("target" => "_blank") )).')</h2>';
	else
		$out .= '&nbsp;&nbsp;&nbsp;&nbsp;('.l("Course Schedule", "https://www.dhamma.org/en/schedules/".$code, array('attributes' => array("target" => "_blank") )).')</h2>';
	
	$finalized = theme("table", array('header' => $header, 'rows' => $rows ));
	$fieldset_vars = array(
		'element' => array(
		'#title' => t('Finalized Courses'),
		'#collapsible' => true,
		'#collapsed' => true,
		'#value' => '',
		'#children' => $finalized
		),
	);
	$out .= theme('fieldset', $fieldset_vars);

	$out .= '<table id="course-table" class="display">
				<thead>
					<tr>
						<th>Id</th>
						<th>Course Type</th>
						<th>Start Date</th>
						<th>End Date</th>
						<th>Cancel</th>
						<th>Status</th>
						<th>Status Male (N/O)</th>
						<th>Status Female (N/O)</th>
						<th>Comments</th>
						<th>Description<br>(Course Type)</th>
						<th>Finalized?</th>
					</tr>
				</thead>

	</table>';
	return $out;
}


function dh_manage_course_handler()
{
	global $user;
	$centre_id = arg(2);
	include_once "sites/all/libraries/editor/php/DataTables.php";
	$max_old_courses =  date('Y-m-d',strtotime("-".variable_get('max_old_courses', '6 month')));

/*use
    DataTables\Editor,
    DataTables\Editor\Field,
    DataTables\Editor\Format,
    DataTables\Editor\Mjoin,
    DataTables\Editor\Options,
    DataTables\Editor\Upload,
    DataTables\Editor\Validate;
*/
	//global $db;
	//var_dump($db);
//	_datatable_include();
	$editor = \DataTables\Editor::inst( $db, 'dh_course', 'c_id' ); 
    $editor->fields(\DataTables\Editor\Field::inst( 'dh_course.c_course_type' )
   	            ->options( \DataTables\Editor\Options::inst()
                ->table( 'dh_type_detail' )
                ->value( 'td_id' )
                ->label( 'td_val1' )
                ->where( function ($q) {
		            $q->where( 'td_type', 'COURSE-TYPE', '=' );
		            $q->where( 'td_active', '1', '=' );
        		}
            ))
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Please select a course Type' )
            ) ),
        \DataTables\Editor\Field::inst( 'ctype.td_val1' ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_start' )
	 	    ->getFormatter( 'Format::datetime', array(
					'from' => 'Y-m-d',
					'to' => 'd M Y'
					) )
	 	    ->setFormatter( 'Format::datetime', array(
					'from' => 'd M Y',
					'to' => 'Y-m-d'
					) )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Course Start date is required' )
            ) )
            ->validator( \DataTables\Editor\Validate::dateFormat('d M Y', \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Course Start date must be in dd Mmm yyyy (31 Jan 2021) format' ) ) ),
 	    \DataTables\Editor\Field::inst( 'dh_course.c_end' )
	 	    ->getFormatter( 'Format::datetime', array(
					'from' => 'Y-m-d',
					'to' => 'd M Y'
					) )
	 	    ->setFormatter( 'Format::datetime', array(
					'from' => 'd M Y',
					'to' => 'Y-m-d'
					) )
            ->validator( \DataTables\Editor\Validate::dateFormat('d M Y', \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Course End date must be in dd Mmm yyyy (31 Jan 2021) format' ) ) ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_cancelled' ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_finalized' ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_enrol_date' )
	 	    ->getFormatter( 'Format::datetime', array(
					'from' => 'Y-m-d',
					'to' => 'd M Y'
					) )
	 	    ->setFormatter( 'Format::datetime', array(
					'from' => 'd M Y',
					'to' => 'Y-m-d'
					) )
            ->validator( \DataTables\Editor\Validate::dateFormat('d M Y', \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Course Enrol date must be in dd Mmm yyyy (31 Jan 2021) format' ) ) ),
		\DataTables\Editor\Field::inst( 'dh_course.c_status' )
   	            ->options( /*function () {
			        return array(
			            array( 'value' => 'Open', 'label' => 'Open' ),
			            array( 'value' => 'FastFilling', 'label' => 'FastFilling' ),
			            array( 'value' => 'Completed', 'label' => 'Completed' ),
			            array( 'value' => 'Cancel', 'label' => 'Cancelled' ),
			            array( 'value' => 'Listonly', 'label' => 'Listonly' ),
			            array( 'value' => 'Full', 'label' => 'Full' ),
			        ); }*/
   	    		\DataTables\Editor\Options::inst() 
                ->table( 'dh_type_detail' )
                ->value( 'td_key' )
                ->label( 'td_key' )
                ->where( function ($q) {
		            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
		            $q->where( 'td_active', '1', '=' );
        		}
            ))
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Status is required' )
            ) ),
        \DataTables\Editor\Field::inst( 'dh_course.c_id' ),
        \DataTables\Editor\Field::inst( 'cstatus.td_val1' ),            
		\DataTables\Editor\Field::inst( 'dh_course.c_status_om' )
   	            ->options( 
	   	             \DataTables\Editor\Options::inst() 
	                ->table( 'dh_type_detail' )
	                ->value( 'td_key' )
	                ->label( 'td_key' )
	                ->where( function ($q) {
			            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
			            $q->where( 'td_active', '1', '=' );
        			})
         ),        
		\DataTables\Editor\Field::inst( 'dh_course.c_status_of' )
   	            ->options( 
	   	             \DataTables\Editor\Options::inst() 
	                ->table( 'dh_type_detail' )
	                ->value( 'td_key' )
	                ->label( 'td_key' )
	                ->where( function ($q) {
			            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
    		            $q->where( 'td_active', '1', '=' );
    			})
         ),        
		\DataTables\Editor\Field::inst( 'dh_course.c_status_nm' )
   	            ->options( 
	   	             \DataTables\Editor\Options::inst() 
	                ->table( 'dh_type_detail' )
	                ->value( 'td_key' )
	                ->label( 'td_key' )
	                ->where( function ($q) {
			            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
    		            $q->where( 'td_active', '1', '=' );
    			})
         ),        
		\DataTables\Editor\Field::inst( 'dh_course.c_status_nf' )
   	            ->options( 
	   	             \DataTables\Editor\Options::inst() 
	                ->table( 'dh_type_detail' )
	                ->value( 'td_key' )
	                ->label( 'td_key' )
	                ->where( function ($q) {
			            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
    		            $q->where( 'td_active', '1', '=' );
    			})
         ),        
		\DataTables\Editor\Field::inst( 'dh_course.c_status_svr_m' )
   	            ->options( 
	   	             \DataTables\Editor\Options::inst() 
	                ->table( 'dh_type_detail' )
	                ->value( 'td_key' )
	                ->label( 'td_key' )
	                ->where( function ($q) {
			            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
    		            $q->where( 'td_active', '1', '=' );
    			})
         ),        
		\DataTables\Editor\Field::inst( 'dh_course.c_status_svr_f' )
   	            ->options( 
	   	             \DataTables\Editor\Options::inst() 
	                ->table( 'dh_type_detail' )
	                ->value( 'td_key' )
	                ->label( 'td_key' )
	                ->where( function ($q) {
			            $q->where( 'td_type', 'ENROLL-STATUS', '=' );
    		            $q->where( 'td_active', '1', '=' );
    			})
         ),        
	    \DataTables\Editor\Field::inst( 'dh_course.c_list_only' ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_date_change' ),   	            
        \DataTables\Editor\Field::inst( 'dh_course.c_comments' ),            
        \DataTables\Editor\Field::inst( 'dh_course.c_description' ),
        \DataTables\Editor\Field::inst( 'dh_course.c_at_m_count' ),
        \DataTables\Editor\Field::inst( 'dh_course.c_at_f_count' ),
		\DataTables\Editor\Field::inst( 'dh_course.c_combined_seat_course' )
   	            ->options( function() {
					$now_minus_6month = date('Y-m-d',strtotime("-6 month"));
   	            	$a = db_query('select c_name as label, c_id as value from dh_course where c_center='.arg(2)." and c_start >= '".$now_minus_6month."' and c_deleted=0 order by c_start")->fetchAllKeyed();
   	            	$out = $a;
   	            	$out['None'] = "0";
        			return $out;
         		}),
              \DataTables\Editor\Field::inst( 'dh_course.c_form_langs' )
                ->options( \DataTables\Editor\Options::inst()
                ->table( 'dh_languages' )
                ->value( 'l_code' )
                ->label( 'l_name' )
                ->where( function ($q) {
                $q->where( 'l_code', 'en', '<>' );
            }
            ))
        ->getFormatter( function ( $data ) {
        return explode(',', $data);
        } )
          ->setFormatter( function ( $data ) {
                return implode(',', $data);
        } ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_deleted' ),   
	    \DataTables\Editor\Field::inst( 'dh_course.c_center' )
	    ->setValue( $centre_id ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_updated_by' )
	    ->setValue( $user->uid ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_created_by' )
	    ->set( \DataTables\Editor\Field::SET_CREATE )
	    ->setValue( $user->uid ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_updated' )
	    ->setValue( date('Y-m-d H:i:s') ),
	    \DataTables\Editor\Field::inst( 'dh_course.c_processed' )
	    ->setValue( 0 )
	    )
    ->leftJoin('dh_type_detail as ctype', 'ctype.td_id', '=', 'dh_course.c_course_type' ) 
    ->leftJoin('dh_type_detail as cstatus', 'cstatus.td_id', '=', 'dh_course.c_status' ) 
    ->pkey('c_id')
    ->on( 'preCreate', function ( $editor, $values ) { _course_set_default( $editor, 0, $values); } )
    ->on( 'preEdit', function ( $editor, $id, $values ) { _course_set_default( $editor, $id, $values); } )
    ->on( 'preRemove', function() { return false; })
    ->where( function ( $q ) {
	$q
	->where( 'c_deleted', 0)
	->and_where( 'c_finalized', 0)
        ->and_where( function ( $q ) {
	$q
	->where( 'c_cancelled', 1 )
        ->and_where( 'c_end', 'NOW()', '>=', false )
	->or_where( 'c_cancelled', 0 );
	});
	})
    ->where( 'c_center', $centre_id)
    ->process( $_POST )
    ->json();
//    ->where( 'c_start', $max_old_courses, '>=')    


//    ->on( 'preRemove', function ( $editor, $id, $values ) { db_update('dh_course')->fields( array('c_processed' => 0, 'c_deleted' => 1) )->condition('c_id', $id)->execute(); return false; } )

//	_get_datatable_json('dh_course', 'c_id', $fields);
	exit;
}

function _course_set_default( &$editor, $id, &$values)
{
//	file_put_contents("/tmp/lala2", print_r($values, true));
	$start = $values['dh_course']['c_start'] ;
	if ($values['dh_course']['c_end'] == '' || strtotime($values['dh_course']['c_end']) <  strtotime($start) )
	{
		$days = db_query("select td_val2 from dh_type_detail where td_id=".$values['dh_course']['c_course_type'])->fetchField() + 1;
		//if ( in_array($values['dh_course']['c_course_type'], array(9,11,41) ) )             //array('2 Day Course', '3 Day Course', 'STP') ) )
		if ($days < 10) // Less than 10 days is 1d, 2d, 3d, STP, for India it ends on same day. 
			$days -= 1;
		if ($days == 7)
			$days += 1;
		$end_date = date("d M Y", strtotime("+$days day", strtotime($start)) );
		if ($days == 1) $end_date = $start;
		//file_put_contents("/tmp/lalalalal", "$days - $end_date");
		$values['dh_course']['c_end'] = $end_date;
        $editor->field( 'dh_course.c_end' )->setValue( $end_date );
	}
	if ($values['dh_course']['c_enrol_date'] == '')
	{
		$enrol_date = date("d M Y", strtotime("-90 day", strtotime($start)) );
		$values['dh_course']['c_enrol_date'] = $enrol_date;
        $editor->field( 'dh_course.c_enrol_date' )->setValue( $enrol_date );			
	}

}


function course_summary( $centre_id, $where, $statuses )
{
	global $user;
	if (empty($statuses))
	{
		$q = "select td_key,td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' order by td_val2";
		$system_statuses = db_query($q)->fetchAllKeyed();

		$statuses = $system_statuses;
		$q = "select td_key from dh_type_detail where td_type='COURSE-STATUS' order by td_val2";
		$s_res = db_query($q);
		while ($r = $s_res->fetchAssoc()) $statuses[] = $r['td_key'];		


		if ( (arg(0) == 'course') && (  is_numeric(arg(1)) ) && (is_numeric(arg(2))) )
		{
			$q  = "select td_val3 from dh_course c left join dh_type_detail td on c.c_course_type=td.td_id where c.c_id=".arg(2);
			$c_data = db_query($q)->fetchAssoc();
			$is_lc = $c_data['td_val3']=='LC'?1:0;
			if ( $is_lc && (user_access('r-at review') || user_access('a-at review')))
			{
				$q = "select td_key, td_val1 from dh_type_detail where td_type='COURSE-LC-STATUS' order by td_val2";
				$s_res = db_query($q);
				while ($r = $s_res->fetchAssoc()) $statuses[] = $r['td_key'];									
			}
		}

	}

	$q = "select c.c_id,c.c_name,a_gender,a_status,a_old,a_type, count(a_id) as 'tot' from dh_applicant a left join dh_course c on (a_course=c.c_id) $where and c_deleted=0  group by c.c_id,c.c_name, a_gender,a_status, a_old, a_type order by c.c_start";

	$out = '';
	$next_month = db_query($q);
	$future_courses = array();
	while( $r = $next_month->fetchAssoc())
	{
		$future_courses[$r['c_name'].'||'.$r['c_id']][$r['a_status']][$r['a_gender']][$r['a_old']][strtolower($r['a_type'])] = $r['tot'];
	}
	
	$header = array('','NM','OM', 'Total', 'SM', '&nbsp;&nbsp;' , 'NF','OF', 'Total', 'SF');
	$gender = array('M', 'F');
	foreach( $future_courses as $course => $status_data )
	{
		$temp = explode("||", $course);
		$course_id = $temp[1];
		//$out .= '<div class="summary-block"><div class="table-heading">'.l($temp[0], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '','g' => ''))).'</div>';
		$out .= '<div class="summary-block"><div class="table-heading">'.l($temp[0], "course/".$centre_id."/".$course_id).'</div>';
		$rows = array();
		$totals_row = array(); $totals_row['M'] = $totals_row['F'] = array(); 
		$totals_row['M'][0] = $totals_row['F'][0] = $totals_row['M'][1] = $totals_row['F'][1] = 0; 
		$sevak['M'] = $sevak['F'] = 0;
		foreach( $statuses as $status ) 
		{
			$r = array();
			$r[] = l($status, "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '','g' => '')));
			foreach( $gender as $v )
			{
				$r[] = isset($status_data[$status][$v][0]['student'])?l($status_data[$status][$v][0]['student'],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '0','g' => $v, 'at' => 'a'))):"";
				$r[] = isset($status_data[$status][$v][1]['student'])?l($status_data[$status][$v][1]['student'],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '1','g' => $v, 'at' => 'a'))):"";
				$old = 0; $new = 0;
				if ( isset($status_data[$status][$v][0]['student']) ) $new = $status_data[$status][$v][0]['student'];
				if ( isset($status_data[$status][$v][1]['student']) ) $old = $status_data[$status][$v][1]['student'];
				$total = $old + $new; $tot = '';
				if ( $total > 0 ) 
				{
					$tot = "<b>".l($total, "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '','g' => $v)) )."</b>";
				}
				$totals_row[$v][0] += $new;
				$totals_row[$v][1] += $old;
				$r[] = $tot;
				if (!isset($sevak[$v]))
					$sevak[$v] = 0;
				$temp = 0;
				if (isset($status_data[$status][$v][0]['sevak']))
				{
					$sevak[$v] += $status_data[$status][$v][0]['sevak'];
					$temp += $status_data[$status][$v][0]['sevak'];
				}
				if (isset($status_data[$status][$v][1]['sevak']))
				{
					$sevak[$v] += $status_data[$status][$v][1]['sevak'];
					$temp += $status_data[$status][$v][1]['sevak'];					
				}
				$r[] = $temp?l($temp,"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '','g' => $v, 'at' => 's'))):'';
				if ($v == 'M')
					$r[] = '&nbsp;&nbsp;&nbsp;';
			}
			$rows[] = $r;
		}
		$r = array();
		$r[] = '<b>Total</b>'; 
		$r[] = l($totals_row['M'][0], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '0','g' => 'M')) ); 
		$r[] = l($totals_row['M'][1], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '1','g' => 'M')) ); 
		$r[] = "<b>".l($totals_row['M'][0] + $totals_row['M'][1], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '','g' => 'M')) )."</b>"; 

		$r[] = l($sevak['M'], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '','g' => 'M', 'at' => 's')) );
		$r[] = '&nbsp;&nbsp;&nbsp;'; 
		$r[] = l($totals_row['F'][0], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '0','g' => 'F')) ); 
		$r[] = l($totals_row['F'][1], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '1','g' => 'F')) ); 
		$r[] = "<b>".l($totals_row['F'][0] + $totals_row['F'][1], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '','g' => 'F')) )."</b>"; 
//		$r[] = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
		$r[] = l($sevak['F'], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '','g' => 'F', 'at' => 's')) );
		$rows[] = $r;
		$out .= theme('table', array('header' => $header, 'rows' => $rows));
		$out .= '</div>';
	}
	return $out;

}


function course_summary_finalized( $course_id )
{
	$q = "select td_key,td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statuses = db_query($q)->fetchAllKeyed();

	$statuses = $system_statuses;
	$q = "select td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$s_res = db_query($q);
	while ($r = $s_res->fetchAssoc()) $statuses[] = $r['td_key'];		

	$q = "select c.c_id,c.c_name,c_center, cs_status, cs_gender, cs_old, cs_count from dh_course_stat cs left join dh_course c on (cs_course=c.c_id) where cs_course=$course_id";

	$out = '';
	$result = db_query($q);
	$course_data = array();
	while( $r = $result->fetchAssoc())
	{
		$centre_id = $r['c_center'];
		$course_data[$r['c_name'].'||'.$r['c_id']][$r['cs_status']][$r['cs_gender']][$r['cs_old']] = $r['cs_count'];
	}
	
	$header = array('','NM','OM', 'Total', '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' , 'NF','OF', 'Total');
	$gender = array('M', 'F');
	foreach( $course_data as $course => $status_data )
	{
		$temp = explode("||", $course);
		$course_id = $temp[1];
		$out .= '<div class="summary-block"><div class="table-heading">'.l($temp[0], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => '', 't' => '','g' => '', 'd' => 's'))).'</div>';
		$rows = array();
		$totals_row = array(); $totals_row['M'] = $totals_row['F'] = array(); 
		$totals_row['M'][0] = $totals_row['F'][0] = $totals_row['M'][1] = $totals_row['F'][1] = 0; 
		foreach( $statuses as $status ) 
		{
			$r = array();
			//$r[] = l($status, "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '','g' => '')));
			$r[] = $status;
			foreach( $gender as $v )
			{
				if ( in_array($status, array($system_statuses['STATUS-ATTENDED'], $system_statuses['STATUS-LEFT'])) )
				{
					$r[] = isset($status_data[$status][$v][0])?l($status_data[$status][$v][0],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '0','g' => $v, 'd' => 's'))):"";
					$r[] = isset($status_data[$status][$v][1])?l($status_data[$status][$v][1],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '1','g' => $v, 'd' => 's'))):"";
				}
				else
				{
					$r[] = isset($status_data[$status][$v][0])?$status_data[$status][$v][0]:"";
					$r[] = isset($status_data[$status][$v][1])?$status_data[$status][$v][1]:"";
				}
				$old = 0; $new = 0;
				if ( isset($status_data[$status][$v][0]) ) $new = $status_data[$status][$v][0];
				if ( isset($status_data[$status][$v][1]) ) $old = $status_data[$status][$v][1];
				$total = $old + $new; $tot = '';
				if ( $total > 0 ) 
				{
					if ( in_array($status, array($system_statuses['STATUS-ATTENDED'], $system_statuses['STATUS-LEFT'])) )
						$tot = "<b>".l($total, "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $status, 't' => '','g' => $v, 'd' => 's')) )."</b>";
					else
						$tot = "<b>$total</b>"; 
				}
				$totals_row[$v][0] += $new;
				$totals_row[$v][1] += $old;
				$r[] = $tot;
				$r[] = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
			}
			$rows[] = $r;
		}
		$r = array();
		$r[] = '<b>Total</b>'; 
		$r[] = $totals_row['M'][0]; 
		$r[] = $totals_row['M'][1]; 
		$r[] = "<b>".($totals_row['M'][0] + $totals_row['M'][1])."</b>"; 

		$r[] = '&nbsp;'; 
		$r[] = $totals_row['F'][0]; 
		$r[] = $totals_row['F'][1]; 
		$r[] = "<b>".($totals_row['F'][0] + $totals_row['F'][1])."</b>"; 
		$r[] = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
		$rows[] = $r;
		$out .= theme('table', array('header' => $header, 'rows' => $rows));
		$out .= '</div>';
	}
	return $out;

}


function course_day11_report()
{
	$course_id = arg(2);
	$pdf = create_d11_report_pdf($course_id);
	header("Content-type: application/pdf");
	header("Content-Disposition: inline; filename=Day11-Report.pdf");
	//@readfile($pdf);
	$ret = s3_get_file(variable_get('s3_bucket_name', ''), $pdf, 'stream');
	if (!$ret['success'])
		watchdog('S3PDF', $ret['message']);
	print($ret['data']);
	exit();
}


function reminder_finalize()
{

	$body = 'Dear Registrar,

The Course [course-name] has not yet been finalized. 


Please update LEFT students and then finalize the course at the earliest.
Failure to finalize the course will result in loss of applicant information after 3 months. 


Thanks,
- DIPI Automation. 

	';
	$q = "select c.c_name, c.c_id, cs.cs_email_reply_to from dh_course c left join dh_center_setting cs on (c.c_center=cs.cs_center) left join dh_type_detail td on (c.c_course_type=td.td_id) where date_format(c.c_end, '%Y%m%d') < '".date("Ymd")."' and td.td_val2 > 1 and c.c_deleted=0 and c.c_finalized=0 and c.c_cancelled=0";
	$result = db_query($q);
	$mg = Mailgun::create(variable_get('mailgun_key', ''));
	while($r = $result->fetchAssoc())
	{
		$test_mode = variable_get('mode_test', '0');
		$to = $r['cs_email_reply_to'];
		if ( $test_mode )
			$to = variable_get('mode_test_emails', '');
		$options = array(
		  'from'    => 'DIPI Automation <noreply@vridhamma.org>',
		  'to'      => $to,  //info@dhamma.net.in
		  'subject' => 'DIPI Course Finalization Reminder',
		  'text'    => str_replace('[course-name]', $r['c_name'], $body) ,
		  'v:test-mode' => $test_mode
		);
    $mailgun_domain = get_mailgun_domain("noreply@vridhamma.org");
		$res = $mg->messages()->send($mailgun_domain, $options );

	}
}

function vri_reminder_finalize()
{

	$body = 'Dear VRI Admin,

This is a monthly report of centers who have not finalized their courses. You can view the report here - https://dipi.vipassana.co/patrika/center-report . Please make sure that centers finalize courses on time.

Thanks,
- DIPI Automation. 

	';
	$mg = Mailgun::create(variable_get('mailgun_key', ''));
	$test_mode = variable_get('mode_test', '0');
	if ( $test_mode )
		$to = variable_get('mode_test_emails', '');
	$options = array(
	  'from'    => 'DIPI Automation <noreply@vridhamma.org>',
	  'to'      => 'vri_admin@vridhamma.org',
	  'subject' => 'DIPI Monthly Center Report',
	  'text'    => $body,
	  'v:test-mode' => $test_mode
	);
  $mailgun_domain = get_mailgun_domain("noreply@vridhamma.org");
	$res = $mg->messages()->send($mailgun_domain, $options );
}

function finalize_course( $course_id )
{
	global $user;
	$res['status'] = false;
	$end_date = db_query("select c_end from dh_course where c_id=$course_id")->fetchField();
	if ( $end_date > date('Y-m-d') )
	{
		$res['msg'] = "Course not completed, cannot finalize";
		echo $res['msg'];
		return $res;
	}

	$q = "select td_key,td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statuses = db_query($q)->fetchAllKeyed();

	$statuses = $system_statuses;
	$q = "select td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$s_res = db_query($q);
	while ($r = $s_res->fetchAssoc()) $statuses[] = $r['td_key'];		

	$q = "update dh_applicant set a_status='".$system_statuses['STATUS-ATTENDED']."' where a_course='$course_id' and a_attended=1";
	db_query($q);

	$q = "select c.c_id,c.c_name,a_gender,a_status,a_old, count(a_id) as 'tot' from dh_applicant a left join dh_course c on (a_course=c.c_id) where c_id=$course_id  group by c.c_id,c.c_name, a_gender,a_status, a_old";

	$out = '';
	$course_data = db_query($q);
	while( $r = $course_data->fetchAssoc())
	{
		unset($row);
		$row['cs_course'] = $r['c_id'];
		$row['cs_old'] = $r['a_old'];
		$row['cs_gender'] = $r['a_gender'];
		$row['cs_status'] = $r['a_status'];
		$row['cs_count'] = $r['tot'];
		//$row['cs_created_by'] = $user->uid;
		db_insert('dh_course_stat')->fields($row)->execute();
	}

	/* Copy Applicant to Student DB */

	$q = "select ct_teacher, ct_group, t_gender from dh_course_teacher left join dh_teacher on ct_teacher=t_id where ct_course=$course_id";
	$result = db_query($q);
	$teacher['M'] = $teacher['F'] = array();
	while($r = $result->fetchAssoc())
		$teacher[$r['t_gender']][$r['ct_group']] = $r['ct_teacher'];

	$result = db_query("desc dh_student");
	$fields = array();
	while($r = $result->fetchAssoc())
	{
		if (! in_array($r['Field'], array('s_id', 's_updated', 's_created', 's_created_by', 's_updated_by', 's_first')) )
		   $fields[]  = $r['Field'];
	}

	if (posix_getuid() == 0)
		posix_setuid(33);

	$q = "select * from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_applicant_extra ae on a_id=ae_applicant where a_course='$course_id' and a_attended=1 and a_type IN ('Student','Sevak')";
	$result = db_query($q);
	while( $r = $result->fetchAssoc() )
	{
		try
		{

			unset($student); unset($student_sci); unset($student_c);
			$sid_sci = $sid_c = '';
			$sid = check_student($r);
			if ($sid > 0)
			{
				//$sid_sci = db_query("select sci_id from dh_student_course_input where sci_student=$sid")->fetchField();
				//$sid_c = db_query("select sc_id from dh_student_course where sc_student=$sid")->fetchField();			
			}

			foreach($fields as $v)
				$student[$v] = $r[str_replace('s_', 'a_', $v)];
			if ( ! $r['a_old'] )
				$student['s_first'] = 1;
			$student['s_created_by'] = $user->uid;
			$student['s_updated_by'] = $user->uid;
			$student['s_updated'] = date('Y-m-d H:i:s');

			if ($student['s_photo'] == '')
				unset($student['s_photo']);

			if ($sid > 0 )
			{
				db_update('dh_student')->fields($student)->condition('s_id', $sid)->execute();

			}
			else
				$sid = db_insert('dh_student')->fields($student)->execute();

			if ( isset($student['s_photo']))
			{
				//Copy photo to final dir
				$path = drupal_realpath('private://');
				$old_photo = $path.'/'. str_replace("private://", "", $student['s_photo']);
				$temp = 'final/photo-id/'.$r['a_center'].'/'.$r['a_course'].'/'.sprintf("%08d", $sid).".jpg";
				$new_photo = $path.'/'.$temp;
				$new_photo_uri = 'private://'.$temp;
				//$student['s_photo'] = $new_photo_uri;
				db_update('dh_student')->fields(array('s_photo' => $new_photo_uri))->condition('s_id', $sid)->execute();
				/*if (!is_dir(dirname($new_photo)))
					mkdir(dirname($new_photo), 0775, true);		
				copy($old_photo, $new_photo);
				*/
				$ret = s3_get_file(variable_get('s3_bucket_name', ''), str_replace("private://", "", $student['s_photo']), 'file' );
				if (!$ret['success'])
					watchdog('S3Photo', $ret['message']);

				$ret = s3_put_file(variable_get('s3_bucket_name', ''), $ret['data'], $temp  );
				if (!$ret['success'])
					watchdog('S3Photo', $ret['message']);
			}


			$student_sci['sci_10d'] = $r['ac_10d']?$r['ac_10d']:'1';
			$student_sci['sci_stp'] = $r['ac_stp']?$r['ac_stp']:'0';
			$student_sci['sci_spl'] = $r['ac_spl']?$r['ac_spl']:'0';
			$student_sci['sci_20d'] = $r['ac_20d']?$r['ac_20d']:'0';
			$student_sci['sci_30d'] = $r['ac_30d']?$r['ac_30d']:'0';
			$student_sci['sci_45d'] = $r['ac_45d']?$r['ac_45d']:'0';
			$student_sci['sci_60d'] = $r['ac_60d']?$r['ac_60d']:'0';
			$student_sci['sci_tsc'] = $r['ac_tsc']?$r['ac_tsc']:'0';
			$student_sci['sci_teacher'] = $r['ac_teacher']?$r['ac_teacher']:'0';
			$student_sci['sci_service'] = $r['ac_service']?$r['ac_service']:'0';
			$student_sci['sci_first_year'] = $r['ac_first_year'];
			$student_sci['sci_first_month'] = $r['ac_first_month'];
			$student_sci['sci_first_day'] = $r['ac_first_day'];
			$student_sci['sci_first_location_str'] = $r['ac_first_teacher_str'];
			$student_sci['sci_first_teacher_str'] = $r['ac_first_teacher_str'];

			if ($sid_sci > 0 )
				db_update('dh_student_course_input')->fields($student_sci)->condition('sci_id', $sid_sci)->execute();
			else
			{
				$student_sci['sci_student'] = $sid;
				db_insert('dh_student_course_input')->fields($student_sci)->execute();
			}

			//$student_c['sc_student'] = ;
			$student_c['sc_center'] = $r['a_center'];
			$student_c['sc_course'] = $r['a_course'];
			$student_c['sc_teacher'] = $teacher[$r['a_gender']][$r['aa_group']];
			$student_c['sc_group'] = $r['aa_group'];
			$student_c['sc_acco'] = $r['aa_section'].'-'.$r['aa_acco'];
			$student_c['sc_laundry'] = $r['aa_laundry'];
			$student_c['sc_valuable'] = $r['aa_valuable'];
			$student_c['sc_cell'] = $r['aa_cell'];
			$student_c['sc_chowky'] = $r['aa_chowky'];
			$student_c['sc_chair'] = $r['aa_chair'];
			$student_c['sc_seat_row'] = $r['aa_seat_row'];
			$student_c['sc_seat_col'] = $r['aa_seat_col'];
			$student_c['sc_comment'] = $r['aa_comment'];
			$student_c['sc_desc_physical'] = $r['ae_desc_physical'];
			$student_c['sc_desc_mental'] = $r['ae_desc_mental'];
			$student_c['sc_addiction_past'] = $r['ae_desc_addiction_past'];
			$student_c['sc_addiction_current'] = $r['ae_desc_addiction_current'];
			$student_c['sc_desc_other_technique'] = $r['ae_desc_other_technique'];
			$student_c['sc_desc_medication'] = $r['ae_desc_medication'];
			$student_c['sc_teach_other_details'] = $r['ae_teach_other_details'];
			if ( $r['a_uri'] <> '')
			{
				$path = drupal_realpath('private://');
				$app_pdf = $path.'/'.str_replace("private://", "", $r['a_uri']);
				$temp = 'final/applications/'.$r['a_center'].'/'.$r['a_course'].'/'.sprintf("%08d", $sid).'-'.sprintf("%06d", $r['a_course']).".pdf";
				$new_pdf = $path.'/'.$temp;
				$new_uri = 'private://'.$temp;
				$student_c['sc_uri'] = $new_uri;
				/*
				if (!is_dir(dirname($new_pdf)))
					mkdir(dirname($new_pdf), 0775, true);		
				copy($app_pdf, $new_pdf);
				*/
				$ret = s3_get_file(variable_get('s3_bucket_name', ''), str_replace("private:///", "", $r['a_uri']), 'file' );
				if (!$ret['success'])
					watchdog('S3Photo', $ret['message']);

				$ret = s3_put_file(variable_get('s3_bucket_name', ''), $ret['data'], $temp  );
				if (!$ret['success'])
					watchdog('S3Photo', $ret['message']);

			}

			if ($sid_c > 0 )
				db_update('dh_student_course')->fields($student_c)->condition('sc_id', $sid_c)->execute();
			else
			{
				$student_c['sc_student'] = $sid;
				db_insert('dh_student_course')->fields($student_c)->execute();

			}
			dh_send_letter('applicant', $r['a_id'], $system_statuses['STATUS-ATTENDED'] );

		}
		catch(Throwable $t)
		{
			watchdog("Finalization Error", $t->getTraceAsString());
		}

	}
	$res['status'] = true;
	$res['msg'] = 'OK';
	return $res;		
}


function check_student($r)
{
	$sid = '';
	if ($r['a_aadhar'] <> '')
		$sid = db_query("select s_id from dh_student where s_aadhar='".$r['a_aadhar']."'")->fetchField();
	if ($sid > 0)  return $sid; 

	if ($r['a_pancard'] <> '')
		$sid = db_query("select s_id from dh_student where s_pancard='".$r['a_pancard']."'")->fetchField();
	if ($sid > 0 ) return $sid;

	if ($r['a_voter_id'] <> '')
		$sid = db_query("select s_id from dh_student where s_voter_id='".$r['a_voter_id']."'")->fetchField();
	if ($sid > 0 ) return $sid;

	if ($r['a_passport'] <> '')
		$sid = db_query("select s_id from dh_student where s_passport='".$r['a_passport']."'")->fetchField();
	if ($sid > 0 ) return $sid;

	if ($r['a_email'] <> '')
		$sid = db_query("select s_id from dh_student where s_f_name='".$r['a_f_name']."' and  s_l_name='".$r['a_l_name']."' and s_email='".$r['a_email']."'")->fetchField();
	if ($sid > 0 ) return $sid;

	if ($r['a_phone_mobile'] <> '')
		$sid = db_query("select s_id from dh_student where s_f_name='".$r['a_f_name']."' and  s_l_name='".$r['a_l_name']."' and s_phone_mobile='".$r['a_phone_mobile']."'")->fetchField();
	if ($sid > 0 ) return $sid;


	if ($r['a_dob'] <> '')
		$sid = db_query("select s_id from dh_student where s_f_name='".$r['a_f_name']."' and  s_l_name='".$r['a_l_name']."' and s_dob='".$r['a_dob']."'")->fetchField();
	if ($sid > 0 ) return $sid;

	return $sid;
}

function generate_course_pdf($course_id, $type = 'M')
{
	watchdog('DIPI', 'generate_course_pdf called', array(), WATCHDOG_NOTICE, 'link');
	$q = "select a_id, a_uri from dh_applicant a left join dh_applicant_course ac on (a.a_id=ac.ac_applicant) left join dh_applicant_attended aa on (a.a_id=aa.aa_applicant) left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course=$course_id and a_attended = 1 and a_gender='$type' order by aa_group asc, a_type desc, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y")."), IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	$pdfs = array();
	$i = 0;
	$meta_data = array();
	$meta_data[$type] = 0;
	while($r = $result->fetchAssoc())
	{
		$meta_data[$type]++;
		try {
			create_application_pdf($r['a_id']);	
		} catch (Exception $e) {
			
		}

		if ($r['a_uri'] <> '')
		{	
			/*		
			if ($i == 0)
			{
				$path = drupal_realpath($r['a_uri']);
				$dirname = dirname($path);
			}
			$path_t = drupal_realpath($r['a_uri']);
			$dirname_t = dirname($path_t);
			if ($dirname_t == $dirname)
				$pdfs[] =  basename($r['a_uri']);
			else
				$pdfs[] =  $dirname_t."/".basename($r['a_uri']);
			*/
			$ret = s3_get_file( variable_get('s3_bucket_name', ''), str_replace("private:///", '', $r['a_uri']), 'file', 'pdf' );
			if (!$ret['success'])
				watchdog('S3PDF', $ret['message']);
			$pdfs[] = $ret['data'];
			$i++;
		}
	}
	$generate = True;
	$q = "select j_data from dh_json where j_identifier = $course_id";
	$res = db_query($q);
	if ($res)
	{
		$tmp = $res->fetchAssoc();
		$json = unserialize($tmp['j_data']);
		if ($json)
		{
			if (isset($json[$type]))
			{
				$total_count = $json[$type];
				if ($total_count <>  $meta_data[$type])
				{
					$json[$type] = $meta_data[$type];
					$upd = array('j_data' => serialize($json));
					db_update('dh_json')->fields($upd)->condition("j_identifier", $course_id)->execute();
				}
				else
					$generate = False;
			}
		}
	}
	if ( count($pdfs) > 0 )
	{
		if ($generate)
		{
			$cur_dir = getcwd();
			//echo "Dir: $dirname\n";
			//print_r($pdfs);
			//chdir($dirname);
			$out_pdf = tempnam("/var/dipi", "mega-").".pdf";
			$cmd = "/usr/bin/pdfjoin ".implode(" " , $pdfs)." --outfile $out_pdf";
			//echo "$cmd\n";
			watchdog('DIPI', 'generate_course_pdf cmd '.$cmd, array(), WATCHDOG_NOTICE, 'link');
			exec($cmd);
			$s3_uri = "applications/".arg(1)."/".$course_id."/course-$type.pdf";
			s3_put_file(variable_get('s3_bucket_name', ''), $out_pdf, $s3_uri);
			foreach ($pdf as $f) {
				unlink($f);
			}
			unlink($out_pdf);
			//chdir($cur_dir);
			if ( isset($json) && (is_array($json)) && (count($json) > 0))
			{
				$json[$type] = $meta_data[$type];
				$upd = array('j_data' => serialize($json));
				db_update('dh_json')->fields($upd)->condition("j_identifier", $course_id)->execute();
			}
			else
			{
				$ins = array('j_data' => serialize($meta_data), 'j_identifier' => $course_id);
				db_insert('dh_json')->fields($ins)->execute();				
			}			
		}
	}
	else
	{
		echo "No pdf applications found\n";
	}
}

function course_pdf()
{
	$course_id = arg(2);
	if (arg(0) == 'course-pdf-m') {
		$type = 'M';
		$pdf = generate_course_pdf($course_id);
	}
	else {
		$type = 'F';
		$pdf = generate_course_pdf($course_id, $type);
	}
	header("Content-type: application/pdf");
	header("Content-Disposition: inline; filename=course-".$type.".pdf");
	$uri = "applications/".arg(1)."/".arg(2)."/course-".$type.".pdf";
	$ret = s3_get_file(variable_get('s3_bucket_name', ''), $uri, 'stream' );
	if (!$ret['success'])
		watchdog('S3PDF', $ret['message']);
	print($ret['data']);
	exit();
	//@readfile($uri."/applications/".arg(1)."/".arg(2)."/course-".$type.".pdf");
}



function dh_course_status_check( $center_id, $course_type, $gender, $course_id )
{
	//watchdog( "COURSE-STATUS", "Centre: $center_id, Course Id: $course_id, Gender: $gender");
	$settings = db_query("select cs_course_config from dh_center_setting where cs_center=$center_id")->fetchField();
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);

	$g = ($gender == 'M')?"Male":"Female";
	//$count_full = 0; $count_wait = 0;
	$count_ini = array('full' => array('all' => 0, 'Old' => 0, 'New' => 0, 'Server' => 0), 
		'wait' => array('all' => 0, 'Old' => 0, 'New' => 0, 'Server' => 0));
	if ( is_array($ini[$course_type]) )
	{
		foreach( array('MaxApps-', 'Waitlist-') as $cc )
		{
			$key = $cc.$g;
			if ( isset($ini[$course_type][$key]) && (trim($ini[$course_type][$key]) <> '') )
			  $count_ini[($cc == 'MaxApps-'?'full':'wait')]['all'] = trim($ini[$course_type][$key]);			
		}
		//if ( isset($ini[$course_type]['Waitlist-'.$g]) && (trim($ini[$course_type]['Waitlist-'.$g]) <> '') )
		//  $count_wait['all'] = trim($ini[$course_type]['Waitlist-'.$g]);			
		foreach( array('Old', 'New', 'Server') as $a_type )
		{
			foreach( array('MaxApps-', 'Waitlist-') as $cc )
			{
				$key = $cc.$g.'-'.$a_type;
				if ( isset($ini[$course_type][$key]) && (trim($ini[$course_type][$key]) <> '') )
				  $count_ini[($cc == 'MaxApps-'?'full':'wait')][$a_type] = trim($ini[$course_type][$key]);

			}
			//if ( isset($ini[$course_type]['Waitlist-'.$g.'-'.$a_type]) && (trim($ini[$course_type]['Waitlist-'.$g]) <> '') )
			// $count_wait[$a_type] = trim($ini[$course_type]['Waitlist-'.$g]);			
		}
	}
	//watchdog( "COURSE-STATUS", "Centre: $center_id, Course Id: $course_id, Gender: $gender, Count Full: $count_full");
	//if ( ( $count_full > 0) || ($count_wait > 0))
	if ( ($count_ini['full']['all'] > 0 || $count_ini['full']['Old'] > 0 || $count_ini['full']['New'] > 0 || $count_ini['full']['Server'] > 0 ) || ($count_ini['wait']['all'] > 0 || $count_ini['wait']['Old'] > 0 || $count_ini['wait']['New'] > 0 || $count_ini['wait']['Server'] > 0) )
	{
		$SYSTEM_USER_ID = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();
		$count_row = db_query("select a_type, a_old, count(a_id) as 'c' from dh_applicant where a_center=$center_id and a_course=$course_id and a_gender='$gender' and a_status not in ('Rejected', 'Cancelled') group by a_type, a_old" );
		$count_cur = array('all' => 0, 'Old' => 0, 'New' => 0, 'Server' => 0);
		$c = 0;
		while( $r = $count_row->fetchAssoc() )
		{
			if ( strtolower($r['a_type']) == 'student')
				$key = ($r['a_old']?'Old':'New');
			elseif ( strtolower($r['a_type']) == 'sevak' )
				$key = 'Server';
			$count_cur[$key] += $r['c'];
			$c += $r['c'];
		}
		$count_cur['all'] = $c;

		// If old, new, server counts are there, then they get preference and all is ignored. 
		if (( $count_ini['Old'] + $count_ini['New'] + $count_ini['Server'] ) > 0 )
			unset($count_ini['all']);
		else
		{
			unset($count_ini['Old']); 
			unset($count_ini['New']);
			unset($count_ini['Server']);
		}

		$full_done = array('all' => False, 'Old' => False, 'New' => False, 'Server' => False);
		foreach( $count_ini as $c_status => $c_status_data )
		{
			foreach( $c_status_data as $key => $val  )
			{
				if ( isset($count_cur[$key]) && ( $count_cur[$key] >= $val ) && ( $val > 0) && ($full_done[$key] == False) )
				{
          $full_done[$key] = True;
          $new_status = ($c_status == 'full'? 'Course Full':'Wait List');
          $current_status ="";

          if ($key == 'all')
            $q = "select c_status_o".strtolower($gender)." from dh_course where c_id=$course_id";
          else
            $q = "select c_status_".( ($key == 'Server'?'svr_':($key == 'Old'?'o':'n')) ).strtolower($gender)." from dh_course where c_id=$course_id";
          $current_status = db_query($q)->fetchField();

          $flag = true;

          if ($current_status == 'Course Full')
            $flag = false;

          if ($current_status == 'Wait List' && $new_status == 'Wait List')
            $flag = false;

          if($flag)
          {
            //watchdog( "COURSE-STATUS", "Centre: $center_id, Course Id: $course_id, Gender: $gender, Type: $key, Count: $val, Count $c_status: ".$count_cur[$key] );
            $fields = array();
            $fields['l_user'] = 15;
            $fields['l_center'] = $center_id;
            $fields['l_msg'] = "Gender: $gender, Type: $key, ConfigCount: $val, Count $c_status: ".$count_cur[$key];
            $fields['l_module'] = 'dh_course';
            $fields['l_event'] = "AUTO-COURSE-STATUS";
            $fields['l_identifier'] = $course_id;
            db_insert('dh_log')->fields($fields)->execute();

            if ($key == 'all')
              $q = "update dh_course set c_status_o".strtolower($gender)." = '$new_status', c_status_n".strtolower($gender)." = '$new_status',
                  c_processed=0, c_updated='".date("Y-m-d H:i:s")."', c_updated_by='$SYSTEM_USER_ID' where c_id=$course_id";
            else
              $q = "update dh_course set c_status_".( ($key == 'Server'?'svr_':($key == 'Old'?'o':'n')) ).strtolower($gender)." = '$new_status',
                  c_processed=0, c_updated='".date("Y-m-d H:i:s")."', c_updated_by='$SYSTEM_USER_ID' where c_id=$course_id";
            db_query($q);
          }

				}
			}
		}
		/*

		if (( $count >= $count_full ) && ($count_full > 0 ))
		{
			watchdog( "COURSE-STATUS", "Centre: $center_id, Course Id: $course_id, Gender: $gender, Count: $count, Count Full: $count_full");
			$q = "update dh_course set c_status_o".strtolower($gender)." = 'Course Full', c_status_n".strtolower($gender)." = 'Course Full', 
			  c_processed=0, c_updated='".date("Y-m-d H:i:s")."', c_updated_by='$SYSTEM_USER_ID' where c_id=$course_id";
			db_query($q);
		}
		elseif ( $count > $count_wait )
		{
			if ($count_wait > 0 )
			{
				watchdog( "COURSE-STATUS","Centre: $center_id, Course Id: $course_id, Gender: $gender, Count: $count, Count Wait: $count_wait");
				$q = "update dh_course set c_status_o".strtolower($gender)." = 'Wait List', c_status_n".strtolower($gender)." = 'Wait List', 
				  c_processed=0, c_updated='".date("Y-m-d H:i:s")."', c_updated_by='$SYSTEM_USER_ID' where c_id=$course_id";
				db_query($q);
			}
		}
		*/
	}
}


function applicant_should_be_waitlisted( $centre, $course, $type, $gender, $old, $app_id )
{
   	$update_waitlist = db_query("select cs_apps_waitlist from dh_center_setting where cs_center=$centre")->fetchField();
   	if ($update_waitlist)
   	{
	 	$course_status = db_query("select c_status_".( (strtolower($type) == 'sevak'?'svr_':($old?'o':'n')) ).strtolower($gender)." from dh_course where c_id=".$course)->fetchField();
	 	if ( in_array($course_status, array('Wait List', 'Course Full')) )
	 		return true;
	}
	return false;

}
