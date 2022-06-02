<?php


function create_pdf_from_html($data, $outfile)
{
	$input = tempnam("/tmp", "DIPI-HTML").".html";
	$handle = fopen($input, "w");
	fwrite($handle, $data);
	fclose($handle);

	$cmd = "/usr/local/bin/wkhtmltopdf $input $outfile";
	exec($cmd);
	unlink($input);
}

function fdf_create($filename, $fields)
{
	$Out = "%FDF-1.2\n%????\n";
	$Out .= "1 0 obj \n<< /FDF << /Fields [\n";

	foreach ($fields as $key => $val )
    {
	     $val = str_replace("\r\n","\n",$val);
	     $val = str_replace("\r"  ,"\n",$val);
	     $Out.= "<< /T (".addcslashes($key,"\n\r\t\\()").") /V (".addcslashes($val,"\n\r\t\\()").") >> \n";
    }


	$Out.= "]\n/F () >>";

	$Out.= ">>\nendobj\ntrailer\n<<\n";
	$Out.= "/Root 1 0 R \n\n>>\n";
	$Out.= "%%EOF";

	file_put_contents($filename, $Out);

}

function pdf_fill_form($fdf, $pdf, $out)
{
	if (is_array($fdf))
	{
		$fdf_file = tempnam(sys_get_temp_dir(), gethostname());
		fdf_create($fdf_file, $fdf);
	}
	else
		$fdf_file = $fdf;
	$cmd = "pdftk $pdf fill_form $fdf_file output $out flatten";
	//drupal_set_message($cmd);
	exec($cmd);

	unlink( $fdf_file );
}

