<?php

function dh_manageapp_search_results( $type, $where, $centre, $show_centre_name = False, $excel_file_name = null )
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
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/location-common.js');
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

	$q = "select td_key, td_key from dh_type_detail where td_type='COURSE-STATUS' and td_key != 'Reconfirmation'";
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

	$centres = array();

	if ( user_access('access other center courses') )
		$q = "select c_id, c_name from dh_center order by c_name";		
	else
		$q = "Select c_id, c_name from dh_center left join dh_user_center uc on c_id=uc_center where uc_user='".$user->uid." and uc_deleted=0' order by 	c_name";

	$centres = db_query($q)->fetchAllKeyed();

	$centres_j = array();
	foreach( $centres as $k => $v)
		$centres_j[] = array('id' => $k, 'name' => $v);
	$centres_json = json_encode($centres_j);

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
		$q = "select
			c.c_finalized,
			a_id,
			a_type,
			a_center,
			a_course,
			a_conf_no as 'ConfNo',
			concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name',
			a_gender,
			a_status,
			ci.c_name as 'City',
			s.s_name,
			co.c_name as 'Country',
			a_city_str,
			c.c_name,
			a_address,
			a_zip,
			a_email,
			a_phone_home,
			a_phone_office,
			a_phone_mobile,
			a_passport,
			a_voter_id,
			a_pancard,
			a_aadhar,
			a_education,
			a_occupation,
			a_occupation_past,
			a_company,
			a_designation,
			a_department,
			a_emergency_name,
			a_emergency_relation,
			a_emergency_num,
			a_alist,
			a_monk,
			a_uri,
			a_photo,
      a_friend_family_details,
      a_dob,
			TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age',
			a_lang_1,
			a_lang_2,
			a_lang_3,
			a_langs,
			a_lang_discourse,
			a_old,
			a_attended,
			a_internal_note,
			a_willing_to_serve,
			a_extra,
			a_referral,
			c.c_end,
			ac_teen,
			ac_10d,
			ac_stp,
			ac_spl,
			ac_20d,
			ac_30d,
			ac_45d,
			ac_60d,
			ac_tsc,
			ac_teacher,
      ac_teacher_code,
			ac_service,
			aa_left_comment,
			aa_section,
			aa_acco,
			CONCAT( IFNULL(ac_first_year, ''),'-',IFNULL(ac_first_month, ''),'-',IFNULL(ac_first_day, ''), ', ', ac_first_location_str) as 'first_course',
			CONCAT( IFNULL(ac_last_year,''),'-',IFNULL(ac_last_month,''),'-',IFNULL(ac_last_day, ''), ', ', ac_last_location_str) as 'last_course',
			ac_practice_details, 
			ae_desc_physical,
			ae_desc_medication,
			ae_desc_mental,
			CONCAT(IFNULL(ae_desc_other_technique,''), ' ',IFNULL(ae_desc_other_technique_old,'')) as 'ae_desc_other_technique',
			ae_desc_addiction_current,
			ae_pregnant,
			ae_pregnant_detail,
			ae_father,
			ae_mother,
      ae_id_issued,
      ae_id_issued_by,
			ae_parent_course,
			ae_teen_std,
            ae_nationality,
            al_recommending,
            al_recommending_approved,
            al_area_at,
            al_area_at_approved
			from
			dh_applicant
			left join dh_applicant_course ac on (a_id=ac_applicant)
			left join dh_applicant_attended aa on (a_id=aa_applicant)
			left join dh_applicant_extra ae on (a_id=ae_applicant)
            left join dh_applicant_lc al on (a_id=al_applicant)
			left join dh_course c on (a_course=c.c_id)
			left join dh_country co on (a_country=co.c_code)
			left join dh_city ci on a_city=ci.c_id
			left join dh_state s on (a_state=s.s_code and a_country=s.s_country)
			left join dh_center ce on (a_center=ce.c_id)
			where
			$where
			order by
			a_created,
			a_vray_id";
	else
		$q = "select
			c.c_finalized,
			s.s_id as 'a_id',
			'Student' as 'a_type',
			sc_course as 'a_course',
			sc_conf_no as 'ConfNo',
			concat(s_f_name, ' ',s_m_name, ' ',s_l_name) as 'Full Name',
			s_gender as 'a_gender',
			'Attended' as 'a_status',
			ci.c_name as 'City',
			st.s_name,
			co.c_name as 'Country',
			'' as 'a_city_str',
			c.c_name,
			s_address as 'a_address',
			s_zip as 'a_zip',
			s_email as 'a_email',
			s_phone_home as 'a_phone_home',
			s_phone_office as 'a_phone_office',
			s_phone_mobile as 'a_phone_mobile',
			s_education as 'a_education',
			s_occupation as 'a_occupation',
			s_occupation_past as 'a_occupation_past',
			s_company as 'a_company',
			s_designation as 'a_designation',
			s_department as 'a_department',
			s_emergency_name as 'a_emergency_name',
			s_emergency_relation as 'a_emergency_relation',
			s_emergency_num as 'a_emergency_num',
			s_alist as  'a_alist',
			s_monk as 'a_monk',
			s_photo as 'a_photo',
			TIMESTAMPDIFF( YEAR, s_dob, CURDATE() ) as 'Age',
			s_lang_1 as 'a_lang_1',
			s_lang_2 as 'a_lang_2',
			s_lang_3 as  'a_lang_3',
			s_langs as 'a_langs',
			1 as 'a_old',
			1 as 'a_attended',
			c.c_end,
			sci_teen as 'ac_teen',
			sci_10d as 'ac_10d',
			sci_stp as 'ac_stp',
			sci_spl as 'ac_spl',
			sci_20d as 'ac_20d',
			sci_30d as 'ac_30d',
			sci_45d as 'ac_45d',
			sci_60d as 'ac_60d',
			sci_tsc as 'ac_tsc',
			sci_teacher as 'ac_teacher',
			CONCAT( IFNULL(sci_first_year, ''),'-',IFNULL(sci_first_month, ''),'-',IFNULL(sci_first_day, ''), ', ', sci_first_location_str) as 'first_course',
			'' as 'ac_practice_details',
			sc_desc_physical as 'ae_desc_physical',
			sc_desc_medication  as 'ae_desc_medication',
			sc_desc_mental as 'ae_desc_mental',
			sc_desc_other_technique as 'ae_desc_other_technique',
			sc_addiction_current 'ae_desc_addiction_current',
			'' as 'ae_pregnant',
			'' as 'ae_pregnant_detail',
			sc_uri as 'a_uri'
			from
			dh_student s
			left join dh_student_course sc on (s.s_id=sc_student)
			left join dh_student_course_input sci on (s.s_id=sci_student)
			left join dh_course c on (sc_course=c.c_id)
			left join dh_country co on (s.s_country=co.c_code)
			left join dh_city ci on s_city=ci.c_id
			left join dh_state st on (s.s_state=st.s_code and s.s_country=st.s_country)
			where
			$where
			order by
			s_f_name";

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
		$rs['gender'] = (strtoupper($r['a_gender'])=='M')?'Male':'Female';
		$rs['o_n'] = $r['a_old']?"Old":"New";
		$rs['o_n'] .= '<br>'.$rs['gender'];
		$rs['courseid'] = $r['a_course'];

		$course_type = db_query("SELECT td_key, td_val3 FROM dh_course left join dh_type_detail on c_course_type=td_id where c_id=:c_id", array('c_id'=>$r['a_course']))->fetchAssoc();

        $LC = false;
		if ($course_type['td_val3'] == 'LC')
        {
			$rs['allowtransfer'] = 0;
            $LC = true;
        }

		$rs['centreid'] = $r['a_center'];
		$rs['finalized'] = $r['c_finalized']?1:0;
		if ( $r['a_attended'] ||  in_array($r['a_status'], array($system_statues['STATUS-LEFT'],$system_statues['STATUS-ATTENDED'] ))  )
			$rs['allowtransfer'] = 0;
		if (date('Y-m-d') > $r['c_end'])
			$rs['allowtransfer'] = 0;
		if ( $r['a_uri'] <> '' )
		{
			//$pdf_link = '&nbsp;('.l("PDF", file_create_url($r['a_uri']), array('attributes' => array("target" => "_blank") ) ).')';
			$pdf_link = '&nbsp;('.l("PDF", "show-application/".$r['a_id'], array('attributes' => array("target" => "_blank") ) ).')';
		}
		$rs['aid'] = $r['a_id'];
		$rs['changestatus'] = '' ;
		$rs['name'] = _ma_make_link($r['Full Name'],'app/'.$r['a_id'].'/edit').$pdf_link;
		if ($rs['type'] == 'sevak')
			$rs['name'] .= ' (Sevak)';
		if ($r['ac_teacher'] == 1)
    {
      if($r['ac_teacher_code'])
        $rs['name'] .= ' (AT, '.stristr($r['ac_teacher_code'], '-', true).')';
      else
        $rs['name'] .= ' (AT)';
    }
		if ( in_array($r['ConfNo'], array('NF', 'OF', 'NF0', 'OF0')) )
			$r['ConfNo'] = '';
		$temp = explode(" / ", $r['c_name']); unset($temp[0]);
		$rs['course'] = implode(" / ", $temp);
		if ($show_centre_name)
			$rs['course'] = $r['c_name'];
		$rs['status'] = $r['a_status'].($r['ConfNo']?" (".$r['ConfNo'].")":"");
		$rs['app_status'] = $r['a_status'];
		$rs['confno'] = $r['ConfNo'];
		if ($r['City'] == '')
			$r['City'] = $r['a_city_str'];
		$rs['location'] =  $r['City'].', '.$r['s_name'].', '.$r['Country'];
		$rs['city'] = $r['City'];
		$rs['state'] = $r['s_name'];
		$rs['country'] = $r['Country'];
		$rs['pin'] = $r['a_zip'];
    $rs['dob'] = $r['a_dob'];
		$rs['age'] = $r['Age'];
		$rs['address'] = $r['a_address'];//." - ".$r['a_zip'];			
		$rs['contact'] = '<label>H:&nbsp;</label>'.$r['a_phone_home']." <label>M:&nbsp;</label>".$r['a_phone_mobile']." <label>O:&nbsp;</label>".$r['a_phone_office']." <label>Email:&nbsp;</label>".$r['a_email'];
		$rs['contact_home'] = $r['a_phone_home'];
		$rs['contact_mobile'] = $r['a_phone_mobile'];
		$rs['contact_office'] = $r['a_phone_office'];
		$rs['contact_email'] = $r['a_email'];
		$rs['occupation'] = $r['a_occupation'];
		$rs['designation'] = $r['a_designation'];
		$rs['company'] = $r['a_company'];
		$rs['emergency_name'] = $r['a_emergency_name'];
		$rs['emergency_relation'] = $r['a_emergency_relation'];
		$rs['emergency_num'] = $r['a_emergency_num'];

		if (isset($r['a_lang_discourse']))
			$rs['lang_discourse'] = $r['a_lang_discourse'];
		else
			$rs['lang_discourse'] = "";

		$rs['friend_family'] = $r['a_friend_family_details'];
    $rs['note'] = $r['a_internal_note'];
		$rs['willingtoserve'] = $r['a_willing_to_serve']?"Yes":"No";
		$rs['extra'] = $r['a_extra'];
		$rs['monk'] = $r['a_monk'];
		$rs['physical'] = $r['ae_desc_physical']?$r['ae_desc_physical']:"";
		$rs['mental'] = $r['ae_desc_mental']?$r['ae_desc_mental']:"";
		$rs['medication'] = $r['ae_desc_medication']?$r['ae_desc_medication']:"";
		$rs['addiction'] = $r['ae_desc_addiction_current']?$r['ae_desc_addiction_current']:"";
		$rs['othertechnique'] = trim($r['ae_desc_other_technique'])?$r['ae_desc_other_technique']:"";
		$rs['pregnant'] = $r['ae_pregnant']?'Yes':'No';
    $rs['id_issued_date'] = $r['ae_id_issued'];
    $rs['id_issued_by'] = $r['ae_id_issued_by'];
    $rs['nationality'] = $r['ae_nationality'];

		if ($r['ae_pregnant'] && ( ! in_array(strtolower($r['ae_pregnant_detail']), array("-", "no")) ) )
			$rs['pregnant'] .= '&nbsp;( '.$r['ae_pregnant_detail'].' months )';

		$temp = "";
		$rs['eocd'] = $r['a_education']." / ".$r['a_occupation']." / ".$r['a_designation']." / ".$r['a_company'].'<div class="amcha-small">'.$rs['course'].'</div><br>'.($rs['physical']?'<div class="blue">Phy</div>&nbsp;|&nbsp;':"").($rs['mental']?'<div class="red">Mental</div>&nbsp;|&nbsp;':"").($rs['addiction']?'<div class="green">Intox</div>&nbsp;|&nbsp;':"").($rs['othertechnique']?'<div class="purple">Oth-Med</div>&nbsp;|&nbsp;':"").($r['ae_pregnant']?'<div class="pink">Preg</div>&nbsp;|&nbsp;':"").($r['a_alist']?'<div class="black">A-List</div>&nbsp;|&nbsp;':"").($r['a_monk']?'<div class="ochre">Monk</div>':"");


		$rs['course_teen'] = $r['ac_teen']?$r['ac_teen']:"0";
		$rs['course_10d'] = $r['ac_10d']?$r['ac_10d']:"0";
		$rs['course_stp'] = $r['ac_stp']?$r['ac_stp']:"0";
		$rs['course_spl'] = $r['ac_spl']?$r['ac_spl']:"0";
		$rs['course_20d'] = $r['ac_20d']?$r['ac_20d']:"0";
		$rs['course_30d'] = $r['ac_30d']?$r['ac_30d']:"0";
		$rs['course_45d'] = $r['ac_45d']?$r['ac_45d']:"0";
		$rs['course_60d'] = $r['ac_60d']?$r['ac_60d']:"0";
		$rs['course_tsc'] = $r['ac_tsc']?$r['ac_tsc']:"0";
		$rs['course_seva'] = $r['ac_service']?$r['ac_service']:"0";
		$rs['left_reason'] = $r['aa_left_comment'];
		$rs['section'] = ($r['aa_section'] && strtolower($r['aa_section']) <> 'null')?$r['aa_section']:"NA";
		$rs['acc'] = ($r['aa_acco'] && strtolower($r['aa_acco']) <> 'null')?$r['aa_acco']:"NA";

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
		$rs['lang'] = trim($r['a_lang_1']." ".$r['a_lang_2']. " ".$r['a_lang_3']." ".$r['a_langs']);
		$rs['alist'] = $r['a_alist'];
		
		$rs['photo'] = '';
		if ( $r['a_photo'] <> '')
		{
			//$rs['photo'] = file_create_url($r['a_photo']);
			$rs['photo'] = file_create_url("show-photo/".$r['a_id']);
		}
		$rs['passport'] = $r['a_passport'];
		$rs['voterid'] = $r['a_voter_id'];
		$rs['pancard'] = $r['a_pancard'];
		$rs['aadhar'] = $r['a_aadhar'];

        $rs['long_course'] = $LC;
        $rs['reco_t'] = $r['al_recommending'];
        $rs['reco_status'] = $r['al_recommending_approved'];
        $rs['area_t'] = $r['al_area_at'];
        $rs['area_status'] = $r['al_area_at_approved'];


		$rs['referral'] = $r['a_referral']?1:0;
		if ($r['a_referral'])
		{
			$q = "select
        concat(ifnull(t_f_name, ''), ' ', ifnull(t_l_name, ''), ' (', t_code, ')') as teacher,
        t_id as teacher_id,
        r.*
        from dh_referral r left join dh_teacher t on r.r_teacher=t.t_id where r_id=".$r['a_referral'];

			$r_ref = db_query($q)->fetchAssoc();

      $show_referral = 0;

      $course_level = array(
        "10-Day" => 1,
        "10-DayExecutive" => 1,
        "Satipatthana" => 2,
        "10-DaySpecial" => 3,
        "TSC" => 4,
        "20-DayOSC" => 5,
        "30-DayOSC" => 6,
        "45-DayOSC" => 7,
        "60-DayOSC" => 8,
      );

      if (!array_key_exists($course_type['td_key'], $course_level))
        $course_level[$course_type['td_key']] = 0;

      if( $r_ref["r_listed_10d"] )
        $show_referral = 1;
      elseif( $r_ref["r_listed_10d_seva"] && strtolower($rs['type']) == "sevak" )
        $show_referral = 1;
      elseif( $r_ref["r_listed_stp"] &&  $course_level[$course_type['td_key']] >= 2)
        $show_referral = 1;
      elseif( $r_ref["r_listed_spl"] &&  $course_level[$course_type['td_key']] >= 3)
        $show_referral = 1;
      elseif( $r_ref["r_listed_tsc"] &&  $course_level[$course_type['td_key']] >= 4)
        $show_referral = 1;
      elseif( $r_ref["r_listed_20d"] &&  $course_level[$course_type['td_key']] >= 5)
        $show_referral = 1;
      elseif( $r_ref["r_listed_30d"] &&  $course_level[$course_type['td_key']] >= 6)
        $show_referral = 1;
      elseif( $r_ref["r_listed_45d"] &&  $course_level[$course_type['td_key']] >= 7)
        $show_referral = 1;
      elseif( $r_ref["r_listed_60d"] &&  $course_level[$course_type['td_key']] >= 8)
        $show_referral = 1;
      elseif( $r_ref["r_listed_lc_seva"] && strtolower($rs['type']) == "sevak" &&  $course_level[$course_type['td_key']] >= 3)
        $show_referral = 1;

      if( date_create() <= date_create($r_ref['r_end']) && $show_referral)
      {
        $rs['ref_reason'] = $r_ref['r_reason'];
        $rs['ref_start'] = $r_ref['r_start'];
        $rs['ref_end'] = $r_ref['r_end'];

        if($r_ref['center'])
        {
           $rs['ref_listed_by'] = $r_ref['r_listed_by'];
           $rs['ref_listed_title'] = "Center RefListed by:";
        }
        else
        {
          $rs['ref_listed_title'] = "Referring AT:";
          $rs['ref_listed_by'] = "";

          if($r_ref['teacher_id'])
            $rs['ref_listed_by'] = '<a href="#" id="'.$r_ref['teacher_id'].'" class="get-at-info" >'.$r_ref['teacher'].'</a>';
        }

        $listed_for = "";
        $temp = array('10d', 'stp', 'spl', '20d', '30d', '45d', '60d', 'tsc', '10d_seva', 'lc_seva', 'longseva');
        foreach( $temp as $a )
          if ($r_ref['r_listed_'.$a])
            $listed_for .= strtoupper($a)." | ";
        $rs['ref_listed_for'] = $listed_for;
      }
      else
      {
        $rs['referral'] = 0;
      }


			
		}

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
		{
			if ($key <> '')
				$status_select .= '<option value="'.$key.'">'.$value.'</option>';
		}


		$status_action = ' if (data.finalized) return "NA"; 
			else  
			{
				var tmp = "";
				if (data.app_status == "R-ATReview" || data.app_status == "A-ATReview")
				{
					var t = "Rejected-"+data.app_status[0]+"-AT";
					return "<select id=\"new-status-"+data.aid+"\" name=\"newstatus"+data.aid+"\" class=\"status-select\"><option value=\"\">Choose</option><option value=\"Approved\">Approved</option><option value=\""+t+"\">Rejected</option><option value=\"Cancelled\">Cancelled</option></select><button type=\"button\" class=\"update-status\" id=\"btn-"+data.aid+"\">Update</button>";
				}
				else if (data.app_status == "R-ATTransfer")
				{
					var t = "R-ATReview";
					return "<select id=\"new-status-"+data.aid+"\" name=\"newstatus"+data.aid+"\" class=\"status-select\"><option value=\"\">Choose</option><option value=\"R-ATReview\">R-AT Review</option><option value=\"Cancelled\">Cancelled</option></select><button type=\"button\" class=\"update-status\" id=\"btn-"+data.aid+"\">Update</button>";

				}
                else if (data.long_course && data.reco_status != "Approved" && data.app_status != "R-ATReview" && data.app_status != "R-ATTransfer")
                {
                    return "NA";
                }
                else if (data.long_course && data.reco_status == "Approved" && data.area_status != "Approved" && data.app_status != "A-ATReview")
                {
                    return "NA";
                }
				if (data.app_status == "Confirmed" || data.app_status == "ReConfirmation")
				{
					 tmp = "<option value=\"ReConfirmation\">ReConfirmation</option><option value=\"Expected\">Expected</option>";
				}
				return "<select id=\"new-status-"+data.aid+"\" name=\"newstatus"+data.aid+"\" class=\"status-select\">'.addslashes($status_select).'"+tmp+"</select><div class=\"sub-letter-"+data.aid+"\" subletter=\"\"></div><button type=\"button\" class=\"update-status\" id=\"btn-"+data.aid+"\">Update</button>";
			}
			';
	}
	else
		$status_action = 'return "No access";';

	
