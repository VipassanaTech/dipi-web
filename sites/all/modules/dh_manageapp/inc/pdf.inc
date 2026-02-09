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


function xfdf_create($filename, $fields)
{
  // Creates an XFDF File from a 2 dimensional
  // Array Format: "array ("key1" => "content1", "key2" => "content2");
  $xfdf = "<?xml version='1.0' encoding='UTF-8'?>\n";
  $xfdf .= "<xfdf xmlns='http://ns.adobe.com/xfdf/' xml:space='preserve'>\n";
  $xfdf .= "<fields>\n";
  // Loop -> Array to XFDF Data
  foreach ($fields as $key => $val)
  {
    $val = str_replace("\r\n","\n",$val);
    $val = str_replace("\r"  ,"\n",$val);
    $xfdf .= "<field name='".htmlentities($key)."'>\n";
    $xfdf .= "<value>".htmlentities($val)."</value>\n";
    $xfdf .= "</field>\n";
  };
  // XFDF "Footer"
  $xfdf .= "</fields>";
  $xfdf .= "</xfdf>";
  file_put_contents($filename, $xfdf);
}

function json_values_create($filename, $fields)
{
  file_put_contents($filename, json_encode($fields));
}


function pdf_fill_form($fdf, $pdf, $out)
{
    global $user;
    $values_json = "";
	if (is_array($fdf))
	{
		$fdf_file = tempnam(sys_get_temp_dir(), gethostname());
		xfdf_create($fdf_file, $fdf);
        $values_json = $fdf_file."-json";
        json_values_create($values_json, $fdf);
	}
	else
		$fdf_file = $fdf;

  $tmp_out_file = tempnam(sys_get_temp_dir(), gethostname());
  //$fontfile = drupal_get_path('module', 'dh_manageapp').'/fonts/FreeMono.ttf';
  $fontfile = drupal_get_path('module', 'dh_manageapp').'/fonts/FreeMonoBold.ttf';
	$cmd = "pdftk $pdf fill_form $fdf_file output $tmp_out_file flatten replacement_font $fontfile";
	//drupal_set_message($cmd);
	if ( $user->uid == 222 )
	{
		$cmd = "cd /dhamma/scripts/dipi-automation && HOME=/dhamma/scripts/dipi-automation/cache /usr/local/bin/uv run  app.py --template ".DRUPAL_ROOT."/$pdf --values $values_json --outfile $tmp_out_file --delete-widgets ";
		watchdog("dipilala", $cmd);
	}
    exec($cmd);
  $cmd = "/usr/bin/ps2pdf $tmp_out_file $out";
  exec($cmd);

	unlink( $fdf_file );
	if ( $user->uid != 222 )
	{
    unlink($fdf_file."-json");
    unlink( $tmp_out_file );
	}
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
	$fields['cname'] = $centre_name;
	$fields['cnumber'] = 'NA';
	$fields['cdate'] = $course_date;
	$fields['ctype'] = $course_type;
	$fields['address'] = $address;
	$i = 1;
	foreach( $teacher['M'] as $t )
	{
		$fields['M'.$i] = $t['t_name'].' ('.$t['t_code'].') - '.$t['ct_type'];
		$i++;		
	}
	$i = 1;
	foreach( $teacher['F'] as $t )
	{
		$fields['F'.$i] = $t['t_name'].' ('.$t['t_code'].') - '.$t['ct_type'];
		$i++;		
	}

	$fields['IN1'] = $totals['STUDENT']['M'][0]['IN'];
	$fields['IO1'] = $totals['STUDENT']['M'][1]['IN'];
	$fields['FN1'] = $totals['STUDENT']['M'][0]['NON-IN'];
	$fields['FO1'] = $totals['STUDENT']['M'][1]['NON-IN'];
	$fields['DS1'] = $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'];
	$fields['T1'] = $totals['STUDENT']['M'][0]['IN'] + $totals['STUDENT']['M'][1]['IN'] + $totals['STUDENT']['M'][0]['NON-IN'] + $totals['STUDENT']['M'][1]['NON-IN'] + $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'];


	$fields['IN2'] = $totals['STUDENT']['F'][0]['IN'];
	$fields['IO2'] = $totals['STUDENT']['F'][1]['IN'];
	$fields['FN2'] = $totals['STUDENT']['F'][0]['NON-IN'];
	$fields['FO2'] = $totals['STUDENT']['F'][1]['NON-IN'];
	$fields['DS2'] = $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];
	$fields['T2'] = $totals['STUDENT']['F'][0]['IN'] + $totals['STUDENT']['F'][1]['IN'] + $totals['STUDENT']['F'][0]['NON-IN'] + $totals['STUDENT']['F'][1]['NON-IN'] + $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];


	$fields['IN3'] = $totals['STUDENT']['M'][0]['IN'] + $totals['STUDENT']['F'][0]['IN'];
	$fields['IO3'] = $totals['STUDENT']['M'][1]['IN'] + $totals['STUDENT']['F'][1]['IN'];
	$fields['FN3'] = $totals['STUDENT']['M'][0]['NON-IN'] + $totals['STUDENT']['F'][0]['NON-IN'];
	$fields['FO3'] = $totals['STUDENT']['M'][1]['NON-IN'] + $totals['STUDENT']['F'][1]['NON-IN'];
	$fields['DS3'] = $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'] + $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];;
	$fields['T3'] = $totals['STUDENT']['M'][0]['IN'] + $totals['STUDENT']['F'][0]['IN'] + $totals['STUDENT']['M'][1]['IN'] + $totals['STUDENT']['F'][1]['IN'] + $totals['STUDENT']['M'][0]['NON-IN'] + $totals['STUDENT']['F'][0]['NON-IN'] + $totals['STUDENT']['M'][1]['NON-IN'] + $totals['STUDENT']['F'][1]['NON-IN'] + $totals['SEVAK']['M'][1]['IN'] + $totals['SEVAK']['M'][1]['NON-IN'] + $totals['SEVAK']['F'][1]['IN'] + $totals['SEVAK']['F'][1]['NON-IN'];


	$fields['IN4'] = $left['STUDENT']['M'][0]['IN'] + $left['STUDENT']['F'][0]['IN'];
	$fields['IO4'] = $left['STUDENT']['M'][1]['IN'] + $left['STUDENT']['F'][1]['IN'];
	$fields['FN4'] = $left['STUDENT']['M'][0]['NON-IN'] + $left['STUDENT']['F'][0]['NON-IN'];
	$fields['FO4'] = $left['STUDENT']['M'][1]['NON-IN'] + $left['STUDENT']['F'][1]['NON-IN'];
	$fields['DS4'] = $left['SEVAK']['M'][1]['IN'] + $left['SEVAK']['F'][1]['IN'] + $left['SEVAK']['M'][1]['NON-IN'] + $left['SEVAK']['F'][1]['NON-IN'];
	$fields['T4'] = $left['STUDENT']['M'][0]['IN'] + $left['STUDENT']['F'][0]['IN'] + $left['STUDENT']['M'][1]['IN'] + $left['STUDENT']['F'][1]['IN'] + $left['STUDENT']['M'][0]['NON-IN'] + $left['STUDENT']['F'][0]['NON-IN'] + $left['STUDENT']['M'][1]['NON-IN'] + $left['STUDENT']['F'][1]['NON-IN'] + $left['SEVAK']['M'][1]['IN'] + $left['SEVAK']['F'][1]['IN'] + $left['SEVAK']['M'][1]['NON-IN'] + $left['SEVAK']['F'][1]['NON-IN'];


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
		$fields['Std'] = $row['ae_teen_std'];
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

		if ($row['td_val3'] == 'Teen')
		{
			$template = drupal_get_path("module", "dh_manageapp")."/templates/teenager-template.pdf";
		}
		elseif ($row['td_val3'] == 'Child')
		{
			$fields['extra'] = $row['a_extra'];
			$template = drupal_get_path("module", "dh_manageapp")."/templates/children-template.pdf";
		}
		
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
	if ($row['a_voter_id']) { $fields['national id'] = 'Yes'; $fields['Photo ID'] = $row['a_voter_id']; }
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
		$fields['physical'] = $row['ae_desc_physical']; 	
		$fields['medical_yes'] = 'Yes';	
	}
	else
		$fields['medical_no'] = 'Yes';

	if ($row['a_medication'])
	{
		$fields['medication'] = $row['ae_desc_medication']; 	
		$fields['medication_yes'] = 'Yes';
	}
	else
		$fields['medication_no'] = 'Yes';

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
	$fields['Centre Name'] = trim($temp[0]);
	if( strtolower($row['a_type']) == 'student' )
		$fields['Sit'] = 'Yes';
	else
		$fields['Serve'] = 'Yes';

	$fields['Conf No'] = $row['a_conf_no'];
	if ( $row['aa_acco'] )
		$fields['Acc No'] = $row['aa_section']."-".$row['aa_acco'];
	//$fields['form1[0].#subform[0].seat[0]'] = '';
	$fields['Cell No'] = $row['aa_cell'];

	$fields['Course Type'] = $row['td_val1'];

	$temp = explode("-", $row['c_start']); $temp1 = explode("-", $row['c_end']);
	$fields['Course Date'] = $temp[2].'/'.$temp[1].'/'.$temp[0].' To '.$temp1[2].'/'.$temp1[1].'/'.$temp1[0];
	$fields['Gender'] = ($row['a_gender'] == 'M')?'Male':'Female';
	$fields['Age'] = $row['Age'];
	$temp = explode("-", $row['a_dob']);	
	$fields['DOB'] = $temp[2].'/'.$temp[1].'/'.$temp[0];

	$fields['Full Name'] = $row['a_f_name'].' '.$row['a_m_name'].' '.$row['a_l_name'];
	$fields['Address'] = $row['a_address'].', '.$row['City'].', '.$row['a_state'].', '.$row['a_country'].' - '.$row['a_zip'];


	if ($row['a_aadhar']) { $fields['aadhar'] = 'Yes'; $fields['Photo ID'] = $row['a_aadhar']; }
	if ($row['a_passport']) { $fields['passport'] = 'Yes'; $fields['Photo ID'] = $row['a_passport']; }
	if ($row['a_voter_id']) { $fields['national id'] = 'Yes'; $fields['Photo ID'] = $row['a_voter_id']; }
	if ($row['a_pancard']) { $fields['pan card'] = 'Yes'; $fields['Photo ID'] = $row['a_pancard']; }

	$fields['Home'] = $row['a_phone_home']; 	
	$fields['Office'] = $row['a_phone_office'];	
	$fields['Mobile'] = $row['a_phone_mobile']; 
	$fields['Email'] = $row['a_email'];

	$fields['Education'] = $row['a_education'];

	$occupation = strtolower($row['a_occupation']);
	if ( $row['a_occupation_past'] )
		$occupation = $row['a_occupation_past'];
	$fields['Occupation'] = $occupation;
	$fields['Designation'] = $row['a_designation'];
	$fields['Company Name'] = $row['a_company'];
	if ($row['al_at_year']) $fields['AT Year of Appointment'] = $row['al_at_year'];
	
	$fields['Ename'] = $row['a_emergency_name'];
	$fields['Erelation'] = $row['a_emergency_relation'];
	$fields['Emobile'] = $row['a_emergency_num'];
	
	if ($row['al_committed']) $fields['Committed Y'] = 'Yes'; else $fields['Committed N'] = 'Yes';
	if ($row['al_exclusive_2yrs']) $fields['Exclusive Y'] = 'Yes'; else $fields['Exclusive N'] = 'Yes';

	$fields['Dailypractice'] = $row['al_daily_practice_yrs'];
	$fields['practicedetails'] = $row['al_daily_practice_details'];

	if ($row['al_5_precepts']) $fields['5 Precepts Y'] = 'Yes'; else $fields['5 Precepts N'] = 'Yes';
	if ($row['al_sexual_misconduct']) $fields['Sexual Misconduct Y'] = 'Yes'; else $fields['Sexual Misconduct N'] = 'Yes';
	if ($row['al_intoxicants']) $fields['Intoxicants Y'] = 'Yes'; else $fields['Intoxicants N'] = 'Yes';
	//if ($row['al_requirements']) $fields['form1[0].#subform[0].pd11[0]'] = 'Yes'; else $fields['form1[0].#subform[0].pd12[0]'] = 'Yes';
	//$fields['form1[0].#subform[1].creqdetails[0]'] = $row['al_requirements_details'];

	$fields['First Location'] = $row['ac_first_location_str'];
	$fields['First Date'] = $row['ac_first_day'].'/'.$row['ac_first_month'].'/'.$row['ac_first_year'];
	$fields['First Teacher'] = $row['ac_first_teacher_str'];
	$fields['Last Location'] = $row['ac_last_location_str'];
	$fields['Last Date'] = $row['ac_last_day'].'/'.$row['ac_last_month'].'/'.$row['ac_last_year'];
	$fields['Last Teacher'] = $row['ac_last_teacher_str'];
	$fields['Last Course Type'] = $row['al_recent_sat_type'];
	$fields['Last LC Location'] = $row['al_last_lc_location'];
	$fields['Last LC Teacher'] = $row['al_last_lc_teacher'];
	$fields['Last LC Type'] = $row['al_last_lc_type'];	

	if ($row['al_last_lc_date'])
	{
		$temp = explode('/', str_replace('-', '/', $row['al_last_lc_date']));
		$temp1='';
		foreach ($temp as $val)
	    {   
	        $temp1 = "/$val$temp1";
	    }   
	    $fields['Last LC Date'] = ltrim($temp1, '/');
	}

    $fields['7DayTeen'] = $row['ac_teen'];
	$fields['10Day'] = $row['ac_10d'];
	$fields['STP'] = $row['ac_stp'];
	$fields['10Day SPL'] = $row['ac_spl'];
	$fields['20Day'] = $row['ac_20d'];
	$fields['30Day'] = $row['ac_30d'];
	$fields['45Day'] = $row['ac_45d'];
	$fields['60Day'] = $row['ac_60d'];
	$fields['TSC'] = $row['ac_tsc'];
	$fields['Dhamma Service'] = $row['ac_service'];
	$fields['Other courses served'] = $row['ae_seva_details']." ".$row['al_served_other'];

	if (isset($row['al_10day_after_last_lc']) && !is_null($row['al_10day_after_last_lc']))
	{
		if ($row['al_10day_after_last_lc'])
			$fields['10 Day After LC Y'] = 'Yes';
		else
			$fields['10 Day After LC N'] = 'Yes';
	}
	else
	{
		if($row['td_val1'] == '30 Day Course' && ($row['ac_30d'] == 0 || $row['ac_30d'] == ''))
		{
			if ($row['al_recent_sat_type']=='10-Day')
				$fields['10 Day After LC Y'] = 'Yes';
			else
				$fields['10 Day After LC N'] = 'Yes';
		}

		if($row['td_val1'] == '45 Day Course' && ($row['ac_45d'] == 0 || $row['ac_45d'] == ''))
		{
			if ($row['al_recent_sat_type']=='10-Day')
				$fields['10 Day After LC Y'] = 'Yes';
			else
				$fields['10 Day After LC N'] = 'Yes';
		}
	}



	if($row['al_last_lc_type'] && $row['al_last_lc_date'])
	{
		$q = "select td_val2 from dh_type_detail where td_key=:td_key";
		$lc_days = db_query($q, array('td_key' => $row['al_last_lc_type']))->fetchField();
		if ($lc_days)
		{
			if (($last_lc_start_time = strtotime(str_replace('/', '-', $row['al_last_lc_date']))) !== false)
			{
				$last_lc_end_time = $last_lc_start_time + ($lc_days*24*60*60);
				$current_lc_start_time = strtotime($row['c_start']);
				if ($current_lc_start_time > $last_lc_end_time)
				{
					$diff_days = round(($current_lc_start_time - $last_lc_end_time) / (60 * 60 * 24));
					$fields['gap'] = floor($diff_days / 30)." Months, ".($diff_days % 30)." Days";
				}
			}
		}
	}
	 
	if ($row['ae_pregnant_detail'])
		$fields['Pregnant'] = $row['ae_pregnant_detail']; // Pregnancy	 


	if ($row['al_relationship']) $fields['Relationship Y'] = 'Yes'; else $fields['Relationship N'] = 'Yes';
	if ($row['al_relationship'])
	{
		if ($row['al_relationship_lifelong']) $fields['Relationship Lifelong Y'] = 'Yes'; else $fields['Relationship Lifelong N'] = 'Yes';
		$fields['Spouse'] = $row['al_spouse_name'];
		if ($row['al_relation_harmonious']) $fields['Relation Harmonious Y'] = 'Yes'; else $fields['Relation Harmonious N'] = 'Yes';
		if ($row['al_spouse_approve']) $fields['Spouse Approve Y'] = 'Yes'; else $fields['Spouse Approve N'] = 'Yes';
		if ($row['al_spouse_meditator']) $fields['Spouse Meditator Y'] = 'Yes'; else $fields['Spouse Meditator N'] = 'Yes';
		if ($row['al_spouse_other_technique']) $fields['Spouse Other Tech Y'] = 'Yes'; else $fields['Spouse Other Tech N'] = 'Yes';

	}

	if ( $row['a_problem_physical'])
	{
		$fields['Physical Y'] = 'Yes';
		$fields['Physical Detail'] =  $row['ae_desc_physical']; 	
	}
	else
		$fields['Physical N'] = 'Yes';

	if ( $row['a_problem_mental'])
	{
		$fields['Mental Detail'] = $row['ae_desc_mental']; 
		$fields['Mental Y'] = 'Yes';
	}
	else
		$fields['Mental N'] = 'Yes';


	if ( $row['a_medication'])
	{
		$fields['Medication Detail'] = $row['ae_desc_medication']; 
		$fields['Medication Y'] = 'Yes';
	}
	else
		$fields['Medication N'] = 'Yes';


	if ( $row['al_left_course'])
	{
		$fields['Left Course Detail'] = $row['al_left_course_details']; 
		$fields['Left Course Y'] = 'Yes';
	}
	else
		$fields['Left Course N'] = 'Yes';


	if ( $row['al_reduce_practice'])
	{
		$fields['Reduce Practice Detail'] = $row['al_reduce_practice_details']; 
		$fields['Reduce Practice Y'] = 'Yes';
	}
	else
		$fields['Reduce Practice N'] = 'Yes';


	if ( $row['al_personal_tragedy'])
	{
		$fields['Tragedy Detail'] = $row['al_personal_tragedy_details']; 
		$fields['Tragedy Y'] = 'Yes';
	}
	else
		$fields['Tragedy N'] = 'Yes';


	if ( $row['al_difficulty'])
	{
		$fields['Difficulty Detail'] = $row['al_difficulty_details']; 
		$fields['Difficulty Y'] = 'Yes';
	}
	else
		$fields['Difficulty N'] = 'Yes';


	$fields['Special Req'] = $row['al_special_req_details']; 

	$fields['lang1'] = $row['a_lang_1'];
	$fields['l1_level'] = $row['a_lang_1_level'];
	$fields['lang2'] = $row['a_lang_2'];
	$fields['l2_level'] = $row['a_lang_2_level'];
	$fields['lang3'] = $row['a_lang_3'];
	$fields['l3_level'] = $row['a_lang_3_level'];
	$fields['Arrival'] = $row['al_arrival'];
	$temp = explode("-", $row['c_start']); 
	$fields['Date'] = $temp[2].'/'.$temp[1].'/'.$temp[0];
	$fields['lingo'] = $row['a_lang_discourse'];
	$fields['otherlingo'] = $row['a_langs'];
	$fields['R AT'] = "{$row['al_recommending']} ({$row['al_recommending_approved']})";
	$fields['A AT'] = "{$row['al_area_at']} ({$row['al_area_at_approved']})";

	return $fields;

}