function create_d11_report_pdf( $course_id )
{
	$q = "select ce.c_name as 'CentreName', ce.c_id as 'CentreID', ce.c_address, ce.c_pincode, ce.c_state, ce.c_country, ci.c_name as 'City', c_phone, co.c_name as 'CourseName', c_start  from dh_course co left join dh_center ce on co.c_center=ce.c_id left join dh_city ci on ce.c_city=ci.c_id where co.c_id=$course_id";
	$course_det = db_query($q)->fetchAssoc();
	$temp = explode("/", $course_det['CourseName']);
	$centre_name = trim($temp[0]);
	$course_type = trim($temp[1]);
	$course_date = trim($temp[3]).' ('.$temp[2].')';
	$address = $course_det['c_address'].', '.$course_det['City'].', '.$course_det['c_state'].', '.$course_det['c_country'].' - '.$course_det['c_pincode'];
	$centre_id = $course_det['CentreID'];
	$t = explode("-", $course_det['c_start']);
	$year = $t[0];

	$q = "select ct_type, ct_group, t_code, CONCAT(t_f_name, ' ',t_l_name) as 't_name', t_gender from dh_course_teacher ct left join dh_teacher t on ct_teacher=t_id where ct_course='$course_id' and ct_status='Confirmed'";
	$t_hand = db_query($q);
	$teacher = array();
	while($t_r = $t_hand->fetchAssoc())
	{
		//$teacher[strtoupper($t_r['t_gender'])][$t_r['ct_group']] = $t_r;
		$teacher[strtoupper($t_r['t_gender'])][] = $t_r;
	}
	$q = "select a_type, a_gender, a_old, a_country, count(a_id) as 'tot' from dh_applicant where a_course=$course_id and a_attended=1 group by a_type, a_gender, a_old, a_country";
	$hand = db_query($q);
	$totals['STUDENT']['M'][0]['IN'] = $totals['STUDENT']['M'][1]['IN'] = $totals['STUDENT']['F'][0]['IN'] = $totals['STUDENT']['F'][1]['IN'] = 0;
	$totals['STUDENT']['M'][0]['NON-IN'] = $totals['STUDENT']['M'][1]['NON-IN'] = $totals['STUDENT']['F'][0]['NON-IN'] = $totals['STUDENT']['F'][1]['NON-IN'] = 0;
	$totals['SEVAK']['M'][1]['IN'] = $totals['SEVAK']['F'][1]['IN'] = $totals['SEVAK']['M'][1]['NON-IN'] = $totals['SEVAK']['F'][1]['NON-IN'] = 0;
	while( $row = $hand->fetchAssoc())
	{
		if ( in_array(strtoupper($row['a_country']), array('IN', '') ) )
			$totals[strtoupper($row['a_type'])][strtoupper($row['a_gender'])][$row['a_old']]['IN'] += $row['tot'];
		else
			$totals[strtoupper($row['a_type'])][strtoupper($row['a_gender'])][$row['a_old']]['NON-IN'] += $row['tot'];
	}

	$q = "select a_type, a_gender, a_old, a_country, count(a_id) as 'tot' from dh_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course=$course_id and (aa_left=1 or a_status='LEFT') group by a_type, a_gender, a_old, a_country";
	$hand = db_query($q);
	$left['STUDENT']['M'][0]['IN'] = $left['STUDENT']['M'][1]['IN'] = $left['STUDENT']['F'][0]['IN'] = $left['STUDENT']['F'][1]['IN'] = 0;
	$left['STUDENT']['M'][0]['NON-IN'] = $left['STUDENT']['M'][1]['NON-IN'] = $left['STUDENT']['F'][0]['NON-IN'] = $left['STUDENT']['F'][1]['NON-IN'] = 0;
	$left['SEVAK']['M'][1]['IN'] = $left['SEVAK']['F'][1]['IN'] = $left['SEVAK']['M'][1]['NON-IN'] = $left['SEVAK']['F'][1]['NON-IN'] = 0;
	while( $row = $hand->fetchAssoc())
	{
		if ( in_array(strtoupper($row['a_country']), array('IN', '') ) )
			$left[strtoupper($row['a_type'])][strtoupper($row['a_gender'])][$row['a_old']]['IN'] += $row['tot'];
		else
			$left[strtoupper($row['a_type'])][strtoupper($row['a_gender'])][$row['a_old']]['NON-IN'] += $row['tot'];
	}

	$fields = array();
	$fields['form1[0].#subform[0].Header[0].cname[0]'] = $centre_name;
	$fields['form1[0].#subform[0].cnumber[0]'] = 'NA';
	$fields['form1[0].#subform[0].Header[0].cdate[0]'] = $course_date;
	$fields['form1[0].#subform[0].ctype[0]'] = $course_type;
	$fields['form1[0].#subform[0].address[0]'] = $address;
	$i = 1;
	foreach( $teacher['M'] as $t )
	{
		$fields['form1[0].#subform[0].Table1[0].Row'.($i+1).'[0].M'.$i.'[0]'] = $t['t_name'].' ('.$t['t_code'].') - '.$t['ct_type'];
		$i++;		
	}
	$i = 1;
	foreach( $teacher['F'] as $t )
	{
		$fields['form1[0].#subform[0].Table1[0].Row'.($i+1).'[0].F'.$i.'[0]'] = $t['t_name'].' ('.$t['t_code'].') - '.$t['ct_type'];
		$i++;		
	}

	$fields['form1[0].#subform[0].Table2[0].Row2[0].IN1[0]'] = $totals['STUDENT']['M'][0]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row2[0].IO1[0]'] = $totals['STUDENT']['M'][1]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row2[0].FN1[0]'] = $totals['STUDENT']['M'][0]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row2[0].FO1[0]'] = $totals['STUDENT']['M'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row2[0].DS1[0]'] = $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row2[0].T1[0]'] = $totals['STUDENT']['M'][0]['IN'] + $totals['STUDENT']['M'][1]['IN'] + $totals['STUDENT']['M'][0]['NON-IN'] + $totals['STUDENT']['M'][1]['NON-IN'] + $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'];


	$fields['form1[0].#subform[0].Table2[0].Row3[0].IN2[0]'] = $totals['STUDENT']['F'][0]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row3[0].IO2[0]'] = $totals['STUDENT']['F'][1]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row3[0].FN2[0]'] = $totals['STUDENT']['F'][0]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row3[0].FO2[0]'] = $totals['STUDENT']['F'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row3[0].DS2[0]'] = $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row3[0].T2[0]'] = $totals['STUDENT']['F'][0]['IN'] + $totals['STUDENT']['F'][1]['IN'] + $totals['STUDENT']['F'][0]['NON-IN'] + $totals['STUDENT']['F'][1]['NON-IN'] + $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];


	$fields['form1[0].#subform[0].Table2[0].Row4[0].IN3[0]'] = $totals['STUDENT']['M'][0]['IN'] + $totals['STUDENT']['F'][0]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row4[0].IO3[0]'] = $totals['STUDENT']['M'][1]['IN'] + $totals['STUDENT']['F'][1]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row4[0].FN3[0]'] = $totals['STUDENT']['M'][0]['NON-IN'] + $totals['STUDENT']['F'][0]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row4[0].FO3[0]'] = $totals['STUDENT']['M'][1]['NON-IN'] + $totals['STUDENT']['F'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row4[0].DS3[0]'] = $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'] + $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];;
	$fields['form1[0].#subform[0].Table2[0].Row4[0].T3[0]'] = $totals['STUDENT']['M'][0]['IN'] + $totals['STUDENT']['F'][0]['IN'] + $totals['STUDENT']['M'][1]['IN'] + $totals['STUDENT']['F'][1]['IN'] + $totals['STUDENT']['M'][0]['NON-IN'] + $totals['STUDENT']['F'][0]['NON-IN'] + $totals['STUDENT']['M'][1]['NON-IN'] + $totals['STUDENT']['F'][1]['NON-IN'] + $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'] + $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];


	$fields['form1[0].#subform[0].Table2[0].Row5[0].IN4[0]'] = $left['STUDENT']['M'][0]['IN'] + $left['STUDENT']['F'][0]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row5[0].IO4[0]'] = $left['STUDENT']['M'][1]['IN'] + $left['STUDENT']['F'][1]['IN'];
	$fields['form1[0].#subform[0].Table2[0].Row5[0].FN4[0]'] = $left['STUDENT']['M'][0]['NON-IN'] + $left['STUDENT']['F'][0]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row5[0].FO4[0]'] = $left['STUDENT']['M'][1]['NON-IN'] + $left['STUDENT']['F'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row5[0].DS4[0]'] = $left['SEVAK']['M'][1]['IN'] + $left['SEVAK']['F'][1]['IN'] + $left['SEVAK']['M'][1]['NON-IN'] + $left['SEVAK']['F'][1]['NON-IN'];
	$fields['form1[0].#subform[0].Table2[0].Row5[0].T4[0]'] = $left['STUDENT']['M'][0]['IN'] + $left['STUDENT']['F'][0]['IN'] + $left['STUDENT']['M'][1]['IN'] + $left['STUDENT']['F'][1]['IN'] + $left['STUDENT']['M'][0]['NON-IN'] + $left['STUDENT']['F'][0]['NON-IN'] + $left['STUDENT']['M'][1]['NON-IN'] + $left['STUDENT']['F'][1]['NON-IN'] + $left['SEVAK']['M'][1]['IN'] + $left['SEVAK']['F'][1]['IN'] + $left['SEVAK']['M'][1]['NON-IN'] + $left['SEVAK']['F'][1]['NON-IN'];


	$template = drupal_get_path("module", "dh_manageapp")."/templates/report-d11-template.pdf";
	$uri = 'private://';
	$path= drupal_realpath($uri);
	$dir = $path."/reports/".$centre_id."/".$year;
	/*
	if (!is_dir($dir))
	{
		if (posix_getuid() == 0)
			posix_setuid(33);
		mkdir($dir, 0775, true);
	}
	*/	
	//$out_pdf = $dir."/report-Day11-".$course_id.".pdf";
	$out_pdf = tempnam("/var/dipi", "report-D11-")."-".$course_id.".pdf";	
	$out_uri = $uri."/reports/".$centre_id."/".$year."/report-Day11-".$course_id.".pdf";
	pdf_fill_form($fields, $template, $out_pdf);

	$ret = s3_put_file(variable_get('s3_bucket_name', ''), $out_pdf, str_replace("private:///", '', $out_uri) );
	if (!$ret['success'])
		watchdog('S3Pdf', $ret['message']);
	unlink($out_pdf);

	//chown($out_pdf, "www-data");
	//drupal_set_message($out_uri." called ");
	db_update('dh_course')->fields(array('c_uri' => $out_uri))->condition('c_id', $course_id)->execute();

	return str_replace("private:///", '', $out_uri);
}