//	$add_action = 'return "<a href=\"#\" action=\"Add\" class=\"applicant-add\">Add</a>";';

	if ((user_access('transfer course')) || (user_access('add application')) || (user_access('delete application')))
	{	
		$action_select = '<option value=\"\" selected=\"selected\">Choose</option>';
		$actions = array();
		if (user_access("transfer course")) {
			$action_select .='<option value=\"Transfer\" >Transfer</option>';
			$actions[] = "Transfer";
		}
		if (user_access("add application")) {
			$action_select .='<option value=\"Add\">Add</option>';
			$actions[] = "Add";
		}
		if (user_access("manage referral list")) {
			$action_select .='<option value=\"Add-referral\">Add Referral</option>';
			$actions[] = "Add Referral";
		}
		if (user_access("manage center referral list")) {
			$action_select .='<option value=\"Add-center-referral\">Add Center Referral</option>';
			$actions[] = "Add Center Referral";
		}
		if (user_access("delete application")) {
			$action_select .='<option value=\"Delete\">Delete</option>';
			$actions[] = "Delete";
		}
		//$action = 'return "<select id=\"action-type\" class=\"action-select\">'.$action_select.'</select>";';
		$action = implode("," , $actions);
//		$action_select .='<option value="Add">'.$add_action.'</option>';
		//$action_select .= 'return "<a href=\"#\" action=\"Transfer\" class=\"course-transfer\">Transfer</a>";';
	}
	else
		$action = '';
		//$action = 'return "No access"';


	$button_access = '';
    $excel_file_name = ($excel_file_name)?$excel_file_name:"Student Data";
	if (user_access('export data'))
		if ( !($iPad || $iPhone || $iPod) )
			$button_access = '                        
				$.extend( true, {}, buttonCommon, {
      			    extend: "excelHtml5",
	            	title: "'.$excel_file_name.'"
				} )
			';

	//$allow_other_centers = user_access('access other center courses')?1:0; 

