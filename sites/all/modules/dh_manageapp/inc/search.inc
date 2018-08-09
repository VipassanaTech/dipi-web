<?php

function dh_manageapp_search_results( $type, $where, $centre )
{
	global $user;
  	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];
	//drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/datatables.min.js' );
	//drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/datatables.min.css' );
	$iPod    = stripos($_SERVER['HTTP_USER_AGENT'],"iPod");
	$iPhone  = stripos($_SERVER['HTTP_USER_AGENT'],"iPhone");
	$iPad    = stripos($_SERVER['HTTP_USER_AGENT'],"iPad");
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	if ( !($iPad || $iPhone || $iPod) )
	{
		//drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.html5.min.js");
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.print.min.js");
	}

	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );
	$q = "select td_key, td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statues = db_query($q)->fetchAllKeyed();

	$q = "select td_key, td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$statuses = db_query($q)->fetchAllKeyed();
	$statuses[''] = 'Choose';
	$statuses[$system_statues['STATUS-CONFIRMED']] = $system_statues['STATUS-CONFIRMED'];
	$statuses[$system_statues['STATUS-CANCELLED']] = $system_statues['STATUS-CANCELLED'];
	//$statuses[$system_statues['STATUS-ATTENDED']] = $system_statues['STATUS-ATTENDED'];
	$statuses[$system_statues['STATUS-CUSTOM']] = $system_statues['STATUS-CUSTOM'];

	$letters = array();
	$q = "select l_id, l_event, l_name from dh_letter where l_center=$centre and l_deleted=0 order by l_event, l_name";
	$letter_res = db_query($q);
	while( $l = $letter_res->fetchAssoc())
	{
		$letters[$l['l_event']][] = array('id' => $l['l_id'], 'status' => $l['l_name']);
	}
	$letters_json = json_encode($letters);

	$courses = array(); $today = date('Y-m-d',strtotime("-1 month"));
	$q = "select c_id, c_name from dh_course where c_center=$centre and c_start >= '$today' and c_deleted=0 order by c_start";
	$courses = db_query($q)->fetchAllKeyed();
	$courses_j = array();
	foreach( $courses as $k => $v)
		$courses_j[] = array('id' => $k, 'name' => $v);
	$courses_json = json_encode($courses_j);

	$pagination = db_query("select cs_pagination from dh_center_setting where cs_center=$centre")->fetchField();
	if ($pagination == '')
		$pagination = 'bottom';
	switch ($pagination) {
		case 'top':
			$page_dom = 'lBfr<\"clear\"p>ti';
			break;
		case 'both':
			$page_dom = 'lBfr<\"clear\"p>tip';
			break;
		default:
			$page_dom = 'lBfrtip';
			break;
	}

	if ( $type == 'applicant')
		$q = "select c.c_finalized, a_id, a_type, a_course, a_conf_no as 'ConfNo', concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', a_gender, a_status, ci.c_name as 'City', s.s_name, co.c_name as 'Country', a_city_str, c.c_name, a_address, a_zip, a_email, a_phone_home, a_phone_office, a_phone_mobile, a_education, a_occupation, a_occupation_past, a_company, a_designation, a_department, a_alist, a_uri, a_photo, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_lang_1,a_lang_2, a_lang_3, a_langs, a_old, a_attended, a_internal_note, a_willing_to_serve, a_extra, c.c_end, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher,ac_service, CONCAT( IFNULL(ac_first_year, ''),'-',IFNULL(ac_first_month, ''),'-',IFNULL(ac_first_day, ''), ', ', ac_first_location_str) as 'first_course', CONCAT( IFNULL(ac_last_year,''),'-',IFNULL(ac_last_month,''),'-',IFNULL(ac_last_day, ''), ', ', ac_last_location_str) as 'last_course', ac_practice_details,  ae_desc_physical,  ae_desc_medication,  ae_desc_mental, ae_desc_other_technique, ae_desc_addiction_current, ae_pregnant, ae_pregnant_detail from dh_applicant left join dh_applicant_course ac on (a_id=ac_applicant) left join dh_applicant_extra ae on (a_id=ae_applicant) left join dh_course c on (a_course=c.c_id) left join dh_country co on (a_country=co.c_code) left join dh_city ci on a_city=ci.c_id left join dh_state s on (a_state=s.s_code and a_country=s.s_country)  where $where order by a_created,a_vray_id";
	else
		$q = "select c.c_finalized, s.s_id as 'a_id', 'Student' as 'a_type', sc_course as 'a_course', sc_conf_no as 'ConfNo', concat(s_f_name, ' ',s_m_name, ' ',s_l_name) as 'Full Name', s_gender as 'a_gender', 'Attended' as 'a_status', ci.c_name as 'City', st.s_name, co.c_name as 'Country', '' as 'a_city_str', c.c_name, s_address as 'a_address', s_zip as 'a_zip', s_email as  'a_email', s_phone_home as  'a_phone_home',s_phone_office as 'a_phone_office',s_phone_mobile as 'a_phone_mobile',s_education as 'a_education',s_occupation as 'a_occupation',s_occupation_past as 'a_occupation_past',s_company as 'a_company', s_designation as 'a_designation', s_department as 'a_department',s_alist as  'a_alist', '' as 'a_uri', s_photo as 'a_photo', TIMESTAMPDIFF( YEAR, s_dob, CURDATE() ) as 'Age', s_lang_1 as 'a_lang_1', s_lang_2 as 'a_lang_2', s_lang_3 as  'a_lang_3', s_langs as 'a_langs', 1 as 'a_old', 1 as 'a_attended', c.c_end, sci_10d as 'ac_10d', sci_stp as 'ac_stp', sci_spl as 'ac_spl', sci_20d as  'ac_20d',sci_30d as  'ac_30d', sci_45d as  'ac_45d', sci_60d as 'ac_60d', sci_tsc as 'ac_tsc', sci_teacher as 'ac_teacher', CONCAT( IFNULL(sci_first_year, ''),'-',IFNULL(sci_first_month, ''),'-',IFNULL(sci_first_day, ''), ', ', sci_first_location_str) as 'first_course', '' as 'ac_practice_details', sc_desc_physical as 'ae_desc_physical', sc_desc_medication  as 'ae_desc_medication',  sc_desc_mental as 'ae_desc_mental', sc_desc_other_technique as 'ae_desc_other_technique', sc_addiction_current 'ae_desc_addiction_current', '' as 'ae_pregnant', '' as 'ae_pregnant_detail', sc_uri as 'a_uri' from dh_student s left join dh_student_course sc on (s.s_id=sc_student) left join dh_student_course_input sci on (s.s_id=sci_student) left join dh_course c on (sc_course=c.c_id) left join dh_country co on (s.s_country=co.c_code) left join dh_city ci on s_city=ci.c_id left join dh_state st on (s.s_state=st.s_code and s.s_country=st.s_country)  where $where order by s_f_name";

	//c.c_name as 'last_course'
	//drupal_set_message($q);