function create_application_pdf( $application_id )
{
	$query = "select c.*, a.*, ac.*, ae.*, aa.*, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ci.c_name as 'City',td_val3, td_val1, al.* from dh_applicant a left join dh_course c on a.a_course=c.c_id left join dh_applicant_course ac on ac.ac_applicant=a.a_id left join dh_applicant_extra ae on ae.ae_applicant=a.a_id left join dh_applicant_attended aa on aa.aa_applicant=a.a_id left join dh_applicant_lc al on al.al_applicant=a.a_id left join dh_city ci on a_city=ci.c_id left join dh_type_detail td on c.c_course_type=td.td_id where a_id='$application_id'";
	$row = db_query($query)->fetchAssoc(); 
	if ($row['City'] == '')
		$row['City'] = $row['a_city_str'];
	if ( strtolower($row['td_val3']) == 'lc')
	{
		$fields = map_application_fields_lc($row);
		$template = drupal_get_path("module", "dh_manageapp")."/templates/lc-applicant-template.pdf";
	}
	elseif ( $row['a_type'] == 'Sevak')
	{

		$fields = map_application_fields($row);
		if ( $row['ae_seva_details'])
		{
		$fields['seva_details'] = $row['ae_seva_details']; 	
		$fields['seva_yes'] = 'Yes';	
		}
		else
			$fields['seva_no'] = 'Yes';

		unset($fields['psychological_yes']);
		unset($fields['psychological_no']);
		if ( $row['a_problem_mental'] || $row['a_problem_physical'])
		{
			$fields['psychological'] = $row['ae_desc_mental']."\n".$row['ae_desc_physical']."\n".$row['ae_desc_medication']; 
			$fields['psychological_yes'] = 'Yes';
		}
		else
			$fields['psychological_no'] = 'Yes';
			
		$template = drupal_get_path("module", "dh_manageapp")."/templates/seva-template.pdf";
	}

	elseif ( $row['td_val3'] == 'Teen' || $row['td_val3'] == 'Child')
	{

		$fields = map_application_fields($row);
		$fields['dad'] = $row['ae_father']; 	
		$fields['mom'] = $row['ae_mother'];
		$fields['comp_course'] = $row['ae_parent_course']?'Yes':'No';

		if ($row['ae_ch_course'] == 1)
			$fields['ccc_yes'] = 'Yes';
		else
			$fields['ccc_no'] = 'Yes';

		$fields['ccc_detail'] = $row['ae_ch_course_detail'];

		if ($row['ae_ch_practice'] == 1)
			$fields['pana_yes'] = 'Yes';
		else
			$fields['pana_no'] = 'Yes';

		$fields['pana_detail'] = $row['ae_ch_practice_detail'];
			
		$template = drupal_get_path("module", "dh_manageapp")."/templates/teenager-template.pdf";
	}

	else
	{
		$fields = map_application_fields($row);
		$template = drupal_get_path("module", "dh_manageapp")."/templates/applicant-template.pdf";
	}
	$uri = 'private://';
	$path= drupal_realpath($uri);
	$dir = $path."/applications/".$row['a_center']."/".$row['c_id'];
	/*
	if (!is_dir($dir))
	{
		if (posix_getuid() == 0)
			posix_setuid(33);
		mkdir($dir, 0775, true);		
	}*/
	//$out_pdf = $dir."/app-".$application_id.".pdf";
	$out_pdf = tempnam("/var/dipi", "app-")."-".$application_id.".pdf";
	$out_uri = $uri."/applications/".$row['a_center']."/".$row['c_id']."/app-".$application_id.".pdf";
	pdf_fill_form($fields, $template, $out_pdf);
	if ($row['a_photo'] <> '')
	{
		$photo_path = str_replace("private://", $path."/", $row['a_photo'] );
		$photo_path = str_replace("private://", '', $row['a_photo'] );
		$ret = s3_get_file(variable_get('s3_bucket_name', ''), $photo_path, 'file');
		if (!$ret['success'])
			watchdog('S3Pdf', $ret['message']);
		$type = (strtoupper($row['td_val3']) == 'LC')?'LC':'GEN';
		exec("/dhamma/scripts/merge-img ".$ret['data']." $type $out_pdf");		
		unlink($ret['data']) ;
	}
	//chown($out_pdf, "www-data");
	//drupal_set_message($out_uri." called ");
	$ret = s3_put_file(variable_get('s3_bucket_name', ''), $out_pdf, str_replace("private:///", '', $out_uri) );
	if (!$ret['success'])
		watchdog('S3Pdf', $ret['message']);
	unlink($out_pdf);
	db_update('dh_applicant')->fields(array('a_uri' => $out_uri))->condition('a_id', $application_id)->execute();
	return $out_uri;
}