/*

	            $.extend( true, {}, buttonCommon, {
	                extend: "pdfHtml5",
	            	title: "A-List"
				} ),


 */
	//$out .= '<div id="replace-me"></div>';
	$js = '
		function format ( d ) {
			var pregnant = "", photo = "", referral = "", left_reason = "";
            var long_course = "";
			
			if (d.gender == "Female")
			{
 			    pregnant =   "<label>Pregnant:</label>"+
		                "<br>"+d.pregnant;
			}
			if ( d.photo )
			{
				photo = "<td rowspan=3><img width=\"150\" src=\""+d.photo+"\" /></td>";
			}
			if ( d.left_reason )
			{
				left_reason = "<td><label>Left Reason:</label><br>"+d.left_reason+"</td>";
			}
			if ( parseInt(d.referral) > 0  )
			{
				referral = "<tr><td class=\"referral\"><label>"+d.ref_listed_title+"</label></td><td class=\"referral\">"+d.ref_listed_by+"</td>"+
					"<td class=\"referral\"><label>From:</label></td><td class=\"referral\">"+d.ref_start+"</td>"+
					"<td class=\"referral\"><label>To:</label></td><td class=\"referral\">"+d.ref_end+"</td></tr>"+
					"<tr><td class=\"referral\"><label>Reason:</label></td><td class=\"referral\">"+d.ref_reason+"</td>"+
					"<td class=\"referral\"><label>Listed For:</label></td><td class=\"referral\" colspan=\"3\">"+d.ref_listed_for+"</td></tr>";
			}
            if ( d.long_course )
            {
                long_course = "<tr><td colspan=2> <label>RecoAT:</label> "+d.reco_t+" ("+d.reco_status+")</td><td colspan=3><label>AreaT:</label> "+d.area_t+" ("+d.area_status+")</td></tr>";
            }
		    return "<div class=\"slider\">"+
		        "<table cellpadding=\"5\" cellspacing=\"0\" border=\"0\" style=\"padding-left:50px;\">"+
		            "<tr>"+
		            "<td colspan=6><div class=\"clarification-response-"+d.aid+"\">Loading Clarification Data...</div></td>"+
		            "</tr>"+ referral + long_course +
		            "<tr>"+
		                "<td colspan=\"5\">"+d.address+", "+d.location+" / "+d.contact+"</td>"+
		             "</tr>"+
		            "<tr>"+
			            "<td><label>Courses:</label>"+
		                "<table><tr><td>Teen</td><td>10d</td><td>STP</td><td>SPL</td><td>TSC</td><td>20d</td><td>30d</td><td>45d</td><td>60d</td><td>Seva</td></tr>"+
		                "<tr><td>"+d.course_teen+"</td><td>"+d.course_10d+"</td><td>"+d.course_stp+"</td><td>"+d.course_spl+"</td><td>"+d.course_tsc+"</td><td>"+d.course_20d+"</td><td>"+d.course_30d+"</td><td>"+d.course_45d+"</td><td>"+d.course_60d+"</td><td>"+d.course_seva+"</td></tr></table></td>"+
		                "<td><label>Details:</label>"+
		                "<br>First: "+d.first_course+", Last: "+d.last_course+"</td>"+
		                "<td><label>Practice Details:</label>"+ 
		                "<br>"+d.practice_details+"</td>"+ photo +
		            "</tr>"+
		            "<tr>"+
		                "<td><label>Physical:</label>"+
		                "&nbsp;"+d.physical+"</td>"+			            			                			            	
		                "<td><label>Other Techniques:</label>"+
		                "<br>"+d.othertechnique+"</td>"+			            	
		                "<td><label>Dept:</label>"+
		                "<br>"+d.Dept+"</td>"+
		            "</tr>"+
		            "<tr>"+
		            	"<td><label>Mental:</label>"+
		                "&nbsp;"+d.mental+"</td>"+
 			            "<td><label>Medication:</label>"+
		                "<br>"+d.medication+"</td>"+			            	
 			            "<td><label>Intoxicants:</label>"+
		                "<br>"+d.addiction+"</td>" + left_reason + 
		            "</tr>"+
		            "<tr>"+
			            "<td><label>Internal Note:</label>"+
			            "&nbsp;"+d.note+"</td>"+
	 				    "<td><label>Other Info:</label>"+
	 				    "&nbsp;"+d.extra+"</td>"+
	 				    "<td><label>Willing-to-Serve:</label>"+
	 				    "<br>"+d.willingtoserve+"</td>"+ 
			            "<td><label>Discourse-Lang:</label>"+
			            "<br>"+d.lang_discourse +"</td>" + "<td>" + pregnant + "</td>" +
 		            "</tr>"+
                "<tr>"+
                  "<td><label>Friend-Family:</label>"+
                  "&nbsp;"+d.friend_family+"</td>"+
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
				var centres = '.$centres_json.';

				var comment = "";			

			    var buttonCommon = {
			        exportOptions: {
			            format: {
			            	header: function( data, column, node ) {
			            		var h = ["Name", "Gender", "Age", "Courses", "PhoneMobile", "PhoneHome", "PhoneOffice", "Email", "Education", "Occupation", "Company", "Designation/Dept", "Address", "Pin", "City", "State","Country", "Accommodation","ID Type", "ID No", "Conf No", "Physical Health", "Mental Health", "Medication", "Pregnancy Details", "Other Meditation Techniques", "Emergency Name", "Emergency Relation", "Emergency Contact No", "Language", "Addiction", "Other Info", "Friend Family", "ID Issued Date", "ID Issued By", "DOB", "Nationality"];
			            		return h[column];
			            	},
			                body: function ( data, row, column, node ) {
			                	var tmp = table.rows({search:"applied"}).data();
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
			                		a = "S: "+(+tmp[ row ].course_10d + +tmp[ row ].course_teen + +tmp[ row ].course_stp)+" L: "+(+tmp[ row ].course_20d + +tmp[ row ].course_30d + +tmp[ row ].course_45d + +tmp[ row ].course_60d + +tmp[ row ].course_spl + +tmp[ row ].course_tsc )+ " Seva: "+ tmp[ row ].course_seva; 
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
			                	else if (column == 17) {
			                		return tmp[ row ].section + " - " + tmp[ row ].acc; 
			                	}
			                	else if ( column == 18 )
			                	{
			                		return (tmp[ row ].passport?"Passport":"") + (tmp[ row ].voterid?"Voter ID":"") + (tmp[ row ].pancard?"Pan card":"") + (tmp[ row ].aadhar?"Aadhar":""); 
			                	}
			                	else if ( column == 19 )
			                	{
			                		return (tmp[ row ].passport?tmp[ row ].passport:"") + (tmp[ row ].voterid?tmp[ row ].voterid:"") + (tmp[ row ].pancard?tmp[ row ].pancard:"") + (tmp[ row ].aadhar?tmp[ row ].aadhar:""); 
			                	}
			                	else if ( column == 20 )
			                	{
			                		return tmp[ row ].confno; 
			                	}
			                	else if ( column == 21 )
			                	{
			                		return tmp[ row ].physical; 
			                	}
			                	else if ( column == 22 )
			                	{
			                		return tmp[ row ].mental; 
			                	}
			                	else if ( column == 23 )
			                	{
			                		return tmp[ row ].medication; 
			                	}
			                	else if ( column == 24 )
			                	{
			                		return tmp[ row ].pregnant; 
			                	}
			                	else if ( column == 25 )
			                	{
			                		return tmp[ row ].othertechnique; 
			                	}
			                	else if ( column == 26 )
			                	{
			                		return tmp[ row ].emergency_name; 
			                	}
			                	else if ( column == 27 )
			                	{
			                		return tmp[ row ].emergency_relation; 
			                	}
			                	else if ( column == 28 )
			                	{
			                		return tmp[ row ].emergency_num; 
			                	}
			                	else if ( column == 29 )
			                	{
			                		return tmp[ row ].lang_discourse; 
			                	}
                        else if ( column == 30 )
                        {
                          return tmp[ row ].addiction;
                        }
                        else if ( column == 31 )
                        {
                          return tmp[ row ].extra;
                        }
                        else if ( column == 32 )
                        {
                          return tmp[ row ].friend_family;
                        }
                        else if ( column == 33 )
                        {
                          return tmp[ row ].id_issued_date;
                        }
                        else if ( column == 34 )
                        {
                          return tmp[ row ].id_issued_by;
                        }
                        else if ( column == 35 )
                        {
                          return tmp[ row ].dob;
                        }
                        else if ( column == 36 )
                        {
                          return tmp[ row ].nationality;
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
					                "targets": [ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36],
					                "visible": false,
					                "searchable": false
					            },
					            {
					                "targets": [ 3, 4 ],
					                "visible": false,
					                "searchable": true
					            },
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
			            { data: "eocd", "title" : "Edu/Occ/Comp/Desig" },
			            { data: "contact_mobile", "title" : "Mobile" },
			            { data: "contact_email", "title" : "Email" },
			            { data: "status", "title" : "Status" },
			            { data: "o_n", "title" : "Type" },
				        { data: "age", "title" : "Age"},
				        { data: null, "title" : "ChangeStatus", render: function(data, type, row) {
				        	'.$status_action.'	
				        } },
				        { data: null, "title" : "Action", render: function(data, type, row) {
				        		var a = "'.$action.'";
				        		if (a == "")
				        			return "No Access";
				        		var t = a.split(",");
				        		var str = "<select id=\"action-type\" class=\"action-select\"><option value=\"\" selected=\"selected\">Choose</option>";
				        		for (var i=0; i < t.length; i++ )
				        		{
				        			if ( t[i] == "Transfer" || t[i] == "Delete" )
				        			{
					        			if ((data.allowtransfer == 1)   )
					        			{
					        				str += "<option value=\""+t[i]+"\">"+t[i]+"</option>";
					        			}				       			
				        			}
				        			else
				        				str += "<option value=\""+t[i]+"\">"+t[i]+"</option>";
				        		}
				        		str += "</select>";
				        		return str;
				        } },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
				        { data: null, "title": "" },
                { data: null, "title": "" },
                { data: null, "title": "" },
                { data: null, "title": "" },
                { data: null, "title": "" },
                { data: null, "title": "" },
                { data: null, "title": "" },
                { data: null, "title": "" },
		            ],
		            "fnRowCallback": function (nrow, data, display_index) {
		            	if (parseInt(data.referral) > 0 )
		            	{
 					   		$(nrow).addClass("referral");
		            	}
					},
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
							if ( json.newstatus != "" )
								row1.data().status = json.newstatus;
							if ( json.confno != "" )
								row1.data().status = s + " ("+json.confno+")";
							//row1.data().confno = json.confno;
							var d = row1.data().status;
							row1.invalidate().draw("page");
							tr1.css("background-color", "#bdcdea");						  		
					  	}
					  	else
					  		alert("Update failed!\n"+json.msg)
					  	$(tr1).loading("stop");
					  })
					  .fail(function( jqxhr, textStatus, error ) {
					    var err = textStatus + ", " + error;
					    alert("Error updating "+err);
					    $(this).loading("stop");
					});			

				});

				//$("#table-applicants tbody").on("click", ".applicant-del", function() {
				function app_del( s )
				{
			        var tr = $(s).closest("tr");
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

				}
				function add_referral( s )
				{
			        var tr = $(s).closest("tr");
			        var row = table.row( tr );
			        var id = row.data().aid;
					
					window.location.href = "/referral/'.arg(1).'/add?aid="+id;					
				}
				function add_center_referral( s )
				{
			        var tr = $(s).closest("tr");
			        var row = table.row( tr );
			        var id = row.data().aid;
					
					window.location.href = "/center-referral/'.arg(1).'/add?aid="+id;					
				}

				$(document).on("change", "#centre-select", function(){

					  ///alert("I am called");
					$.getJSON( "/get-courses/"+$(this).val(), {} )
					  .done(function( courses ) {
					        $(".course-select").empty();
					        var i = 0;
					        for(l = courses.length; i < l; i++) 
					        {
					        	$(".course-select").append($("<option>", { value : courses[i].id }).text( courses[i].name ));
					        }

					  })
					  .fail(function( jqxhr, textStatus, error ) {
					    var err = textStatus + ", " + error;
					    alert("Error getting courses "+err);
					  });			
				});

				//$("#table-applicants tbody").on("click", ".course-transfer,.applicant-add", function() {
				$("#table-applicants tbody").on("change", "#action-type", function() {
					var s = $(this);
					//var action = s.attr("action");
					var action = $(this).val();					
			        var tr = $(this).closest("tr");
			        var row = table.row( tr );
			        var centre_id = row.data().centreid;
			        console.log(action);
			        if ( action == "" )
			        	return;
			        if (action == "Add Referral")
			        {
			        	add_referral(s);
			        	return;
			        }
			        if (action == "Add Center Referral")
			        {
			        	add_center_referral(s);
			        	return;
			        }
			        if ( action == "Delete" )
			        {
			        	app_del(s);
			        	return;
			        }
			        var comment_transfer = "";
			        if ( action == "Transfer")
			        	comment_transfer = "<label>Comment</label><br><textarea name=\"comment\" id=\"trf-comment\" size=\"30\"></textarea>";
					$.confirm({
					    title: "",
					    content: "" +
					    "<form action=\"\" class=\"formName\">" +
					    "<div class=\"form-group\">" +
					    "<label>"+action+" "+row.data().name+" To Centre</label>" +
					    "<select class=\"centre-select form-control\" name=\"centre_select\" id=\"centre-select\" required><option value=\"\">Choose</option></select>" + "<label>To Course </label><select class=\"course-select form-control\" name=\"course_select\" required><option value=\"\">Choose</option></select>" +
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
					            	var centre = this.$content.find(".centre-select").val();
					                var centre_txt = this.$content.find(".centre-select :selected").text();
					                var c = this.$content.find("#trf-comment").val();

					                if(!course){
					                    $.alert("Please select a course");
					                    return false;
					                }
							        var tr = s.closest("tr");
							        var row = table.row( tr );
		        					var id = row.data().aid;
		        					if ( course == row.data().courseid && action == "Transfer")
		        					{
		        						$.alert("Source and Destination course cannot be the same!");
		        						return false;	
		        					}
		        					$(tr).loading();
					                //$.alert("Your selected " + course + " - " + row.data().aid );
					                if ( action == "Transfer")
					                {
										$.getJSON( "/move-to-course/"+id+"/"+course+"/"+centre, {c: c} )
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
										location.href = "/app/add/"+centre+"/"+course+"?ref="+id;					                	
					                }
					            }
					        },
					        cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to event_set(event, fd, events, callback)
					        var jc = this;
					        $(".course-select").empty();
					        var i = 0;
					        for(l = courses.length; i < l; i++) 
					        {
					        	$(".course-select").append($("<option>", { value : courses[i].id }).text( courses[i].name ));
					        }
					        i = 0;
					        for(l = centres.length; i < l; i++) 
					        {
				     	 		$(".centre-select").append($("<option>", { value : centres[i].id }).text( centres[i].name ));
					     	}		
					        $(".centre-select").val(centre_id);
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
					        });
					    }
					});

				});



        $("body").on("click", ".get-at-info", function() {
          teacher_id = $(this).attr("id");
          $.getJSON( "/at-schedule/get-at-info/"+teacher_id )
            .done(function( json ) {
              if (json.status == "OK")
              {
                $.alert(
                  "<b><u>"+json.at_name+" ( "+json.at_year+" )</b></u><br><br>"+
                  "<b>Status:</b> "+json.at_status+"<br>"+
                  "<b>Mobile:</b> "+json.at_phone+"<br>"+
                  "<b>Email:</b> "+json.at_email+"<br>"+
                  "<b>Address:</b> "+json.at_address+"<br>"
                );
              }
              else
                $.alert("Not able to get AT Info")
            })
            .fail(function( jqxhr, textStatus, error ) {
            var err = textStatus + ", " + error;
            $.alert("Not able to get AT Info "+err);
            });
          return false;
        });


			});

		})(jQuery);

	';
	drupal_add_js($js, 'inline');	
	return $out;
}