//and a_status='Confirmed' 
	if ($user->uid == 1)
	{
		drupal_set_message($q);
	}
	$result = db_query($q);
	$rows = array();
	while( $r = $result->fetchAssoc() )
	{
		unset($rs);
		if (!isset($r['last_course']))
			$r['last_course'] = '';
		$pdf_link = '';
		$rs['allowtransfer'] = 1;
		$rs['type'] = strtolower($r['a_type']);
		$rs['courseid'] = $r['a_course'];
		$rs['finalized'] = $r['c_finalized']?1:0;
		if ( $r['a_attended'] || ($r['a_status'] == $system_statues['STATUS-LEFT'] ))
			$rs['allowtransfer'] = 0;
		if (date('Y-m-d') > $r['c_end'])
			$rs['allowtransfer'] = 0;
		if ( $r['a_uri'] <> '' )
			$pdf_link = '&nbsp;('.l("PDF", file_create_url($r['a_uri']), array('attributes' => array("target" => "_blank") ) ).')';
		$rs['aid'] = $r['a_id'];
		$rs['changestatus'] = '' ;
		$rs['name'] = _ma_make_link($r['Full Name'],'app/'.$r['a_id'].'/edit').$pdf_link;
		if ($rs['type'] == 'sevak')
			$rs['name'] .= ' (Sevak)';
		if ($r['ac_teacher'] == 1)
			$rs['name'] .= ' (AT)';
		if ( in_array($r['ConfNo'], array('NF', 'OF', 'NF0', 'OF0')) )
			$r['ConfNo'] = '';
		$rs['course'] = $r['c_name'];
		$rs['status'] = $r['a_status'];
		$rs['confno'] = $r['ConfNo'];
		$rs['gender'] = (strtoupper($r['a_gender'])=='M')?'Male':'Female';
		if ($r['City'] == '')
			$r['City'] = $r['a_city_str'];
		$rs['location'] = $r['Country'].', '.$r['s_name'].', '.$r['City'];
		$rs['city'] = $r['City'];
		$rs['state'] = $r['s_name'];
		$rs['country'] = $r['Country'];
		$rs['pin'] = $r['a_zip'];
		$rs['age'] = $r['Age'];
		$rs['address'] = $r['a_address'];//." - ".$r['a_zip'];			
		$rs['contact'] = '<label>H:&nbsp;</label>'.$r['a_phone_home']." <label>M:&nbsp;</label>".$r['a_phone_mobile']." <label>O:&nbsp;</label>".$r['a_phone_office']." <label>Email:&nbsp;</label>".$r['a_email'];
		$rs['contact_home'] = $r['a_phone_home'];
		$rs['contact_mobile'] = $r['a_phone_mobile'];
		$rs['contact_office'] = $r['a_phone_office'];
		$rs['contact_email'] = $r['a_email'];
		$rs['note'] = $r['a_internal_note'];
		$rs['willingtoserve'] = $r['a_willing_to_serve']?"Yes":"No";
		$rs['extra'] = $r['a_extra'];

		$rs['course_10d'] = $r['ac_10d']?$r['ac_10d']:"0";
		$rs['course_stp'] = $r['ac_stp']?$r['ac_stp']:"0";
		$rs['course_spl'] = $r['ac_spl']?$r['ac_spl']:"0";
		$rs['course_20d'] = $r['ac_20d']?$r['ac_20d']:"0";
		$rs['course_30d'] = $r['ac_30d']?$r['ac_30d']:"0";
		$rs['course_45d'] = $r['ac_45d']?$r['ac_45d']:"0";
		$rs['course_60d'] = $r['ac_60d']?$r['ac_60d']:"0";
		$rs['course_tsc'] = $r['ac_tsc']?$r['ac_tsc']:"0";
		$rs['course_seva'] = $r['ac_service']?$r['ac_service']:"0";

		$rs['first_course'] = $rs['last_course'] = '';
		if ($r['a_old'])
		{
			$rs['course_others'] = $r['ac_stp']+$r['ac_spl']+$r['ac_20d']+$r['ac_30d']+$r['ac_45d']+$r['ac_60d']+$r['ac_tsc'];
			$rs['first_course'] = $r['first_course'];
			$rs['last_course'] = $r['last_course'];
			$rs['practice_details'] = $r['ac_practice_details'];
		}
		else
			$rs['course_others'] = "0";
		$rs['Education'] = $r['a_education'];
		$rs['Company'] = $r['a_company'];
		$rs['Dept'] = $r['a_department'];
		$rs['Occ'] = $r['a_occupation'];
		if ($r['a_occupation'] == '')
			$rs['Occ'] = $r['a_occupation_past'];
		$rs['Designation'] = $r['a_designation'];
		$rs['othertechnique'] = $r['ae_desc_other_technique']?$r['ae_desc_other_technique']:"";
		$rs['physical'] = $r['ae_desc_physical']?$r['ae_desc_physical']:"";
		$rs['mental'] = $r['ae_desc_mental']?$r['ae_desc_mental']:"";
		$rs['medication'] = $r['ae_desc_medication']?$r['ae_desc_medication']:"";
		$rs['addiction'] = $r['ae_desc_addiction_current']?$r['ae_desc_addiction_current']:"";
		$rs['lang'] = trim($r['a_lang_1']." ".$r['a_lang_2']. " ".$r['a_lang_3']." ".$r['a_langs']);
		$rs['alist'] = $r['a_alist'];
		$rs['pregnant'] = $r['ae_pregnant']?'Yes':'No';
		if ($r['ae_pregnant'] && ( ! in_array(strtolower($r['ae_pregnant_detail']), array("-", "no")) ) )
			$rs['pregnant'] .= '&nbsp;( '.$r['ae_pregnant_detail'].' months )';
		$rs['photo'] = '';
		if ( $r['a_photo'] <> '')
			$rs['photo'] = file_create_url($r['a_photo']);
		//$rows[] = array_values($rs);
		$rows[] = $rs;
	}
	//$header = array('Detail', 'Applicant Name', 'Course Name', 'Status', 'ConfNo' , 'Gender', 'Country/State/City', 'Age');
	$header = array();
	$attributes = array('id' => 'table-applicants');
	$out = theme('table', array('header' => $header, 'rows' => array(), 'attributes' => $attributes));
	$data = json_encode($rows);

	if ( user_access('change status'))
	{
		$status_select = '<option value="" selected="selected">Choose</option>';
		foreach ($statuses as $key => $value) 
			if ($key <> '')
				$status_select .= '<option value="'.$key.'">'.$value.'</option>';
		$status_action = ' if (data.finalized) return "NA"; else  return "<select id=\"new-status-"+data.aid+"\" name=\"newstatus"+data.aid+"\" class=\"status-select\">'.addslashes($status_select).'</select><div class=\"sub-letter-"+data.aid+"\" subletter=\"\"></div><button type=\"button\" class=\"update-status\" id=\"btn-"+data.aid+"\">Update</button>";
			';
	}
	else
		$status_action = 'return "No access";';

	if (user_access('transfer course'))
		$transfer_action = 'return "<a href=\"#\" action=\"Transfer\" class=\"course-transfer\">Transfer</a>";';
	else
		$transfer_action = 'return "No access"';

	if (user_access('add application'))
		$add_action = 'return "<a href=\"#\" action=\"Add\" class=\"applicant-add\">Add</a>";';
	else
		$add_action = 'return "No access"';


	if (user_access('delete application'))
		$del_action = 'return "<a href=\"#\" action=\"Del\" class=\"applicant-del\">Del</a>";';
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
			if (d.gender == "Female")
			{
 			    pregnant =   "<td><label>Pregnant:</label></td>"+
		                "<td>"+d.pregnant+"</td>";
			}
			if ( d.photo )
			{
				photo = "<td colspan=2><img width=\"150\" src=\""+d.photo+"\" /></td>";
			}
		    return "<div class=\"slider\">"+
		        "<table cellpadding=\"5\" cellspacing=\"0\" border=\"0\" style=\"padding-left:50px;\">"+
		            "<tr>"+
		            "<td colspan=6><div class=\"clarification-response-"+d.aid+"\">Loading Clarification Data...</div></td>"+
		            "</tr>"+		        
		            "<tr>"+
		                "<td><label>Address:</label></td>"+
		                "<td colspan=\"4\">"+d.address+"</td>"+
		                "<td>"+d.location+"</td>"+
		             "</tr>"+
		             "<tr>"+
		                "<td><label>Contact:</label></td>"+
		                "<td colspan=\"3\">"+d.contact+"</td>"+
		                "<td><label>Languages:</label></td>"+
		                "<td>"+d.lang+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Occupation/Edu:</label></td>"+
		                "<td>"+d.Occ+"/"+d.Education +"</td>"+
		                "<td><label>Company:</label></td>"+
		                "<td>"+d.Company+"</td>"+
		                "<td><label>A-List?</label></td>"+
		                "<td>"+(d.alist == 1? "Yes": "No")+"</td>"+
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Designation:</label></td>"+
		                "<td>"+d.Designation+"</td>"+
		                "<td><label>Department:</label></td>"+
		                "<td>"+d.Dept+"</td>"+photo+
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Courses:</label></td>"+
		                "<td><table><tr><td>10d</td><td>STP</td><td>SPL</td><td>TSC</td><td>20d</td><td>30d</td><td>45d</td><td>60d</td><td>Seva</td></tr>"+
		                "<tr><td>"+d.course_10d+"</td><td>"+d.course_stp+"</td><td>"+d.course_spl+"</td><td>"+d.course_tsc+"</td><td>"+d.course_20d+"</td><td>"+d.course_30d+"</td><td>"+d.course_45d+"</td><td>"+d.course_60d+"</td><td>"+d.course_seva+"</td></tr></table></td>"+
		                "<td><label>Course Details:</label></td>"+
		                "<td>First: "+d.first_course+"<br>Last: "+d.last_course+"</td>"+
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Practice Details:</label></td>"+
		                "<td>"+d.practice_details+"</td>"+
		                "<td><label>Other Techniques:</label></td>"+
		                "<td>"+d.othertechnique+"</td>"+			            	
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Mental Heath:</label></td>"+
		                "<td>"+d.mental+"</td>"+			            	
		                "<td><label>Physical Health:</label></td>"+
		                "<td>"+d.physical+"</td>"+			            	
		            "</tr>"+
		            "<tr>"+
 			            "<td><label>Medication:</label></td>"+
		                "<td>"+d.medication+"</td>"+			            	
 			            "<td><label>Intoxicants:</label></td>"+
		                "<td>"+d.addiction+"</td>"+ pregnant +			            	
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Internal Note:</label></td>"+
			            "<td>"+d.note+"</td>"+
	 				    "<td><label>Other Info:</label></td>"+
	 				    "<td>"+d.extra+"</td>"+
	 				    "<td><label>Willing to Serve:</label></td>"+
	 				    "<td>"+d.willingtoserve+"</td>"+
 		            "</tr>"+		       
 		            "<tr>"+
		            "<td colspan=6><div class=\"applied-courses-"+d.aid+"\">Loading Course Data...</div></td>"+
		            "</tr>"+
		            "<tr>"+
		            "<td colspan=6><div class=\"activity-log-"+d.aid+"\">Loading Activity Log...</div></td>"+
		            "</tr>"+
		        "</table>"+
		    "</div>";
		}

		(function ($) {
			$(document).ready(function(){

				var dataset = '.$data.';
				var letters = '.$letters_json.';				
				var courses = '.$courses_json.';	
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
					dom: "'.$page_dom.'",  /*lBfr<\"clear\"p>tip*/
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
			            { data: "course", "title" : "Course Name" },
			            { data: "status", "title" : "Status" },
			            { data: "confno", "title" : "Conf No" },
			            { data: "gender", "title" : "Gender" },
				        { data: "age", "title" : "Age"},
				        { data: null, "title" : "ChangeStatus", render: function(data, type, row) {
				        	'.$status_action.'	
				        } },
				        { data: null, "title" : "Transfer", render: function(data, type, row) {
				        	if (data.allowtransfer )
				        		'.$transfer_action.'
				        	else
				        		return "NA";	
				        } },
				        { data: null, "title" : "Add", render: function(data, type, row) {
				        	'.$add_action.'	
				        } },
				        { data: null, "title" : "Del", render: function(data, type, row) {
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
		            	$(".clarification-response-"+row.data().aid).load("/app-clarifications/"+row.data().aid);
		            	$(".applied-courses-"+row.data().aid).load("/app-courses/"+row.data().aid);
		            	$(".activity-log-"+row.data().aid).load("/app-activity/"+row.data().aid);			            	
			        }
			    });

				$("#table-applicants tbody").on("change", ".status-select", function() {
					var s = $(this);
					//alert($(this).val());
					var tmp = $(this).val();
			        var tr = s.closest("tr");
			        var row = table.row( tr );
	                $(".sub-letter-"+row.data().aid).text("");
	                $(".sub-letter-"+row.data().aid).attr("subletter", "");
					if ( (letters[tmp] === null) || (typeof letters[tmp] !== "object") )
						return;
					if ( letters[tmp].length > 1 )
					{
						$.confirm({
						    title: row.data().name,
						    content: "" +
						    "<form action=\"\" class=\"formName\">" +
						    "<div class=\"form-group\">" +
						    "<label>Select Letter to send</label>" +
						    "<select class=\"letter-select form-control\" name=\"letter_select\" required><option value=\"\">Choose</option></select>" +
						    "<label>Additional Comment to send</label>"+
						    "<textarea name=\"comment\" id=\"txt-comment\"></textarea>"+
						    "</div>" +
						    "</form>",
						    buttons: {
						        formSubmit: {
						            text: "Select",
						            btnClass: "btn-blue",
						            action: function () {
						                var letter = this.$content.find(".letter-select").val();
						                var letter_txt = this.$content.find(".letter-select :selected").text();
						                comment = this.$content.find("#txt-comment").val();
						                if(!letter){
						                    $.alert("Please select a letter");
						                    return false;
						                }
								        var tr = s.closest("tr");
								        var row = table.row( tr );

						                $(".sub-letter-"+row.data().aid).text(letter_txt);
						                $(".sub-letter-"+row.data().aid).attr("subletter", letter);
						                //$.alert("Your selected " + letter);
						            }
						        },
						        cancel: function () {
						            //close
						        },
						    },
						    onContentReady: function () {
						        // bind to events
						        var jc = this;
						        $(".letter-select").empty();
						        var i = 0;
						        for(l = letters[tmp].length; i < l; i++) 
						        {
						        	$(".letter-select").append($("<option>", { value : letters[tmp][i].id }).text( letters[tmp][i].status ));
						        }
						        this.$content.find("form").on("submit", function (e) {
						            // if the user submits the form by pressing enter in the field.
						            e.preventDefault();
						            jc.$$formSubmit.trigger("click"); // reference the button and click it
						        });
						    }
						});
					}

					//alert(letters[tmp].length);
				});

				$("#table-applicants tbody").on("click", ".update-status", function() {
					var id = $(this).attr("id").replace("btn-", "");
					var s = $("#new-status-"+id).val() ;
					var letter = $(".sub-letter-"+id).attr("subletter");
					if (s == "")
					{
						alert("Please select a status");
						return;	
					} 			
					var tr1 = $(this).closest("tr");
					var row1 = table.row( tr1 );

					$(tr1).loading();
					$.getJSON( "/change-status/"+id, { s: s, l: letter, c:comment } )
					  .done(function( json ) {
					  	if (json.status == "OK")
					  	{
					  		if ( s != "'.$system_statues['STATUS-CUSTOM'].'")
								row1.data().status = s;
							row1.data().confno = json.confno;
							var d = row1.data().status;
							row1.invalidate().draw("page");
							tr1.css("background-color", "#bdcdea");						  		
					  	}
					  	else
					  		alert("Update failed "+json.msg)
					  	$(tr1).loading("stop");
					  })
					  .fail(function( jqxhr, textStatus, error ) {
					    var err = textStatus + ", " + error;
					    alert("Error updating "+err);
					    $(this).loading("stop");
					});			

				});

				$("#table-applicants tbody").on("click", ".applicant-del", function() {
			        var tr = $(this).closest("tr");
			        var row = table.row( tr );
			        var id = row.data().aid;
					var r = confirm("Are you sure you want to delete applicant "+$("<div/>").html(row.data().name).text()+" ?");
					if (r == true)
					{
       					$(tr).loading();
						$.getJSON( "/app/"+id+"/delete", {} )
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

				$("#table-applicants tbody").on("click", ".course-transfer,.applicant-add", function() {
					var s = $(this);
					var action = s.attr("action");
			        var tr = $(this).closest("tr");
			        var row = table.row( tr );
			        var comment_transfer = "";
			        if ( action == "Transfer")
			        	comment_transfer = "<label>Comment</label><br><textarea name=\"comment\" id=\"trf-comment\" size=\"30\"></textarea>";
					$.confirm({
					    title: "Select Course",
					    content: "" +
					    "<form action=\"\" class=\"formName\">" +
					    "<div class=\"form-group\">" +
					    "<label>"+action+" "+row.data().name+" To course</label>" +
					    "<select class=\"course-select form-control\" name=\"course_select\" required><option value=\"\">Choose</option></select>" +
					    comment_transfer +
					    "</div>" +
					    "</form>",
					    buttons: {
					        formSubmit: {
					            text: "Select",
					            btnClass: "btn-blue",
					            action: function () {
					                var course = this.$content.find(".course-select").val();
					                var course_txt = this.$content.find(".course-select :selected").text();
					                var c = this.$content.find("#trf-comment").val();

					                if(!course){
					                    $.alert("Please select a course");
					                    return false;
					                }
							        var tr = s.closest("tr");
							        var row = table.row( tr );
		        					var id = row.data().aid;
		        					if ( course == row.data().courseid )
		        					{
		        						$.alert("Source and Destination course cannot be the same!");
		        						return false;	
		        					}
		        					$(tr).loading();
					                //$.alert("Your selected " + course + " - " + row.data().aid );
					                if ( action == "Transfer")
					                {
										$.getJSON( "/move-to-course/"+id+"/"+course, {c: c} )
										  .done(function( json ) {
										  	if (json.status == "OK")
										  	{
												//row1.data().status = s;
												//var d = row1.data().status;
												//row1.invalidate().draw();
												tr.css("background-color", "#ffff00");						  		
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
					                else
					                {
										location.href = "/app/add/'.$centre.'/"+course+"?ref="+id;					                	
					                }
					            }
					        },
					        cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to events
					        var jc = this;
					        $(".course-select").empty();
					        var i = 0;
					        for(l = courses.length; i < l; i++) 
					        {
					        	$(".course-select").append($("<option>", { value : courses[i].id }).text( courses[i].name ));
					        }
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
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

function dh_manageapp_search_form($form, &$form_state)
{
	global $user;
	$centre_id = '';
	if (arg(1) <> '')
		$centre_id = arg(1);
	//$q = "select td_val1, td_val1 from dh_type_detail where td_key='Appl_stat' order by td_val1";
	$q = "select td_key, td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statuses = db_query($q)->fetchAllKeyed();

	$q = "select td_key, td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$statuses = db_query($q)->fetchAllKeyed();
	$statuses[''] = 'Choose';

	$statuses[$system_statuses['STATUS-RECEIVED']] = $system_statuses['STATUS-RECEIVED'];
	$statuses[$system_statuses['STATUS-LEFT']] = $system_statuses['STATUS-LEFT'];
	$statuses[$system_statuses['STATUS-CONFIRMED']] = $system_statuses['STATUS-CONFIRMED'];
	$statuses[$system_statuses['STATUS-CANCELLED']] = $system_statuses['STATUS-CANCELLED'];
	$statuses[$system_statuses['STATUS-CLARIFICATIONRESPONSE']] = $system_statuses['STATUS-CLARIFICATIONRESPONSE'];
	$statuses[$system_statuses['STATUS-ATTENDED']] = $system_statuses['STATUS-ATTENDED'];
//	$statuses = db_query($q)->fetchAllKeyed();
//	$statuses[''] = 'Choose';

	if ( isset($form_state['storage']) )
	{
		$storage = $form_state['storage'];
		if( $centre_id == '')
			$storage['centre'] = $centre_id;

		$where = "a_center='".$centre_id."'";
		if ( $storage['course'] <> '' )
			$where .= " and a_course='".$storage['course']."'";
		if ( $storage['f_name'] <> '' )
			$where .= " and a_f_name like '".trim($storage['f_name'])."%'";
		if ( $storage['l_name'] <> '' )
			$where .= " and a_l_name like '".trim($storage['l_name'])."%'";
		if ( $storage['conf_no'] <> '' )
			$where .= " and a_conf_no = '".trim($storage['conf_no'])."'";
		if ( $storage['phone_home'] <> '' )
			$where .= " and a_phone_home like '".trim($storage['phone_home'])."%'";
		if ( $storage['mobile'] <> '' )
			$where .= " and a_phone_mobile like '".trim($storage['mobile'])."%'";
		if ( $storage['status'] <> '' )
		{
			if ($storage['status'] == 'Attended')
				$where .= " and a_attended = '1'";
			elseif ($storage['status'] == 'Confirmed')
				$where .= " and a_status in ('Confirmed', 'Expected')";
			else
				$where .= " and a_status = '".$storage['status']."'";
		}
		if ( $storage['type'] <> '' )
			$where .= " and a_type = '".$storage['type']."'";

		if ( ! (user_access("access male") && (user_access("access female"))) )
		{
			if ( user_access("access male") ) $gender = 'M'; else $gender = 'F'; 
			$where .= " and a_gender ='$gender'";
		}
		elseif ( $storage['gender'] <> '' )
			$where .= " and a_gender = '".$storage['gender']."'";

		if ( $storage['country'] <> '' )
			$where .= " and a_country = '".$storage['country']."'";

		if ( $storage['is_foreign'] <> '' )
		{
			$centre_country = db_query("select c_country from dh_center where c_id=$centre_id")->fetchField();
			$where .= " and a_country NOT IN ('$centre_country')";
		}

		if ( $storage['state'] <> '' )
			$where .= " and s_name LIKE '".$storage['state']."%'";
		if ( $storage['email'] <> '' )
			$where .= " and a_email LIKE '".$storage['email']."%'";
		if ( $storage['mode'] <> '' )
			$where .= " and a_source = '".$storage['mode']."'";
		if ( isset($storage['alist']) && ($storage['alist']) )
			$where .= " and a_alist=1";
		if ( isset($storage['is_at']) && ($storage['is_at']) )
			$where .= " and ac_teacher=1";
		if ( isset($storage['is_willing_to_serve']) && ($storage['is_willing_to_serve']) )
			$where .= " and a_willing_to_serve=1";
		if ( isset($storage['is_pregnant']) && ($storage['is_pregnant']) )
			$where .= " and ae_pregnant=1";


		$out = '<h2>'.l("Back to Search", "search-app/".$centre_id).'</h2>';
		//if ($storage['course'] <> '')
		//	$out .= "&nbsp;&nbsp;|&nbsp;&nbsp;".l("Add Applicant", "app/add/$centre_id/".$storage['course']);
		//$out .= '</h2>';
		$storage['db_type'] = 'a';
		if ( $storage['db_type'] == 'a' )
			$db_type = 'applicant';
		else
			$db_type = 'student';
		$out .= dh_manageapp_search_results( $db_type, $where, $centre_id );

		$form['out'] = array('#markup' => $out, '#weight' => 100 );
		return $form;
	}
	else
	{
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
		drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');


		if ( $centre_id == '' )
		{
			if (user_access("access all centres"))
				$q = "select c_id, c_name from dh_center order by c_name";
			else
				$q = "select c_id, c_name from dh_center left join dh_user_center uc on c_id=uc_center where uc_user='".$user->uid." and uc_deleted=0' order by c_name";
			$centres = db_query($q)->fetchAllKeyed();
			if (count($centres) > 1)
				$centres[''] = 'Choose';			
			$form['centre'] = array('#title' => 'Centre', '#type' => 'select', '#options' => $centres, '#weight' => 1);
		}

		$max_old_courses =  date('Y-m-d',strtotime("-".variable_get('max_old_courses', '6 month')));
		$centre_append = '';
		if ( $centre_id <> '' )
			$centre_append = ' and c_center='.$centre_id.' ';
		$q = "select c_id, c_name from dh_course where c_start >= '$max_old_courses' $centre_append and c_deleted=0 order by c_start desc";
		$courses = db_query($q)->fetchAllKeyed();
		$courses[''] = 'Choose';

		$q = "select c_code, c_name from dh_country order by c_name";
		$countries = db_query($q)->fetchAllKeyed();
		$countries[''] = 'Choose';
		//$q = "select s_code, s_name from dh_state order by s_name";
		//$states = db_query($q)->fetchAllKeyed();

		$q = "select td_val1, td_val1 from dh_type_detail where td_key='APPLICATION-MODE' order by td_val1";
		$modes = db_query($q)->fetchAllKeyed();
		$modes[''] = 'Choose';

		$breadcrumb = '';
		if (arg(2) <> '')
			$breadcrumb .= l("Back to Course", "course/".$centre_id."/".arg(2));

		if ($centre_id <> '')
			$breadcrumb .= l("Back to Dashboard", "centre/".$centre_id);
		if ($breadcrumb <> '')
			$form['a'] = array('#markup' => '<h3>'.$breadcrumb.'</h3>');
		$form['#attributes'] = array('class' => array('container-inline')); 
		$form['course'] = array('#title' => 'Course', '#type' => 'select', '#options' => $courses, '#weight' => 1);
		/*$form['db_type'] = array('#title' => 'DB Type', '#type' => 'radios', '#options' => array('s' => 'Student', 'a' => 'Applicant' ), '#default_value' => 'a',  '#weight' => 2);*/
		$form['f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#weight' => 3);
		$form['l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#weight' => 4 );
		$form['conf_no'] = array('#title' => 'Confirmation No', '#type' => 'textfield', '#size' => 15, '#weight' => 5 );
		$form['status'] = array('#title' => 'Status', '#type' => 'select', '#options' => $statuses, '#default_value' => '',  '#weight' => 6);
		$form['type'] = array('#title' => 'Applicant Type', '#type' => 'radios', '#options' => array('Student' => 'Applicant', 'Sevak' => 'Sevak'), '#default_value' => 'Student',  '#weight' => 7);
		$form['gender'] = array('#title' => 'Gender', '#type' => 'select', '#options' => array( '' => 'Choose', 'M' => 'Male', 'F' => 'Female'), '#default_value' => '',  '#weight' => 8);
		$form['country'] = array('#title' => 'Country', '#type' => 'select', '#options' => $countries, '#default_value' => '', '#weight' => 9);
		//$form['state'] = array('#title' => 'State', '#type' => 'select', '#options' => $states, '#weight' => 4);
		$form['state'] = array('#title' => 'State', '#type' => 'textfield', '#weight' => 10);
		$form['email'] = array('#title' => 'Email', '#type' => 'textfield', '#weight' => 11);
		$form['mode'] = array('#title' => 'Application Mode', '#type' => 'select', '#options' => $modes, '#weight' => 12 );
		$form['alist'] = array('#title' => '&nbsp;AList?', '#type' => 'checkbox', '#default_value' => '0',  '#weight' => 13);
		$form['is_at'] = array('#title' => '&nbsp;Is Teacher?', '#type' => 'checkbox', '#default_value' => '0',  '#weight' => 14);
		$form['is_foreign'] = array('#title' => '&nbsp;Is Foreign National?', '#type' => 'checkbox', '#default_value' => '0',  '#weight' => 15);
		$form['is_willing_to_serve'] = array('#title' => '&nbsp;Willing To Serve?', '#type' => 'checkbox', '#default_value' => '0',  '#weight' => 16);
		$form['is_pregnant'] = array('#title' => '&nbsp;Is Pregnant?', '#type' => 'checkbox', '#default_value' => '0',  '#weight' => 17);
		$form['mobile'] = array('#title' => 'Mobile', '#type' => 'textfield',  '#weight' => 18);
		$form['phone_home'] = array('#title' => 'Phone Home', '#type' => 'textfield', '#weight' => 19);
		/*$form['date_from'] = array('#title' => 'Received From', '#type' => 'date_popup', '#size' => '15', '#weight' => 6, 
			'#default_value' => '', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
	    		'maxDate' => 0, '#required' => 1, 
	    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
	    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
			'#theme_wrappers' => array('date_popup'), '#suffix' => '');
		$form['date_to'] = array('#title' => 'Received To', '#type' => 'date_popup', '#size' => '15', '#weight' => 6, 
			'#default_value' => '', '#date_format' => 'Y-m-d', '#datepicker_options' => array(
	    		'maxDate' => 0, '#required' => 1, 
	    		'dateFormat' => date_popup_format_to_popup('Y-m-d'),
	    	),'#date_year_range' => '-100:-10', '#date_label_position' => 'above',
			'#theme_wrappers' => array('date_popup'), '#suffix' => '');
		*/
		$form['sub'] = array('#type' => 'submit', '#value' => 'Search', '#weight' => 100);


		$js = '
			(function ($) {
				$(document).ready(function(){
					$("#edit-course").select2();
					$("#edit-country").select2();
				});
			})(jQuery);
		';
		drupal_add_js($js, 'inline');
		return $form;

	}
}

function dh_manageapp_search_form_validate($form, &$form_state) 
{
	$values = $form_state['values'];
	if (arg(1) <> '')
		$values['centre'] = arg(1);
  	if ($values['centre'] == '') 
  	{
    	form_set_error('centre', 'Centre must be selected');
  	}
  	$count = 0;
  	if ( $values['course'] <> '' ) $count++;
  	if ( $values['course'] <> '' ) $count++;
  	if ( $values['f_name'] <> '' ) $count++;
  	if ( $values['l_name'] <> '' ) $count++;
  	if ( $values['conf_no'] <> '' ) $count++;
  	if ( $values['email'] <> '' ) $count++;
  	if ( $values['mobile'] <> '' ) $count++;
  	if ( $values['phone_home'] <> '' ) $count++;
  	if ( $count == 0 )
  		form_set_error('course', 'Please select a Course or Name or Conf No or Email');
}

function dh_manageapp_search_form_submit($form, &$form_state)
{
	$form_state['storage'] = $form_state['input'];
	$form_state['rebuild'] = TRUE;
//	drupal_goto("donation-report-results", array('query' => $data));
}