function map_application_fields($row)
{
	$fields = array();
	$temp = explode("/", $row['c_name']);
	$fields['centre'] = trim($temp[0]);
	if ( $row['aa_acco'] )
		$fields['acc'] = $row['aa_section']."-".$row['aa_acco'];
	if ( $row['aa_group'] )
		$fields['group'] = $row['aa_group'];

	$fields['conf'] = $row['a_conf_no'];
	$fields['age'] = $row['Age'];

	if ( $row['a_old'] )  $fields['Old Student'] = 'Yes'; else	$fields['New Student'] = 'Yes';

	$fields['From'] = $row['c_start']; 	$fields['To'] = $row['c_end'];
	if ($row['a_gender'] == 'M')  $fields['Male'] = 'Yes';	else  $fields['Female'] = 'Yes';

	$temp = explode("-", $row['a_dob']);	
	$fields['dd'] = $temp[2]; 	$fields['mm'] = $temp[1];	$fields['yyyy'] = $temp[0];

	$fields['First Name'] = $row['a_f_name']; 	$fields['Last Name'] = $row['a_l_name'];	$fields['Middle Name'] = $row['a_m_name'];
	$fields['Address'] = $row['a_address'].', '.$row['City'].', '.$row['a_state'].', '.$row['a_country']; 	$fields['pincode'] = $row['a_zip'];
	$fields['home'] = $row['a_phone_home']; 	$fields['work'] = $row['a_phone_office'];	$fields['mobile'] = $row['a_phone_mobile']; 
	$fields['email'] = $row['a_email'];

	if ($row['a_aadhar']) { $fields['aadhar'] = 'Yes'; $fields['Photo ID'] = $row['a_aadhar']; }
	if ($row['a_passport']) { $fields['passport'] = 'Yes'; $fields['Photo ID'] = $row['a_passport']; }
	if ($row['a_voter_id']) { $fields['voter id'] = 'Yes'; $fields['Photo ID'] = $row['a_voter_id']; }
	if ($row['a_pancard']) { $fields['pan card'] = 'Yes'; $fields['Photo ID'] = $row['a_pancard']; }
//	$fields['Photo ID'] = ''; 	

	$occupation = strtolower($row['a_occupation']);
	if ( $row['a_occupation_past'] )
	{
		$occupation = strtolower($row['a_occupation_past']);
		$fields['past'] = 'Yes';		
	}
	else
	 	$fields['present'] = 'Yes'; // Occupation
	$arr_occ = array('doctor', 'lawyer', 'engineer', 'it', 'business', 'ca/acct.', 'ngo', 'student', 'defense', 'govt. (class-1)', 'govt. (class-2)', 'real estate', 'agriculture', 'teacher', 'politician' );
	if ($occupation == 'doctor') $fields['Doctor'] = 'Yes';
	if ($occupation == 'lawyer') $fields['lawyer'] = 'Yes'; 	
	if ($occupation == 'engineer') $fields['engg'] = 'Yes';	
	if ($occupation == 'it') $fields['it'] = 'Yes'; 
	if ($occupation == 'business') $fields['bus'] = 'Yes'; 
	if ($occupation == 'ca/acct.') $fields['ca'] = 'Yes';	
	if ($occupation == 'ngo') $fields['ngo'] = 'Yes';		
	if ($occupation == 'student') $fields['student'] = 'Yes'; 	
	if ($occupation == 'defense') $fields['defence'] = 'Yes'; 
	if ($occupation == 'govt. (class-1)') $fields['class1'] = 'Yes';	
	if ($occupation == 'govt. (class-2)') $fields['class2'] = 'Yes';
	if ($occupation == 'real estate') $fields['real est'] = 'Yes';
	if ($occupation == 'agriculture') $fields['agri'] = 'Yes';
	if ($occupation == 'teacher') $fields['teacher'] = 'Yes';	
	if ($occupation == 'politician') $fields['politician'] = 'Yes';
	if ( ! in_array($occupation, $arr_occ) ) 
	{
		$fields['other'] = 'Yes';
		$fields['Other occupation'] = $occupation;
	}

	$fields['Education'] = $row['a_education']; 	
	$fields['Name of Organization'] = $row['a_company'];		$fields['Designation'] = $row['a_designation'];

	$fields['spouse occupation'] = $row['a_occupation_relation'];
	if ($row['a_friend_family'])	
	{
		$fields['frnd_yes'] = 'Yes';
		$fields['friend or family'] = $row['a_friend_family_details'] ; // .'( '.$row['a_family_relation'].' )';
	}
	else
		$fields['frnd_no'] = 'Yes';	

	$fields['emergency contact info'] = $row['a_emergency_name'].'('.$row['a_emergency_relation'].') - '.$row['a_emergency_num'];

	//if ($row['a_lang_1']) $fields['hi_inter'] = 'Yes'; //else $fields['hi_none'] = 'Yes';	
	//$fields['hi_basic'] = ''; 		$fields['hi_exp'] = ''; 	

	//if ($row['a_lang_2']) $fields['eng_inter'] = 'Yes'; //else $fields['eng_none'] = 'Yes';
	//$fields['eng_basic'] = ''; 	  $fields['eng_exp'] = ''; 	

	$fields['lang1'] = $row['a_lang_1'];
	$fields['lang2'] = $row['a_lang_2'];
	$fields['lang3'] = $row['a_lang_3'];
	$fields['lang4'] = $row['a_lang_discourse'];

	if (strtolower($row['a_lang_1_level'])=='none')
		$fields['l1_none']='Yes';
	elseif (strtolower($row['a_lang_1_level'])=='basic')
		$fields['l1_basic']='Yes';
	elseif (strtolower($row['a_lang_1_level'])=='intermediate')
		$fields['l1_int']='Yes';
	elseif (strtolower($row['a_lang_1_level'])=='expert')
		$fields['l1_exp']='Yes';

	if (strtolower($row['a_lang_2_level'])=='none')
		$fields['l2_none']='Yes';
	elseif (strtolower($row['a_lang_2_level'])=='basic')
		$fields['l2_basic']='Yes';
	elseif (strtolower($row['a_lang_2_level'])=='intermediate')
		$fields['l2_int']='Yes';
	elseif (strtolower($row['a_lang_2_level'])=='expert')
		$fields['l2_exp']='Yes';

	if (strtolower($row['a_lang_3_level'])=='none')
		$fields['l3_none']='Yes';
	elseif (strtolower($row['a_lang_3_level'])=='basic')
		$fields['l3_basic']='Yes';
	elseif (strtolower($row['a_lang_3_level'])=='intermediate')
		$fields['l3_int']='Yes';
	elseif (strtolower($row['a_lang_3_level'])=='expert')
		$fields['l3_exp']='Yes';

	if (strtolower($row['a_lang_discourse_level'])=='none')
		$fields['l4_none']='Yes';
	elseif (strtolower($row['a_lang_discourse_level'])=='basic')
		$fields['l4_basic']='Yes';
	elseif (strtolower($row['a_lang_discourse_level'])=='intermediate')
		$fields['l4_int']='Yes';
	elseif (strtolower($row['a_lang_discourse_level'])=='expert')
		$fields['l4_exp']='Yes';

	if ($row['a_old'])
	{
		$fields['First Course Date'] = $row['ac_first_year'].'-'.$row['ac_first_month'].'-'.$row['ac_first_day']; 
		$fields['Location'] = $row['ac_first_location_str'];	$fields['Teachers'] = $row['ac_first_teacher_str'];

		$fields['Most Recent Course Sat  Date'] = $row['ac_last_year'].'-'.$row['ac_last_month'].'-'.$row['ac_last_day'];; 	
		$fields['Location_2'] = $row['ac_last_location_str'];	$fields['Teachers_2'] = $row['ac_last_teacher_str'];

		$fields['teen'] = $row['ac_teen'];

		$fields['10'] = $row['ac_10d'];		$fields['stp'] = $row['ac_stp'];	$fields['20'] = $row['ac_20d'];		$fields['spl'] = $row['ac_spl'];		
		$fields['tsc'] = $row['ac_tsc']; 	$fields['30'] = $row['ac_30d'];		$fields['45'] = $row['ac_45d'];		$fields['60'] = $row['ac_60d'];
		$fields['seva'] = $row['ac_service'];				
	}

	if ( $row['a_old'] )
	{
		if ($row['ac_practice_details'])
		{
			$fields['dailymed_yes'] = 'Yes';		
			$fields['If yes please give details how much time daily etc'] = $row['ac_practice_details'];
		}
		else
			$fields['dailymed_no'] = 'Yes'; 		
	}


	if ( $row['a_problem_physical'])
	{
		$fields['physical'] = $row['ae_desc_physical']."\n".$row['ae_desc_medication']; 	
		$fields['medical_yes'] = 'Yes';	
	}
	else
		$fields['medical_no'] = 'Yes';

	if ( $row['a_problem_mental'])
	{
		$fields['psychological'] = $row['ae_desc_mental']; 
		$fields['psychological_yes'] = 'Yes';
	}
	else
		$fields['psychological_no'] = 'Yes';

	if ( $row['a_addiction_current'])
	{
		$fields['current use intoxicants'] = $row['ae_desc_addiction_current']; 	
		$fields['curr_alco_yes'] = 'Yes';	
	}
	else
		$fields['curr_alco_no'] = 'Yes';	
	
	if ( $row['a_addiction_past'])
	{
		$fields['addiction'] = $row['ae_desc_addiction_past'];
		$fields['alco_yes'] = 'Yes';
	}
	else
		$fields['alco_no'] = 'Yes';

	if ($row['ae_pregnant_detail'])
		$fields['Text1'] = $row['ae_pregnant_detail']; // Pregnancy

	if ( $row['a_other_technique'] || ($row['a_other_technique_old']))
	{
		$fields['meditation_yes'] = 'Yes'; 
		$fields['reiki'] = $row['ae_desc_other_technique'].' '.$row['ae_desc_other_technique_old'].' '.$row['ae_teach_other_details'];
	}
	else
		$fields['meditation_no'] = 'Yes'; 	

	//$fields['Signature'] = ''; 
	$fields['Date'] = $row['c_start'];	
	return $fields;

}