function dh_manageapp_search_form($form, &$form_state, $edit_data = array())
{
	global $user;
	$centre_id = '';
	if (arg(1) <> '')
		$centre_id = arg(1);

  if (arg(0) == "bulk-mail")
  {
    $update_bm = 1;
    $bm_id = arg(2);
    $edit_data_status = explode( ', ', $edit_data['status'] );
    $edit_data['status'] = array_combine($edit_data_status, $edit_data_status);
  }

	//$q = "select td_val1, td_val1 from dh_type_detail where td_key='Appl_stat' order by td_val1";
	$q = "select td_key, td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statuses = db_query($q)->fetchAllKeyed();

  $q = "select td_key, td_key from dh_type_detail where td_type='COURSE-LC-STATUS'";
  $system_lc_statuses = db_query($q)->fetchAllKeyed();

	$q = "select td_key, td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$statuses = db_query($q)->fetchAllKeyed();
//	$statuses[''] = '';

	$statuses[$system_statuses['STATUS-RECEIVED']] = $system_statuses['STATUS-RECEIVED'];
	$statuses[$system_statuses['STATUS-LEFT']] = $system_statuses['STATUS-LEFT'];
	$statuses[$system_statuses['STATUS-CONFIRMED']] = $system_statuses['STATUS-CONFIRMED'];
	$statuses[$system_statuses['STATUS-CANCELLED']] = $system_statuses['STATUS-CANCELLED'];
	$statuses[$system_statuses['STATUS-CLARIFICATIONRESPONSE']] = $system_statuses['STATUS-CLARIFICATIONRESPONSE'];
	$statuses[$system_statuses['STATUS-ATTENDED']] = $system_statuses['STATUS-ATTENDED'];
  $statuses[$system_statuses['STATUS-EXPECTED']] = $system_statuses['STATUS-EXPECTED'];
  $statuses[$system_statuses['STATUS-RECONFIRMATION']] = $system_statuses['STATUS-RECONFIRMATION'];
  $statuses[$system_lc_statuses['A-ATReview']] = $system_lc_statuses['A-ATReview'];
  $statuses[$system_lc_statuses['R-ATReview']] = $system_lc_statuses['R-ATReview'];
  $statuses[$system_lc_statuses['R-ATTransfer']] = $system_lc_statuses['R-ATTransfer'];
//	$statuses = db_query($q)->fetchAllKeyed();
//	$statuses[''] = 'Choose';

	if ( isset($form_state['storage']) )
	{
		$storage = $form_state['storage'];
		if( $centre_id == '')
			$centre_id = $storage['centre'];

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

		if ( isset($storage['status']) && count($storage['status']) > 0 )
		{
			if ( in_array( 'Attended',  $storage['status']) )
      {
        $where .= " and ( a_attended = '1' or  a_status in ( '".(implode("', '",$storage['status']))."' ) ) ";
      }
      else
      {
        $where .=  " and a_status in ('".(implode("', '",$storage['status']))."'".")";
      }
		}

		if ($storage['type'] <> 'Both')
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

		if ( isset($storage['pin']) && ($storage['pin'] <> '') )
    {
      $temp = explode("\r\n", $storage['pin']);
			$where .=  " and a_zip in ('".trim(implode("', '",$temp))."')";
    }

		if ( isset($storage['city']) && ( $storage['city'] <> '') )
		{
			$temp = explode("\r\n", $storage['city']);
			$where .=  " and (ci.c_name in ('".trim(implode("', '",$temp))."'".") or a_city_str in ('".trim(implode("', '",$temp))."'".")) ";
		}
		if ( isset($storage['occupation']) && ( $storage['occupation'] <> '') )
		{
			$temp = explode("\r\n", $storage['occupation']);
			$where .=  " and a_occupation in ('".trim(implode("', '",$temp))."'".")";
		}
		if ( isset($storage['designation']) && ( $storage['designation'] <> '') )
		{
			$temp = explode("\r\n", $storage['designation']);
			$where .=  " and a_designation in ('".trim(implode("', '",$temp))."'".")";
		}
		if ( isset($storage['company']) && ( $storage['company'] <> '') )
		{
			$temp = explode("\r\n", $storage['company']);
			$where .=  " and a_company in ('".trim(implode("', '",$temp))."'".")";
		}
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
    if ( isset($storage['least_10_day']) && ($storage['least_10_day']) )
      $where .= " and ac.ac_10d >= {$storage['least_10_day']} ";
    if ( isset($storage['least_stp']) && ($storage['least_stp']) )
      $where .= " and ac.ac_stp >= {$storage['least_stp']} ";
    if ( isset($storage['least_spl']) && ($storage['least_spl']) )
      $where .= " and ac.ac_spl >= {$storage['least_spl']} ";
    if ( isset($storage['least_20_day']) && ($storage['least_20_day']) )
      $where .= " and ac.ac_20d >= {$storage['least_20_day']} ";
    if ( isset($storage['least_30_day']) && ($storage['least_30_day']) )
      $where .= " and ac.ac_30d >= {$storage['least_30_day']} ";
    if ( isset($storage['least_45_day']) && ($storage['least_45_day']) )
      $where .= " and ac.ac_45d >= {$storage['least_45_day']} ";
    if ( isset($storage['least_60_day']) && ($storage['least_60_day']) )
      $where .= " and ac.ac_60d >= {$storage['least_60_day']} ";
    if ( isset($storage['least_service']) && ($storage['least_service']) )
      $where .= " and ac.ac_service >= {$storage['least_service']} ";

    if ( isset($storage['max_10_day']) && ($storage['max_10_day']) )
      $where .= " and ac.ac_10d <= {$storage['max_10_day']} ";
    if ( isset($storage['max_stp']) && ($storage['max_stp']) )
      $where .= " and ac.ac_stp <= {$storage['max_stp']} ";
    if ( isset($storage['max_spl']) && ($storage['max_spl']) )
      $where .= " and ac.ac_spl <= {$storage['max_spl']} ";
    if ( isset($storage['max_20_day']) && ($storage['max_20_day']) )
      $where .= " and ac.ac_20d <= {$storage['max_20_day']} ";
    if ( isset($storage['max_30_day']) && ($storage['max_30_day']) )
      $where .= " and ac.ac_30d <= {$storage['max_30_day']} ";
    if ( isset($storage['max_45_day']) && ($storage['max_45_day']) )
      $where .= " and ac.ac_45d <= {$storage['max_45_day']} ";
    if ( isset($storage['max_60_day']) && ($storage['max_60_day']) )
      $where .= " and ac.ac_60d <= {$storage['max_60_day']} ";
    if ( isset($storage['max_service']) && ($storage['max_service']) )
      $where .= " and ac.ac_service <= {$storage['max_service']} ";


		$out = '<h2>'.l("Back to Search", "search-app/".$centre_id).'</h2>';
		//if ($storage['course'] <> '')
		//	$out .= "&nbsp;&nbsp;|&nbsp;&nbsp;".l("Add Applicant", "app/add/$centre_id/".$storage['course']);
		//$out .= '</h2>';
		$storage['db_type'] = 'a';
		if ( $storage['db_type'] == 'a' )
			$db_type = 'applicant';
		else
			$db_type = 'student';

    if($storage['bulk-mail'])
    {
        $schedule_date = $storage['bulk-mail-schedule']['date'];
        if($storage['bm_id'])
        {
          dh_create_edit_bulk_mail_schedule($storage['bulk_mail_name'], $where, $storage['letters'], $schedule_date, $storage, $storage['bm_id']);
          drupal_set_message("Scheduled Bulk Email Updated.", 'status');
        }
        else
        {
          dh_create_edit_bulk_mail_schedule($storage['bulk_mail_name'], $where, $storage['letters'], $schedule_date, $storage);
          drupal_set_message("Scheduled Bulk Email.", 'status');
        }
        drupal_goto("centre/$centre_id/bulk-mail-schedule");
    }

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
		$q = "select c_id, c_name from dh_course where c_start >= '$max_old_courses' $centre_append and c_deleted=0 order by c_start asc";
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
		$form['course'] = array('#title' => 'Course', '#type' => 'select','#default_value' => $update_bm?$edit_data['course']:'', '#options' => $courses, '#weight' => 1);
		/*$form['db_type'] = array('#title' => 'DB Type', '#type' => 'radios', '#options' => array('s' => 'Student', 'a' => 'Applicant' ), '#default_value' => 'a',  '#weight' => 2);*/
		$form['f_name'] = array('#title' => 'First Name', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['f_name']:'', '#weight' => 3);
		$form['l_name'] = array('#title' => 'Last Name', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['l_name']:'', '#weight' => 4 );
		$form['conf_no'] = array('#title' => 'Confirmation No', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['conf_no']:'', '#size' => 15, '#weight' => 5 );
		$form['status'] = array('#title' => 'Status', '#type' => 'select', '#default_value' => $update_bm?$edit_data['status']:'', '#options' => $statuses, '#multiple' => "multiple", '#weight' => 6, '#attributes' => array('id' => 'edit-app-status'));
		$form['type'] = array('#title' => 'Applicant Type', '#type' => 'radios', '#options' => array('Student' => 'Applicant', 'Sevak' => 'Sevak', 'Both' => 'Both'), '#default_value' => $update_bm?$edit_data['type']:'Both',  '#weight' => 7);
		$form['gender'] = array('#title' => 'Gender', '#type' => 'select', '#options' => array( '' => 'Choose', 'M' => 'Male', 'F' => 'Female'), '#default_value' => $update_bm?$edit_data['gender']:'',  '#weight' => 8);
		$form['country'] = array('#title' => 'Country', '#type' => 'select', '#options' => $countries, '#default_value' => $update_bm?$edit_data['country']:'', '#weight' => 9);
		//$form['state'] = array('#title' => 'State', '#type' => 'select', '#options' => $states, '#weight' => 4);
		$form['state'] = array('#title' => 'State', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['state']:'','#weight' => 10);
		$form['email'] = array('#title' => 'Email', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['email']:'', '#weight' => 11);
		$form['mode'] = array('#title' => 'Application Mode', '#type' => 'select', '#default_value' => $update_bm?$edit_data['mode']:'', '#options' => $modes, '#weight' => 12 );
		$form['alist'] = array('#title' => '&nbsp;AList?', '#type' => 'checkbox', '#default_value' => $update_bm?$edit_data['alist']:'0',  '#weight' => 13);
		$form['is_at'] = array('#title' => '&nbsp;Is Teacher?', '#type' => 'checkbox', '#default_value' => $update_bm?$edit_data['is_at']:'0',  '#weight' => 14);
		$form['is_foreign'] = array('#title' => '&nbsp;Is Foreign National?', '#type' => 'checkbox', '#default_value' => $update_bm?$edit_data['is_foreign']:'0',  '#weight' => 15);
		$form['is_willing_to_serve'] = array('#title' => '&nbsp;Willing To Serve?', '#type' => 'checkbox', '#default_value' => $update_bm?$edit_data['is_willing_to_serve']:'0',  '#weight' => 16);
		$form['is_pregnant'] = array('#title' => '&nbsp;Is Pregnant?', '#type' => 'checkbox', '#default_value' => $update_bm?$edit_data['is_pregnant']:'0',  '#weight' => 17);
		$form['mobile'] = array('#title' => 'Mobile', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['mobile']:'',  '#weight' => 18);
		$form['phone_home'] = array('#title' => 'Phone Home', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['phone_home']:'', '#weight' => 19);
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

		if ($centre_id <> '')
		{		
  		$group = 'Advanced';
  		$form['#attributes'] = array('class' => array('container-inline'));
  		$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false, '#weight' => 98);
      $form[$group]['pin'] = array('#title' => 'Pin Code', '#type' => 'textarea', '#default_value' => $update_bm?$edit_data['pin']:'', '#size' => '15', '#weight' => 1, '#description' => 'Enter values in separate lines');
			$form[$group]['city'] = array('#title' => 'City', '#type' => 'textarea', '#default_value' => $update_bm?$edit_data['city']:'', '#size' => '15', '#weight' => 2, '#description' => 'Enter values in separate lines');
			$form[$group]['occupation'] = array('#title' => 'Occupation', '#type' => 'textarea', '#default_value' => $update_bm?$edit_data['occupation']:'', '#size' => '15', '#weight' => 3, '#description' => 'Enter values in separate lines');
			$form[$group]['designation'] = array('#title' => 'Designation', '#type' => 'textarea', '#default_value' => $update_bm?$edit_data['designation']:'', '#size' => '15', '#weight' => 4, '#description' => 'Enter values in separate lines');
			$form[$group]['company'] = array('#title' => 'Company', '#type' => 'textarea', '#default_value' => $update_bm?$edit_data['company']:'', '#size' => '15', '#weight' => 5, '#description' => 'Enter values in separate lines');
      $form[$group]['least_10_day'] = array('#title' => 'Least number of 10 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_10_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '200' ), '#size' => '15', '#weight' => 5.1, );
      $form[$group]['least_stp'] = array('#title' => 'Least number of STP', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_stp']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.2, );
      $form[$group]['least_spl'] = array('#title' => 'Least number of SPL', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_spl']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.3, );
      $form[$group]['least_20_day'] = array('#title' => 'Least number of 20 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_20_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.4, );
      $form[$group]['least_30_day'] = array('#title' => 'Least number of 30 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_30_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.5, );
      $form[$group]['least_45_day'] = array('#title' => 'Least number of 45 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_45_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100'), '#size' => '15', '#weight' => 5.6, );
      $form[$group]['least_60_day'] = array('#title' => 'Least number of 60 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_60_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100'), '#size' => '15', '#weight' => 5.7, );
      $form[$group]['least_service'] = array('#title' => 'Least number of Seva', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['least_service']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '200'), '#size' => '15', '#weight' => 5.8, );

      $form[$group]['max_10_day'] = array('#title' => 'Max number of 10 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_10_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '300' ), '#size' => '15', '#weight' => 5.81, );
      $form[$group]['max_stp'] = array('#title' => 'Max number of STP', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_stp']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.82, );
      $form[$group]['max_spl'] = array('#title' => 'Max number of SPL', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_spl']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.83, );
      $form[$group]['max_20_day'] = array('#title' => 'Max number of 20 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_20_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.84, );
      $form[$group]['max_30_day'] = array('#title' => 'Max number of 30 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_30_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100' ), '#size' => '15', '#weight' => 5.85, );
      $form[$group]['max_45_day'] = array('#title' => 'Max number of 45 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_45_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100'), '#size' => '15', '#weight' => 5.86, );
      $form[$group]['max_60_day'] = array('#title' => 'Max number of 60 day', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_60_day']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '100'), '#size' => '15', '#weight' => 5.87, );
      $form[$group]['max_service'] = array('#title' => 'Max number of Seva', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['max_service']:'', '#attributes' => array(' type' => 'number', 'min' => '0', 'max' => '200'), '#size' => '15', '#weight' => 5.88, );
		}

    if (user_access('mass mail') && $centre_id <> '')
    {
      $group = 'Bulk-Mail';
      $form['#attributes'] = array('class' => array('container-inline'));
      $form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false, '#weight' => 99);
      if($update_bm)
      {
        $form['bulk-mail'] = array(
          '#type' => 'hidden',
          '#value' => 1,
        );
        $form['bm_id'] = array(
          '#type' => 'hidden',
          '#value' => $bm_id,
        );
      }
      else
      {
        $form[$group]['bulk-mail'] = array('#title' => '&nbsp;Bulk-Mail to Search Result?', '#type' => 'checkbox', '#default_value' => '0',  '#weight' => 1);
      }

      $form[$group]['bulk_mail_name'] = array('#title' => 'Bulk Mail Batch Name', '#type' => 'textfield', '#default_value' => $update_bm?$edit_data['bulk_mail_name']:'', '#size' => '15', '#weight' => 5.9, '#description' => 'Give a name to this bulk mail batch');
      $q = "select l_id, l_name from dh_letter where l_center = $centre_id and l_event='custom'";
      $letters = db_query($q)->fetchAllKeyed();
      $letters[''] = 'Choose';
      $form[$group]['letters'] = array('#title' => 'Custom Letter to Send', '#type' => 'select', '#default_value' => $update_bm?$edit_data['letters']:'', '#options' => $letters, '#weight' => 6);
      $form[$group]['bulk-mail-schedule'] = array('#title' => '&nbsp;Bulk-Mail Date?', '#type' => 'date_popup', '#default_value' => $update_bm?$edit_data['bulk-mail-schedule']:'',
        '#date_format' => 'Y-m-d',
        '#default_value' => date('Y-m-d'),
        '#date_year_range' => '+0:+1',
    '#datepicker_options' => array(
        'dateFormat' => date_popup_format_to_popup('Y-m-d'),
      ),
      '#date_label_position' => 'above',
    '#theme_wrappers' => array('date_popup'),
    '#suffix' => '',
        '#weight' => 7);
    }

		$js = '
			(function ($) {
				$(document).ready(function(){
					$("#edit-course").select2();
					$("#edit-country").select2();
          $("#edit-app-status").select2();
					$("#edit-f-name").focus();

          var old_status_values = [];
          var status = $("#edit-app-status");
          status.on("select2:select", function(event) {
            console.log("inside status select");
            var values = [];
            // copy all option values from selected
            $(event.currentTarget).find("option:selected").each(function(i, selected){
              values[i] = $(selected).text();
            });
            // doing a diff of old_values gives the new values selected
            var last = $(values).not(old_status_values).get();
            // update old_values for future use
            old_status_values = values;


            if($.inArray("Confirmed", last) !== -1)
            {
              console.log("inside last confirm");
              if($.inArray("Expected", values) == -1)
              {
                console.log("inside last confirm Expected");
                values.push("Expected");
              }
              if($.inArray("ReConfirmation", values) == -1)
              {
                console.log("inside last confirm ReConfirmation");
                values.push("ReConfirmation");
              }
              status.val(values).trigger("change");
            }

            // output values (all current values selected)
            console.log("selected values: ", values);
            // output last added value
            console.log("last added: ", last);
          });


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
  	if ( $count == 0 && !$values['bulk-mail'])
  		form_set_error('course', 'Please select a Course or Name or Conf No or Email');

    if($values['bulk-mail'])
    {
      if(!$values['letters'])
        form_set_error('letters', 'Letter must be selected in case of bulk-mail');

      if(!$values['bulk_mail_name'])
        form_set_error('bulk_mail_name', 'Bulk Mail Batch Name is must.');
    }
}

function dh_manageapp_search_form_submit($form, &$form_state)
{
	$form_state['storage'] = $form_state['input'];
	$form_state['rebuild'] = TRUE;
//	drupal_goto("donation-report-results", array('query' => $data));
}

function dh_search_lc ($type)
{
	global $user;
	$res = db_query("select uc_center from dh_user_center where uc_user='".$user->uid."'  and uc_deleted=0");
	$centres = array();
	while($r = $res->fetchAssoc())
	{
		$centres[] = $r['uc_center'];
	}
	$where = " a_center in (".implode(",", $centres).") and a_status in ('".$type."')";
	$out = dh_manageapp_search_results( 'applicant', $where, $centres[0], True );

	$form['out'] = array('#markup' => $out, '#weight' => 100 );
	return $form;
}


function dh_manageapp_search_show_app()
{
  $center_id = arg(1);
  $bulk_mail_id = arg(2);
  $app_id = arg(3);
  $where = " a_id='$app_id' and a_center='$center_id' ";
  $out = '<h3>'.l("Back to Bulk-Mail-Log", "bulk-mail/$center_id/$bulk_mail_id/show-log").' | '.l("Back to Bulk-Mail-Schedule", "centre/$center_id/bulk-mail-schedule").' | '.l("Back to Dashboard", "centre/$center_id").'</h3>';
  $out .= dh_manageapp_search_results( 'applicant', $where, $center_id );
  return $out;
}