function map_application_fields_lc($row)
{
	$fields = array();
	$temp = explode("/", $row['c_name']);
	$fields['form1[0].#subform[0].centre[0]'] = trim($temp[0]);
	if( strtolower($row['a_type']) == 'student' )
		$fields['form1[0].#subform[0].#area[0].Radio1[0]'] = "1";
	else
		$fields['form1[0].#subform[0].#area[0].Radio2[0]'] = "1";

	$fields['form1[0].#subform[0].conf[0]'] = $row['a_conf_no'];
	if ( $row['aa_acco'] )
		$fields['form1[0].#subform[0].acc[0]'] = $row['aa_section']."-".$row['aa_acco'];
	//$fields['form1[0].#subform[0].seat[0]'] = '';
	$fields['form1[0].#subform[0].cell[0]'] = $row['aa_cell'];

	$fields['form1[0].#subform[0].ctype[0]'] = $row['td_val1'];

	$temp = explode("-", $row['c_start']); $temp1 = explode("-", $row['c_end']);
	$fields['form1[0].#subform[0].cdate[0]'] = $temp[2].'/'.$temp[1].'/'.$temp[0].' To '.$temp1[2].'/'.$temp1[1].'/'.$temp1[0];
	$fields['form1[0].#subform[0].gender[0]'] = ($row['a_gender'] == 'M')?'Male':'Female';
	$fields['form1[0].#subform[0].age[0]'] = $row['Age'];
	$temp = explode("-", $row['a_dob']);	
	$fields['form1[0].#subform[0].dob[0]'] = $temp[2].'/'.$temp[1].'/'.$temp[0];

	$fields['form1[0].#subform[0].fullname[0]'] = $row['a_f_name'].' '.$row['a_m_name'].' '.$row['a_l_name'];
	$fields['form1[0].#subform[0].address[0]'] = $row['a_address'].', '.$row['City'].', '.$row['a_state'].', '.$row['a_country'].' - '.$row['a_zip'];
	$fields['form1[0].#subform[0].home[0]'] = $row['a_phone_home']; 	
	$fields['form1[0].#subform[0].office[0]'] = $row['a_phone_office'];	
	$fields['form1[0].#subform[0].mobile[0]'] = $row['a_phone_mobile']; 
	$fields['form1[0].#subform[0].email[0]'] = $row['a_email'];

	$fields['form1[0].#subform[0].education[0]'] = $row['a_education'];

	$occupation = strtolower($row['a_occupation']);
	if ( $row['a_occupation_past'] )
		$occupation = $row['a_occupation_past'];
	$fields['form1[0].#subform[0].occupation[0]'] = $occupation;
	$fields['form1[0].#subform[0].designation[0]'] = $row['a_designation'];
	$fields['form1[0].#subform[0].company[0]'] = $row['a_company'];
	$fields['form1[0].#subform[0].atapptyear[0]'] = $row['al_at_year'];
	
	$fields['form1[0].#subform[0].ename[0]'] = $row['a_emergency_name'];
	$fields['form1[0].#subform[0].erelation[0]'] = $row['a_emergency_relation'];
	$fields['form1[0].#subform[0].emobile[0]'] = $row['a_emergency_num'];
	
	if ($row['al_committed']) $fields['form1[0].#subform[0].pd1[0]'] = '1'; else $fields['form1[0].#subform[0].pd2[0]'] = '1';
	if ($row['al_exclusive_2yrs']) $fields['form1[0].#subform[0].pd3[0]'] = '1'; else $fields['form1[0].#subform[0].pd4[0]'] = '1';

	$fields['form1[0].#subform[0].dailypractice[0]'] = $row['al_daily_practice_yrs'];
	$fields['form1[0].#subform[0].practicedetails[0]'] = $row['al_daily_practice_details'];

	if ($row['al_5_precepts']) $fields['form1[0].#subform[1].pd5[0]'] = '1'; else $fields['form1[0].#subform[1].pd6[0]'] = '1';
	if ($row['al_sexual_misconduct']) $fields['form1[0].#subform[1].pd7[0]'] = '1'; else $fields['form1[0].#subform[1].pd8[0]'] = '1';
	if ($row['al_intoxicants']) $fields['form1[0].#subform[1].pd9[0]'] = '1'; else $fields['form1[0].#subform[1].pd10[0]'] = '1';
	//if ($row['al_requirements']) $fields['form1[0].#subform[0].pd11[0]'] = 'Yes'; else $fields['form1[0].#subform[0].pd12[0]'] = 'Yes';
	//$fields['form1[0].#subform[1].creqdetails[0]'] = $row['al_requirements_details'];

	$fields['form1[0].#subform[1].Table1[0].Row1[0].L1[0]'] = $row['ac_first_location_str'];
	$fields['form1[0].#subform[1].Table1[0].Row1[0].D1[0]'] = $row['ac_first_day'].'/'.$row['ac_first_month'].'/'.$row['ac_first_year'];
	$fields['form1[0].#subform[1].Table1[0].Row1[0].C1[0]'] = $row['ac_first_teacher_str'];
	$fields['form1[0].#subform[1].Table1[0].Row2[0].L2[0]'] = $row['ac_last_location_str'];
	$fields['form1[0].#subform[1].Table1[0].Row2[0].D2[0]'] = $row['ac_last_day'].'/'.$row['ac_last_month'].'/'.$row['ac_last_year'];
	$fields['form1[0].#subform[1].Table1[0].Row2[0].C2[0]'] = $row['ac_last_teacher_str'];
	$fields['form1[0].#subform[1].Table1[0].Row2[0].rec_course[0]'] = $row['al_recent_sat_type'];
	$fields['form1[0].#subform[1].Table1[0].Row3[0].L3[0]'] = $row['al_last_lc_location'];
	$fields['form1[0].#subform[1].Table1[0].Row3[0].D3[0]'] = $row['al_last_lc_date'];
	$fields['form1[0].#subform[1].Table1[0].Row3[0].C3[0]'] = $row['al_last_lc_teacher'];
	$fields['form1[0].#subform[1].Table1[0].Row3[0].rec_lc[0]'] = $row['al_last_lc_type'];

	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd1[0]'] = $row['ac_10d'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd2[0]'] = $row['ac_stp'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd3[0]'] = $row['ac_spl'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd4[0]'] = $row['ac_20d'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd5[0]'] = $row['ac_30d'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd6[0]'] = $row['ac_45d'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd7[0]'] = $row['ac_60d'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd8[0]'] = $row['ac_tsc'];
	$fields['form1[0].#subform[1].Table2[0].Row2[0].cd9[0]'] = $row['ac_service'];
	$fields['form1[0].#subform[1].sevadetails[0]'] = $row['al_served_other'];

	//form1[0].#subform[1].cd1[0] = Yes form1[0].#subform[1].cd2[0] = No Done 10d after 30d/45d ? 

	if ($row['al_relationship']) $fields['form1[0].#subform[1].rd1[0]'] = '1'; else $fields['form1[0].#subform[1].rd2[0]'] = '1';
	if ($row['al_relationship'])
	{
		if ($row['al_relationship_lifelong']) $fields['form1[0].#subform[1].rd3[0]'] = '1'; else $fields['form1[0].#subform[1].rd4[0]'] = '1';
		$fields['form1[0].#subform[1].spouse[0]'] = $row['al_spouse_name'];
		if ($row['al_relationship_lifelong']) $fields['form1[0].#subform[1].rd3[0]'] = '1'; else $fields['form1[0].#subform[1].rd4[0]'] = '1';
		if ($row['al_relation_harmonious']) $fields['form1[0].#subform[1].rd5[0]'] = '1'; else $fields['form1[0].#subform[1].rd6[0]'] = '1';
		if ($row['al_spouse_approve']) $fields['form1[0].#subform[1].rd7[0]'] = '1'; else $fields['form1[0].#subform[1].rd8[0]'] = '1';
		if ($row['al_spouse_meditator']) $fields['form1[0].#subform[1].rd9[0]'] = '1'; else $fields['form1[0].#subform[1].rd10[0]'] = '1';
		if ($row['al_spouse_other_technique']) $fields['form1[0].#subform[1].rd11[0]'] = '1'; else $fields['form1[0].#subform[1].rd12[0]'] = '1';

	}

	if ( $row['a_problem_physical'])
	{
		$fields['form1[0].#subform[1].hi3[0]'] = '1';
		$fields['form1[0].#subform[1].physicalinfo[0]'] =  $row['ae_desc_physical']; 	
	}
	else
		$fields['form1[0].#subform[1].hi4[0]'] = '1';

	if ( $row['a_problem_mental'])
	{
		$fields['form1[0].#subform[1].mentalinfo[0]'] = $row['ae_desc_mental']; 
		$fields['form1[0].#subform[1].hi1[0]'] = '1';
	}
	else
		$fields['form1[0].#subform[1].hi2[0]'] = '1';


	if ( $row['a_medication'])
	{
		$fields['form1[0].#subform[1].medinfo[0]'] = $row['ae_desc_medication']; 
		$fields['form1[0].#subform[1].hi5[0]'] = '1';
	}
	else
		$fields['form1[0].#subform[1].hi6[0]'] = '1';


	if ( $row['al_left_course'])
	{
		$fields['form1[0].#subform[4].refusedinfo[0]'] = $row['al_left_course_details']; 
		$fields['form1[0].#subform[4].ec1[0]'] = '1';
	}
	else
		$fields['form1[0].#subform[4].ec2[0]'] = '1';


	if ( $row['al_reduce_practice'])
	{
		$fields['form1[0].#subform[4].diffinfo[0]'] = $row['al_reduce_practice_details']; 
		$fields['form1[0].#subform[4].ec3[0]'] = '1';
	}
	else
		$fields['form1[0].#subform[4].ec4[0]'] = '1';


	if ( $row['al_personal_tragedy'])
	{
		$fields['form1[0].#subform[4].tragicinfo[0]'] = $row['al_personal_tragedy_details']; 
		$fields['form1[0].#subform[4].ec5[0]'] = '1';
	}
	else
		$fields['form1[0].#subform[4].ec6[0]'] = '1';


	if ( $row['al_difficulty'])
	{
		$fields['form1[0].#subform[4].partdiff[0]'] = $row['al_difficulty_details']; 
		$fields['form1[0].#subform[4].ec7[0]'] = '1';
	}
	else
		$fields['form1[0].#subform[4].ec8[0]'] = '1';


	$fields['form1[0].#subform[4].specialreq[0]'] = $row['al_special_req_details']; 

	$fields['form1[0].#subform[4].Table3[0].Row1[1].lang1[0]'] = $row['a_lang_1'];
	$fields['form1[0].#subform[4].Table3[0].Row1[1].lang1prof[0]'] = $row['a_lang_1_level'];
	$fields['form1[0].#subform[4].Table3[0].Row2[0].lang2[0]'] = $row['a_lang_2'];
	$fields['form1[0].#subform[4].Table3[0].Row2[0].lang2prof[0]'] = $row['a_lang_2_level'];
	$fields['form1[0].#subform[4].Table3[0].Row2[1].lang3[0]'] = $row['a_lang_3'];
	$fields['form1[0].#subform[4].Table3[0].Row2[1].lang3prof[0]'] = $row['a_lang_3_level'];
	$fields['form1[0].#subform[4].arrival[0]'] = $row['al_arrival'];
	$temp = explode("-", $row['c_start']); 
	$fields['form1[0].#subform[4].date[0]'] = $temp[2].'/'.$temp[1].'/'.$temp[0];
	$fields['form1[0].#subform[4].lingo[0]'] = $row['al_lang_discourse'];
	$fields['form1[0].#subform[4].otherlingo[0]'] = $row['a_langs'];
	$fields['form1[0].#subform[4].Table4[0].Row2[0].recomat[0]'] = $row['al_recommending'];
	$fields['form1[0].#subform[4].Table4[0].Row3[0].areat[0]'] = $row['al_area_at'];

	return $fields;

}


