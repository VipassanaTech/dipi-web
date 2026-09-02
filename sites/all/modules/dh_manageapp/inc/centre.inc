<?php

define('LEFT_TO_RIGHT', 1);
define('RIGHT_TO_LEFT', 2);

function dh_manage_centre()
{
	if ( arg(1) == '' )
		return drupal_get_form('dh_zero_select_centre');
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

	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');

	$name = db_query("select c_name from dh_center where c_id=".arg(1))->fetchField();
	drupal_set_title("Manage $name");
	$out = '<div style="color:#ff0000; font-size:26px; margin-bottom:10px;">'.variable_get('important_notice', '').'</div>';
	$modules = array(
		'Centre Settings' => 'centre/'.arg(1).'/edit',
		'Manage Courses' => 'manage-course/'.arg(1) ,
		'Manage Letters' => 'letters/'.arg(1), 
		'Search' => 'search-app/'.arg(1),
		'Manage Tags' => 'tags/'.arg(1),
		'Daily Activity' => 'daily-activity/'.arg(1),
		'AT Schedule' => 'at-schedule/'.arg(1),
		'Referral List' => 'referral/'.arg(1),
		'Center Referral List' => 'center-referral/'.arg(1),
		'SMS Report' => 'centre/'.arg(1).'/sms-report' ,
		'Course Report' => 'centre/'.arg(1).'/course-report',
    'Bulk Mail Schedule' => 'centre/'.arg(1).'/bulk-mail-schedule',
	);
	$out .= '<ul class="multi-column">';
	foreach($modules as $key => $val )
	{
		if (drupal_valid_path($val))
			$out .= '<h2><li>'.l($key, $val).'</li></h2>';
	}
	$out .= '</ul>';

	// Application tags: pinned quick-access + a searchable (select2) picker, each
	// linking to the tag's Upcoming / All / Past applications (by course start date).
	if ( user_access('use tag') )
	{
		global $user;
		drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/tags.css');
		drupal_add_css(drupal_get_path('module', 'dh_manageapp') . '/css/select2.min.css');
		drupal_add_js(drupal_get_path('module', 'dh_manageapp') . '/js/select2.min.js');

		$centre_arg = arg(1);
		$base = url('search-tag/' . $centre_arg);
		$pinned = _dh_tag_pinned($user->uid, $centre_arg);
		$all_tags = _dh_tag_center_tags($centre_arg);

		// Per-tag counts (one grouped query) split by course start date, scoped to
		// the viewer's gender access, so the size is visible before opening a view.
		$gfilter = '';
		if ( !(user_access('access male') && user_access('access female')) )
			$gfilter = " AND a.a_gender = '" . (user_access('access male') ? 'M' : 'F') . "'";
		$counts = array();
		$cres = db_query("SELECT at.at_tag AS tid, COUNT(*) AS c_all,
				SUM(c.c_start >= CURDATE()) AS c_up, SUM(c.c_start < CURDATE()) AS c_past
			FROM dh_application_tag at
			JOIN dh_applicant a ON at.at_application = a.a_id
			JOIN dh_course c ON a.a_course = c.c_id
			WHERE at.at_deleted = 0 AND a.a_center = :c $gfilter
			GROUP BY at.at_tag", array(':c' => $centre_arg));
		foreach ($cres as $cr)
			$counts[$cr->tid] = array('up' => (int) $cr->c_up, 'all' => (int) $cr->c_all, 'past' => (int) $cr->c_past);

		$period_links = function ($tid) use ($base, $counts) {
			$c = isset($counts[$tid]) ? $counts[$tid] : array('up' => 0, 'all' => 0, 'past' => 0);
			return '<a class="button tag-period" href="' . $base . '/' . $tid . '/upcoming">' . t('Upcoming') . ' (' . $c['up'] . ')</a> '
				. '<a class="button tag-period" href="' . $base . '/' . $tid . '/all">' . t('All') . ' (' . $c['all'] . ')</a> '
				. '<a class="button tag-period" href="' . $base . '/' . $tid . '/past">' . t('Past') . ' (' . $c['past'] . ')</a>';
		};

		$out .= '<div class="tag-dash-block"><h3>' . t('Tagged Applications')
			. ' <a href="' . url('tags/' . $centre_arg) . '" class="tag-manage-link" style="font-size:13px;font-weight:normal">' . t('Manage tags') . '</a></h3>';

		if ($pinned) {
			$out .= '<div class="tag-pinned">';
			foreach ($pinned as $t) {
				$out .= '<div class="tag-pinned-row">'
					. '<a href="#" class="tag-unpin" data-tid="' . $t['t_id'] . '" title="' . t('Unpin') . '">&#9733;</a> '
					. '<span class="tag-chip" style="' . _dh_tag_chip_style($t['t_color']) . '">' . check_plain($t['t_name']) . '</span> '
					. $period_links($t['t_id'])
					. '</div>';
			}
			$out .= '</div>';
		}

		$opts = '<option value="">' . t('Choose a tag') . '</option>';
		foreach ($all_tags as $t) {
			$opts .= '<option value="' . $t['t_id'] . '" data-color="' . check_plain($t['t_color']) . '">' . check_plain($t['t_name']) . '</option>';
		}
		$out .= '<div class="tag-choose">'
			. '<select id="dash-tag-select">' . $opts . '</select> '
			. '<a href="#" id="dash-tag-pin" title="' . t('Pin this tag') . '" style="display:none">&#9734;</a> &nbsp;'
			. '<a id="dash-tag-upcoming" class="button tag-period" href="#">' . t('Upcoming') . '</a> '
			. '<a id="dash-tag-all" class="button tag-period" href="#">' . t('All') . '</a> '
			. '<a id="dash-tag-past" class="button tag-period" href="#">' . t('Past') . '</a>'
			. '</div></div>';

		$app_tag_url = url('app-tag/' . $centre_arg . '/handler');
		$pinned_ids_json = json_encode(array_values(array_map(function ($t) { return (string) $t['t_id']; }, $pinned)));
		drupal_add_js('
		(function ($) {
			$(document).ready(function () {
				var base = "' . $base . '/";
				var tagUrl = "' . $app_tag_url . '";
				var pinnedIds = ' . $pinned_ids_json . ';
				var tagCounts = ' . json_encode($counts) . ';
				var $sel = $("#dash-tag-select");
				function fmtTag(opt) {
					if (!opt.id) { return opt.text; }
					var color = $(opt.element).data("color") || "#607d8b";
					return $("<span class=\"tag-opt\"><span class=\"tag-dot\" style=\"background:" + color + "\"></span></span>").append(document.createTextNode(opt.text));
				}
				if ($.fn.select2) { $sel.select2({ width: "240px", allowClear: true, placeholder: "' . t('Choose a tag') . '", templateResult: fmtTag, templateSelection: fmtTag }); }
				var up = $("#dash-tag-upcoming"), al = $("#dash-tag-all"), pa = $("#dash-tag-past"), pin = $("#dash-tag-pin");
				function setEnabled(on) { [up, al, pa].forEach(function (a) { a.css({ pointerEvents: on ? "" : "none", opacity: on ? 1 : 0.4 }); }); }
				setEnabled(false);
				$sel.on("change", function () {
					var tid = $(this).val();
					if (tid) {
						var c = tagCounts[tid] || { up: 0, all: 0, past: 0 };
						up.attr("href", base + tid + "/upcoming").text("Upcoming (" + c.up + ")");
						al.attr("href", base + tid + "/all").text("All (" + c.all + ")");
						pa.attr("href", base + tid + "/past").text("Past (" + c.past + ")");
						setEnabled(true);
						pin.show().html(pinnedIds.indexOf(String(tid)) >= 0 ? "&#9733;" : "&#9734;").data("tid", tid);
					} else {
						up.text("Upcoming"); al.text("All"); pa.text("Past");
						setEnabled(false); pin.hide();
					}
				});
				pin.on("click", function (e) {
					e.preventDefault();
					var tid = String($(this).data("tid")); if (!tid) { return; }
					var op = pinnedIds.indexOf(tid) >= 0 ? "unpin" : "pin";
					$.post(tagUrl, { op: op, tag: tid }, function (j) { if (j.status) { location.reload(); } else { alert(j.msg); } }, "json");
				});
				$(".tag-unpin").on("click", function (e) {
					e.preventDefault();
					$.post(tagUrl, { op: "unpin", tag: String($(this).data("tid")) }, function (j) { if (j.status) { location.reload(); } else { alert(j.msg); } }, "json");
				});
			});
		})(jQuery);
		', 'inline');
	}

	$with_header = false;
	$temp = drupal_get_form('dh_zero_select_course', $with_header);
	$out .= drupal_render($temp);
	$out .= dashboard( arg(1) );

	return $out;

	//return $out;
}

function dh_addedit_centre()
{
	if ( arg(1) == 'add')
		return drupal_get_form('dh_ma_centre_form');

	if (arg(2) == 'edit')
	{
		$row = db_query("select * from dh_center c left join dh_center_setting cs on c.c_id=cs.cs_center where c.c_id=".arg(1))->fetchAssoc() ;
		$out = drupal_get_form('dh_ma_centre_form', $row);
		return $out;		
	}
}


function dh_ma_centre_form($form, &$form_state, $edit_data = array())
{
	include_once(drupal_get_path('module', 'dh_manageapp').'/inc/qrcode.inc');
	drupal_add_library('system', 'ui.core');
	drupal_add_library('system', 'ui.dialog');
	drupal_add_library('system', 'ui.datepicker');
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/location-common.js');

	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
	$add = 0;
	if (count($edit_data) == 0)
		$add = 1;
	$group = 'Centre Details';
	$form['#attributes'] = array('class' => array('container-inline'));
	$identifier = '';
	$city_name = ''; $qr_code_img = '';
	if (!$add)
	{
		$form['b'] = array('#markup' => '<h2>'.l("Back to Centre Dashboard", "centre/".arg(1)).'</h2>');
		$ID_CODE =  simple_crypt(arg(1));
		$identifier = 'Identifier: '.$ID_CODE;
		$qr_code = drupal_get_path('module', 'dh_manageapp').'/cache/centre-'.arg(1).'.png';
		QRcode::png($ID_CODE, $qr_code);
		$qr_code_img = '<img src="/'.$qr_code.'" />';
		if ($edit_data['c_city'] <> '')
			$city_name = db_query("select c_name from dh_city where c_id='".$edit_data['c_city']."'")->fetchField();
	}

	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['c_name'] = array('#title' => 'Centre Name', '#type' => 'textfield', '#size' => '50', '#weight' => 1, 
		'#default_value' => $add?'':$edit_data['c_name'], '#required' => 1, '#suffix' => $identifier.$qr_code_img  );
	if(user_has_role(3)){
		$form[$group]['c_vri'] = array('#title' => 'Enable VRI Schedule', '#type' => 'radios', '#weight' => 1.1,
			'#options' => array(1 => 'Yes', 0 => 'No'), 
			'#default_value' => $add?'1':$edit_data['c_vri'], '#required' => 1,  );
    $form[$group]['c_type'] = array('#title' => 'Location-Type', '#type' => 'radios', '#weight' => 1.2,
      '#options' => array("center" => 'Center', 'noncenter' => 'NonCenter'),
      '#default_value' => $add?'center':$edit_data['c_type'], '#required' => 1,  );

    if (!$add)
      drupal_set_message("Center Subdomain: ".$edit_data['c_subdomain']);
	}
	if ($add)
	{	$form[$group]['c_subdomain'] = array('#title' => 'Dhamma.org Identifier', '#type' => 'textfield', '#size' => '20', '#weight' => 2,
			'#default_value' => $add?'':$edit_data['c_subdomain'], );
		$form[$group]['c_dana'] = array('#title' => '&nbsp;Enable Dana?', '#type' => 'checkbox',
      		'#default_value' => $add?'0':$edit_data['c_dana'],  '#weight' => 3);}
	$form[$group]['c_trust'] = array('#title' => 'Trust Name', '#type' => 'textfield', '#size' => '50', '#weight' => 4, 
		'#default_value' => $add?'':$edit_data['c_trust'], '#required' => 0, );
	$form[$group]['c_pincode'] = array('#title' => 'PinCode', '#type' => 'textfield', '#size' => '15', '#weight' => 5, 
		'#default_value' => $add?'':$edit_data['c_pincode'], '#required' => 1, );

	$form[$group]['c_country'] = array('#title' => 'Country', '#type' => 'textfield', '#size' => '40', '#weight' => 6, 
		'#default_value' => $add?'':$edit_data['c_country'], '#required' => 1,
				'#attached' => array(
      				'library' => array(
        				array('system', 'ui.autocomplete'),
      		)),

		 );
	$form[$group]['c_state'] = array('#title' => 'State', '#type' => 'textfield', '#size' => '40', '#weight' => 7, 
		'#default_value' => $add?'':$edit_data['c_state'], '#required' => 1, );
	$form[$group]['c_city'] = array('#title' => 'City', '#type' => 'textfield', '#size' => '40', '#weight' => 8, 
		'#default_value' => $add?'':$city_name, '#required' => 1, );

	$form[$group]['c_address'] = array('#title' => 'Address', '#type' => 'textarea', '#rows' => '2', '#weight' => 9, 
		'#default_value' => $add?'':$edit_data['c_address'], '#required' => 1, );

	$form[$group]['c_phone'] = array('#title' => 'Phone', '#type' => 'textfield', '#size' => '50', '#weight' => 10, 
		'#default_value' => $add?'':$edit_data['c_phone'], '#required' => 1, );
	$form[$group]['c_fax'] = array('#title' => 'Fax', '#type' => 'textfield', '#size' => '50', '#weight' => 11, 
		'#default_value' => $add?'':$edit_data['c_fax'], '#required' => 0, );

	$form[$group]['c_email'] = array('#title' => 'Email', '#type' => 'textfield', '#size' => '50', '#weight' => 12, 
		'#default_value' => $add?'':$edit_data['c_email'], '#required' => 1, );
	$form[$group]['c_url'] = array('#title' => 'Website', '#type' => 'textfield', '#size' => '50', '#weight' => 13, 
		'#default_value' => $add?'':$edit_data['c_url'], '#required' => 1, );
	
	$form[$group]['cs_email_from_name'] = array('#title' => 'Email From Name', '#type' => 'textfield', '#size' => '50', '#weight' => 14, 
		'#default_value' => $add?'':$edit_data['cs_email_from_name'], '#required' => 1, );
	$form[$group]['cs_email_reply_to'] = array('#title' => 'Email Reply to', '#type' => 'textfield', '#size' => '50', '#weight' => 15, 
		'#default_value' => $add?'':$edit_data['cs_email_reply_to'], '#required' => 1, );
	$form[$group]['c_announcement'] = array('#title' => 'Announcement', '#type' => 'textarea', '#weight' => 16, 
		'#default_value' => $add?'':$edit_data['c_announcement'], '#required' => 0, );


	$group = 'Registration Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group, '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_preconf_enable'] = array('#title' => 'Enable AutoCancellation after Preconfirmation', '#type' => 'radios', '#weight' => 1,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'0':$edit_data['cs_preconf_enable'], '#required' => 1,  );
	$form[$group]['cs_preconf_days'] = array('#title' => 'Auto-Cancel (Days after Preconfirmation)', '#type' => 'textfield', '#weight' => 2,
		'#default_value' => $add?'5':$edit_data['cs_preconf_days'], '#required' => 0, '#size' => 5 );

	$form[$group]['cs_reconf_enable'] = array('#title' => 'Enable Reconfirmation', '#type' => 'radios', '#weight' => 3,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'0':$edit_data['cs_reconf_enable'], '#required' => 1,  );
	$form[$group]['cs_reconf_days'] = array('#title' => 'Send Reconfirmation (Days before Start Date)', '#type' => 'textfield', '#weight' => 4,
		'#default_value' => $add?'0':$edit_data['cs_reconf_days'], '#required' => 0, '#size' => 5 );
	$form[$group]['cs_reconf_cancel'] = array('#title' => 'Auto-Cancel (Days after Reconfirmation)', '#type' => 'textfield', '#weight' => 5,
		'#default_value' => $add?'0':$edit_data['cs_reconf_cancel'], '#required' => 0, '#size' => 5 );

	$form[$group]['cs_expected_enable'] = array('#title' => 'Enable Reminder Mail (Change Status to "Expected")', '#type' => 'radios', '#weight' => 6,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'0':$edit_data['cs_expected_enable'], '#required' => 1,  );
	$form[$group]['cs_expected_days'] = array('#title' => 'Send Reminder (Days before Start Date)', '#type' => 'textfield', '#weight' => 7,
		'#default_value' => $add?'3':$edit_data['cs_expected_days'], '#required' => 0, '#size' => 5 );


	$form[$group]['cs_whatsapp_preconf'] = array('#title' => 'Allow WhatsApp Confirmation in case of PreConfirmation', '#type' => 'radios', '#weight' => 8,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'1':$edit_data['cs_whatsapp_preconf'], '#required' => 1,  );

	$form[$group]['cs_whatsapp_reconf'] = array('#title' => 'Allow WhatsApp Confirmation in case of ReConfirmation', '#type' => 'radios', '#weight' => 9,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'1':$edit_data['cs_whatsapp_reconf'], '#required' => 1,  );

  $form[$group]['cs_whatsapp_msg'] = array('#title' => 'Enable WhatsApp Messages on Status-Change', '#type' => 'radios',
    '#weight' => 9.1,
    '#options' => array(1 => 'Yes', 0 => 'No'),
    '#default_value' => $add?'1':$edit_data['cs_whatsapp_msg'], '#required' => 1,  );


	$form[$group]['c_lc_admin_email'] = array('#title' => 'Long course admin email', '#type' => 'textfield', '#weight' => 10,
		'#default_value' => $add?'':$edit_data['c_lc_admin_email'], '#required' => 0, '#description' => 'Multiple emails can be added comma-separated' );

	$form[$group]['cs_applicant_template'] = array('#title' => 'Applicant PDF Layout', '#type' => 'radios', '#weight' => 11,
		'#options' => array(0 => 'Standard (2 pages)', 1 => 'Large (3 pages) - bigger answer boxes'),
		'#default_value' => $add?0:$edit_data['cs_applicant_template'], '#required' => 0,
		'#description' => 'The Large layout spreads the same questions over three pages so long answers '
			. '(health details, reason for attending, family/mental state) print at a readable size instead '
			. 'of shrinking to fit. Applies to the general applicant form only - long-course, seva, teenager '
			. 'and children forms are unchanged.' );


	$group = 'Search Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_pagination'] = array('#title' => 'Show Pagination on', '#type' => 'radios', '#weight' => 1,
		'#options' => array('top' => 'Top', 'bottom' => 'Bottom', 'both' => 'Top and Bottom'), 
		'#default_value' => $add?'bottom':$edit_data['cs_pagination'], '#required' => 0,  );
	
	$group = 'Course Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_course_config'] = array('#title' => 'Course Configuration', '#type' => 'textarea', '#rows' => '10', '#weight' => 3, 
		'#default_value' => $add?"[1-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[2-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[3-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[10-Day]\nMaxApps-Male = \nMaxApps-Female = \n[10-DayExecutive]\nMaxApps-Male = \nMaxApps-Female = \n[10-DaySpecial]\nMaxApps-Male = \nMaxApps-Female = \n[Satipatthana]\nMaxApps-Male = \nMaxApps-Female = \n[20-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[30-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[45-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n[60-DayOSC]\nMaxApps-Male = \nMaxApps-Female = \n\n":$edit_data['cs_course_config'], '#required' => 1,
		'#description' => 'Keywords: MaxApps-Male, MaxApps-Female, Waitlist-Male, Waitlist-Female. ' );

	$form[$group]['cs_apps_waitlist'] = array('#title' => 'Move Applications to Waitlist if course status is WaitList?', '#weight' => 4,
		'#default_value' => $add?0:$edit_data['cs_apps_waitlist'], '#type' => 'radios', '#options' => array('1' => 'Yes', '0' => 'No') );

	$group = 'Hall Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group,  '#collapsible' => true, '#collapsed' => false);
	$form[$group]['cs_hall_combined'] = array('#title' => 'Male/Female Students in same Hall', '#type' => 'radios', '#weight' => 1,
		'#options' => array(1 => 'Yes', 0 => 'No'), 
		'#default_value' => $add?'':$edit_data['cs_hall_combined'], '#required' => 1,  );
	$form[$group]['cs_seat_naming_conv'] = array('#title' => 'Seat naming convention', '#type' => 'radios', '#weight' => 1,
		'#options' => array(
			0 => 'Numerical (running number)',
			1 => 'Alphanumeric - Columns A, B, C  /  Rows 1, 2, 3  (e.g. A1, B1 across a row)',
			2 => 'Alphanumeric - Rows A, B, C  /  Columns 1, 2, 3  (e.g. A1, A2 across a row)',
		),
		'#default_value' => $add?'':$edit_data['cs_seat_naming_conv'], '#required' => 1,  );	

	/*$seat_direction = array(LEFT_TO_RIGHT => 'Left To Right', RIGHT_TO_LEFT => 'Right to Left');
	$form[$group]['cs_seating_dir_male'] = array('#title' => 'Male Seating Direction', '#type' => 'select', '#weight' => 2,
		'#options' => $seat_direction, 
		'#default_value' => $add?'1':$edit_data['cs_seating_dir_male'], '#required' => 1,  );
	$form[$group]['cs_seating_dir_female'] = array('#title' => 'Female Seating Direction', '#type' => 'select', '#weight' => 3,
		'#options' => $seat_direction, 
		'#default_value' => $add?'1':$edit_data['cs_seating_dir_female'], '#required' => 1,  );

	$form[$group]['cs_seat_per_row_male'] = array('#title' => 'No of seats per row (Male)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 4,
		'#default_value' => $add?'1':$edit_data['cs_seat_per_row_male'], '#required' => 1,  );
	$form[$group]['cs_seat_per_row_female'] = array('#title' => 'No of seats per row (Female)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 5,
		'#default_value' => $add?'1':$edit_data['cs_seat_per_row_female'], '#required' => 1,  );

	$form[$group]['cs_chowky_per_row_male'] = array('#title' => 'No of chowky per row (Male)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 6,
		'#default_value' => $add?'1':$edit_data['cs_chowky_per_row_male'], '#required' => 1,  );
	$form[$group]['cs_chowky_per_row_female'] = array('#title' => 'No of chowky per row (Female)', '#type' => 'textfield', '#size'=> 5,  '#weight' => 7,
		'#default_value' => $add?'1':$edit_data['cs_chowky_per_row_female'], '#required' => 1,  );

	*/
	// Friendly Seating Layout editor -> assembled to the [RIGHT] (Male) / [LEFT]
	// (Female) INI in cs_seat_config by _dh_ma_seatcfg_ini() on submit. Up to 20
	// sections: Male/Female (Main Plan) - always present - plus Male/Female (Group
	// 1..9), created on demand. Each section is one .dh-seatviz editor bound (via
	// data-prefix) to its own seatcfg_* fields; seat-visual.js drives them all and
	// handles creating/deleting the group sections.
	$scfg = (!$add && !empty($edit_data['cs_seat_config'])) ? dh_parse_group_seat_config($edit_data['cs_seat_config']) : array();
	if (!is_array($scfg)) $scfg = array();
	$dir_opts = array('' => 'Default (natural side)', 'right' => 'Left to right', 'left' => 'Right to left');
	$pos_opts = array('' => 'Default (natural side)', 'left' => 'Left', 'right' => 'Right', 'back' => 'Back (row behind the seats)');
	$g = function($k, $arr) { if (is_array($arr)) foreach ($arr as $kk => $vv) if (strcasecmp($kk, $k) === 0) return $vv; return ''; };

	$form[$group]['seatlayout'] = array('#type' => 'fieldset', '#title' => 'Seating Layout', '#weight' => 8);
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/seat-visual.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/seat-visual.js');

	// Shared instructions + legend - the same for every section, so shown once here
	// (above the sections) rather than repeated inside each editor. A short lead
	// line then scannable bullets, so it does not read as one dense paragraph.
	$seat_help = '<div class="dhv-help">'
		. '<p class="dhv-help-lead">Male and Female (Main Plan) are always here. <b>Add a group plan</b> for any group that seats differently &ndash; groups without their own plan use the Main Plan. Each plan is collapsed: <b>click the green + on a heading to expand it</b> (red &minus; to collapse).</p>'
		. '<ul>'
		. '<li>Set each section with the <b>&minus;/+ steppers</b>; the diagram redraws live.</li>'
		. '<li><b>Number of columns</b> is the width of the main block &ndash; students fill it row by row, so the rows shown are just a preview.</li>'
		. '<li>The <b>front row (row 1) is at the bottom</b>, nearest the teacher &ndash; exactly as the plan prints.</li>'
		. '<li>The <b>chowky grid</b> seats anyone marked chowky (low seat) or chair; they go there instead of the main grid. It appears in the plan only when at least one person is marked.</li>'
		. '<li><b>Click any seat</b> (main or chowky) to leave it empty (a gap); click it again to restore.</li>'
		. '<li>With <b>Chowky position = Back</b>, the chowky band spans the full width and its columns follow the main grid.</li>'
		. '</ul>'
		. '<div class="dhv-legend"><span class="dhv-sw"></span> chowky block (separate, low seats) &nbsp;&middot;&nbsp; '
		. '<span class="dhv-sw dhv-se"></span> empty seat (click a seat to toggle) &nbsp;&middot;&nbsp; plain = cushion seat</div>'
		. '</div>';
	$form[$group]['seatlayout']['help'] = array('#markup' => $seat_help, '#weight' => -5);

	// Builds one section: a collapsible fieldset with the visual editor container
	// and the raw seatcfg_* fields it owns. $prefix e.g. "seatcfg_male_" (Main) or
	// "seatcfg_male_g1_" (Group 1). All start collapsed.
	$mk_section = function($title, $prefix, $nat, $c, $plan_kind, $gender, $gn, $label) use ($dir_opts, $pos_opts, $g) {
		$data = 'data-prefix="'.$prefix.'" data-nat="'.$nat.'" data-plan="'.$plan_kind.'" data-gender="'.$gender.'" data-group="'.$gn.'" data-label="'.check_plain($label).'"';
		$sec = array('#type' => 'fieldset', '#collapsible' => true, '#collapsed' => true, '#title' => $title,
			'#attributes' => array('class' => array('dh-plan-section')));
		$sec['visual'] = array('#markup' => '<div class="dh-seatviz no-print" '.$data.'></div>', '#weight' => -3);
		$sec[$prefix.'spr']       = array('#type' => 'textfield', '#title' => 'Number of columns', '#size' => 6, '#default_value' => $g('SeatsPerRow', $c));
		$sec[$prefix.'sprc']      = array('#type' => 'textfield', '#title' => 'Number of chowky columns', '#size' => 6, '#default_value' => $g('SeatsPerRowChowky', $c));
		$sec[$prefix.'dir']       = array('#type' => 'select', '#title' => 'Seat direction', '#options' => $dir_opts, '#default_value' => strtolower(trim($g('SeatDirection', $c))));
		$sec[$prefix.'pos']       = array('#type' => 'select', '#title' => 'Chowky position', '#options' => $pos_opts, '#default_value' => strtolower(trim($g('ChowkyPosition', $c))));
		$sec[$prefix.'empty']     = array('#type' => 'textfield', '#title' => 'Empty seats', '#default_value' => $g('EmptySeats', $c),
			'#description' => 'Row-col pairs (row 1 = front), e.g. 2-3, 4-1. Usually set by clicking seats above.');
		$sec[$prefix.'empty_cho'] = array('#type' => 'textfield', '#title' => 'Empty chowky seats', '#default_value' => $g('EmptyChowky', $c),
			'#description' => 'Row-col pairs, e.g. 1-2. Usually set by clicking chowky seats above.');
		return $sec;
	};

	$note_main = ' <span class="dhv-sec-note">&mdash; also used by any group without its own plan</span>';
	// Two mandatory Main Plan sections ([RIGHT] = Male, [LEFT] = Female).
	$form[$group]['seatlayout']['sec_male_main'] = $mk_section('Male (Main Plan)'.$note_main, 'seatcfg_male_', 'right',
		isset($scfg['RIGHT']['']) ? $scfg['RIGHT'][''] : array(), 'main', 'male', 0, 'Male (Main Plan)');
	$form[$group]['seatlayout']['sec_male_main']['#weight'] = -4;
	$form[$group]['seatlayout']['sec_female_main'] = $mk_section('Female (Main Plan)'.$note_main, 'seatcfg_female_', 'left',
		isset($scfg['LEFT']['']) ? $scfg['LEFT'][''] : array(), 'main', 'female', 0, 'Female (Main Plan)');
	$form[$group]['seatlayout']['sec_female_main']['#weight'] = -3;

	// Add-a-group-plan control (built by seat-visual.js from the group sections below).
	$form[$group]['seatlayout']['plan_add'] = array('#markup' => '<div id="dh-plan-add" class="no-print"></div>', '#weight' => -2);

	// 18 group sections (Male/Female x Group 1..9). All are rendered; seat-visual.js
	// hides the empty ones and lets the operator create/delete them (collapsed).
	$gw = 0;
	for ($n = 1; $n <= 9; $n++)
		foreach (array('male' => array('RIGHT', 'right'), 'female' => array('LEFT', 'left')) as $gender => $meta)
		{
			list($sec_ini, $nat) = $meta;
			$prefix = 'seatcfg_'.$gender.'_g'.$n.'_';
			$gc = isset($scfg[$sec_ini][$n]) ? $scfg[$sec_ini][$n] : array();
			$lbl = ($gender == 'male' ? 'Male' : 'Female').' (Group '.$n.')';
			$form[$group]['seatlayout']['sec_'.$gender.'_g'.$n] = $mk_section($lbl, $prefix, $nat, $gc, 'group', $gender, $n, $lbl);
			$form[$group]['seatlayout']['sec_'.$gender.'_g'.$n]['#weight'] = $gw++;
		}


	$group = 'Cell Settings';
	$form[$group] = array('#type' => 'fieldset', '#title' => $group, '#collapsible' => true, '#collapsed' => false,
		'#description' => 'Tell us which meditation cells your centre has - the technical configuration is built for you.');
	// Pre-fill the friendly fields by parsing any existing config (edit mode).
	$cini = (!$add && !empty($edit_data['cs_cells_config'])) ? parse_ini_string($edit_data['cs_cells_config'], true, INI_SCANNER_RAW) : array();
	if (!is_array($cini)) $cini = array();
	// Some centres have no meditation cells at all. When unticked, the cell
	// configuration below is hidden and its validation is skipped.
	$has_cells = $add ? 1 : (isset($edit_data['cs_has_cells']) ? $edit_data['cs_has_cells'] : 1);
	$form[$group]['cs_has_cells'] = array('#type' => 'checkbox', '#title' => 'This centre has meditation cells',
		'#default_value' => $has_cells, '#weight' => -10,
		'#description' => 'Untick if this centre has no meditation cells - the cell configuration below is then not needed.',
		// The theme floats this form-item, so the Male fieldset wraps up beside it.
		// Clear the float so the cell fieldsets sit on their own rows below.
		'#suffix' => '<div style="clear:both"></div>');
	$range_help = 'Separate cell numbers with commas, and use a hyphen for a range. Example: 1-40, 55, 60-65 (letters are fine too, e.g. A1-A20, B1-B10).';
	foreach (array('male' => 'MALE', 'female' => 'FEMALE') as $gk => $sec)
	{
		$v_cells = isset($cini[$sec]['Cells'])    ? $cini[$sec]['Cells']    : '';
		$v_res   = isset($cini[$sec]['Reserved']) ? $cini[$sec]['Reserved'] : '';
		$v_old   = isset($cini[$sec]['Old'])      ? $cini[$sec]['Old']      : '';
		$v_new   = isset($cini[$sec]['New'])      ? $cini[$sec]['New']      : '';
		$sep     = ($v_old !== '' || $v_new !== '');   // separate old/new pools were used
		$sub = 'cell_'.$gk;
		$form[$group][$sub] = array('#type' => 'fieldset', '#title' => ($gk == 'male' ? 'Male' : 'Female'),
			'#states' => array('visible' => array(':input[name="cs_has_cells"]' => array('checked' => TRUE))));
		$form[$group][$sub]['cellcfg_'.$gk.'_cells'] = array('#type' => 'textfield', '#title' => 'Which cells are available?',
			'#description' => $range_help, '#default_value' => $v_cells,
			'#states' => array('visible' => array(':input[name="cellcfg_'.$gk.'_sep"]' => array('checked' => FALSE))));
		$form[$group][$sub]['cellcfg_'.$gk.'_reserved'] = array('#type' => 'textfield', '#title' => 'Any cells to keep aside?',
			'#description' => 'Held back, never auto-assigned. '.$range_help.' Leave blank if none.', '#default_value' => $v_res);
		$form[$group][$sub]['cellcfg_'.$gk.'_sep'] = array('#type' => 'checkbox', '#title' => 'Use separate cells for old & new students', '#default_value' => $sep);
		$form[$group][$sub]['cellcfg_'.$gk.'_old'] = array('#type' => 'textfield', '#title' => 'Cells for OLD students',
			'#description' => $range_help, '#default_value' => $v_old,
			'#states' => array('visible' => array(':input[name="cellcfg_'.$gk.'_sep"]' => array('checked' => TRUE))));
		$form[$group][$sub]['cellcfg_'.$gk.'_new'] = array('#type' => 'textfield', '#title' => 'Cells for NEW students',
			'#description' => $range_help, '#default_value' => $v_new,
			'#states' => array('visible' => array(':input[name="cellcfg_'.$gk.'_sep"]' => array('checked' => TRUE))));
	}


	if ($add)
		$action = 'Add Centre';
	else
		$action = 'Update';
	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 100);


	$centre_id = arg(1);

	if ( !$add )
	{
		$js = '
			var editor;
			(function ($) {
				function load_editor()
				{
					editor = new $.fn.dataTable.Editor({ 
					    ajax: {url: "/centre/'.arg(1).'/acco-handler", type: "POST"},
						table: "#centre-acco-table",
					    fields: [
					        { name: "dh_center_setting_acco.csa_gender", label: "Gender", type: "select" },
					        { name: "dh_center_setting_acco.csa_section", label:  "Section", type: "text" }, 
					        { name: "dh_center_setting_acco.csa_room", label:  "Rooms", type: "textarea" },
					        { name: "dh_center_setting_acco.csa_deleted", type: "hidden" },

					    ]
					});


					var table = $("#centre-acco-table").DataTable( {
					    ajax: "/centre/'.arg(1).'/acco-handler",
					    dom: "Bfrtip",
						columns: [
					        { data: null, render: function(data, type, row) {
					        	//alert(row.csa_gender);
					        	if (data.dh_center_setting_acco.csa_gender == "F")
					        		return "Female";
					        	else
					        		return "Male";	
					        } },
							{ data: "dh_center_setting_acco.csa_section" },
							{ data: "dh_center_setting_acco.csa_room" },
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
				                        .val( "dh_center_setting_acco.csa_deleted", 1 );
				                }
				            }
					    ]
					});

				}



				$(document).ready(function(){
					load_editor();
					autocomplete_country("#edit-c-country");
					autocomplete_state("#edit-c-state", "#edit-c-country");
					autocomplete_city("#edit-c-city", "#edit-c-country", "#edit-c-state");
					$("#edit-c-pincode").focusout(function() {
						do_pincode( $(this).val(), "#edit-c-country", "#edit-c-state", "#edit-c-city" );
					});
				});
			})(jQuery);
		';
		drupal_add_js($js, 'inline');

		$out = '<h2>Accomodation Settings</h2>
				<table id="centre-acco-table">
					<thead>
						<tr>
							<th>Gender</th>
							<th>Section</th>
							<th>Rooms</th>
						</tr>
					</thead>

		</table>';
		$form['a'] = array('#markup' => $out, '#weight' => 1000);

	}
	else
	{
		$js = '
			(function ($) {
				$(document).ready(function(){
					autocomplete_country("#edit-c-country");
					autocomplete_state("#edit-c-state", "#edit-c-country");
					autocomplete_city("#edit-c-city", "#edit-c-country", "#edit-c-state");
					$("#edit-c-pincode").focusout(function() {
						do_pincode( $(this).val(), "#edit-c-country", "#edit-c-state", "#edit-c-city" );
					});
				});
			})(jQuery);
		';
		drupal_add_js($js, 'inline');

	}

	return $form;
}

function dh_ma_centre_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];
	$centre_id = '';
	if (arg(1) <> 'add')
		$centre_id = arg(1);

	// Normalise the "has cells" checkbox to 0/1 (an unticked checkbox is absent
	// from the POST) so it always persists. Only build the Cells Configuration
	// INI when the centre actually has cells; otherwise store an empty config.
	$form_state['input']['cs_has_cells'] = empty($form_state['input']['cs_has_cells']) ? 0 : 1;
	$form_state['input']['cs_cells_config'] = $form_state['input']['cs_has_cells']
		? _dh_ma_cellcfg_ini($form_state['input'])   // cellcfg_* fields -> cs_cells_config (loop below)
		: '';

	// Assemble the Seating Configuration INI from the friendly seatcfg_* fields.
	$form_state['input']['cs_seat_config'] = _dh_ma_seatcfg_ini($form_state['input']);

	$centre = array(); $centre_setting = array();
	foreach ($form_state['input'] as $key => $value)
	{
		if ( substr($key,0,2) == 'c_')
			$centre[$key] = $value;
		if ( substr($key,0,3) == 'cs_')
			$centre_setting[$key] = $value;
	}

	if ($centre['c_city'] <> '')
	{	
		$q = "select c_id from dh_city where c_country='".$centre['c_country']."' and c_state='".$centre['c_state']."' and c_name='".$centre['c_city']."' limit 1";
		$city_id = db_query($q)->fetchField();
		//watchdog('lala - $city_id', $q);
		if ( $city_id <> '')
			$centre['c_city'] = $city_id;
	}
	if ( $centre_id <> '')
	{
		$centre['c_updated_by'] = $user->uid;
		$centre['c_updated'] = date('Y-m-d H:i:s');
		db_update('dh_center')->fields($centre)->condition('c_id', $centre_id )->execute();
		db_update('dh_center_setting')->fields($centre_setting)->condition('cs_center', $centre_id )->execute();				
	}
	else
	{
		$centre['c_created_by'] = $user->uid;
		$centre['c_created'] = date('Y-m-d H:i:s');
		$centre['c_updated_by'] = $user->uid;
		$centre['c_updated'] = date('Y-m-d H:i:s');
		$centre_id = db_insert('dh_center')->fields($centre)->execute();
		$centre_setting['cs_center'] = $centre_id;
		db_insert('dh_center_setting')->fields($centre_setting)->execute();		
		copy_letters_to_centre($centre_id);		
	}
	drupal_goto("centre/$centre_id");
}

/* Assemble the Cells Configuration INI from the friendly per-gender fields. When
   "separate old/new" is ticked, write Old/New pools; otherwise a combined Cells
   pool. Reserved is always written. */
function _dh_ma_cellcfg_ini($input)
{
	$get = function($k) use ($input) { return isset($input[$k]) ? trim($input[$k]) : ''; };
	$out = '';
	foreach (array('male' => 'MALE', 'female' => 'FEMALE') as $gk => $sec)
	{
		$out .= "[$sec]\n";
		if (!empty($input['cellcfg_'.$gk.'_sep']))
		{
			$out .= "Old = ".$get('cellcfg_'.$gk.'_old')."\n";
			$out .= "New = ".$get('cellcfg_'.$gk.'_new')."\n";
		}
		else
			$out .= "Cells = ".$get('cellcfg_'.$gk.'_cells')."\n";
		$out .= "Reserved = ".$get('cellcfg_'.$gk.'_reserved')."\n\n";
	}
	return rtrim($out)."\n";
}

/* Assemble the Seating Configuration INI ([RIGHT] = Male, [LEFT] = Female) from
   the friendly seatcfg_* fields. A side is written only when it has a seats-per-
   row value; per-group GROUP<n>- blocks are written for any group slot with data. */
function _dh_ma_seatcfg_ini($input)
{
	$get = function($k) use ($input) { return isset($input[$k]) ? trim((string)$input[$k]) : ''; };
	$out = '';
	foreach (array('male' => 'RIGHT', 'female' => 'LEFT') as $gk => $sec)
	{
		$spr = $get('seatcfg_'.$gk.'_spr');
		if ($spr === '') continue;   // this side is not defined
		$out .= "[$sec]\n";
		$out .= "SeatsPerRow = $spr\n";
		if ($get('seatcfg_'.$gk.'_sprc') !== '') $out .= 'SeatsPerRowChowky = '.$get('seatcfg_'.$gk.'_sprc')."\n";
		if ($get('seatcfg_'.$gk.'_dir') !== '')  $out .= 'SeatDirection = '.$get('seatcfg_'.$gk.'_dir')."\n";
		if ($get('seatcfg_'.$gk.'_pos') !== '')  $out .= 'ChowkyPosition = '.$get('seatcfg_'.$gk.'_pos')."\n";
		// Empty seats (row-col pairs). Stored as-is; the plan allocator will honour
		// these in a later change - for now it is saved config only.
		if ($get('seatcfg_'.$gk.'_empty') !== '') $out .= 'EmptySeats = '.$get('seatcfg_'.$gk.'_empty')."\n";
		if ($get('seatcfg_'.$gk.'_empty_cho') !== '') $out .= 'EmptyChowky = '.$get('seatcfg_'.$gk.'_empty_cho')."\n";
		for ($n = 1; $n <= 30; $n++)   // scan every rendered per-group slot
		{
			$gspr  = $get('seatcfg_'.$gk.'_g'.$n.'_spr');
			$gsprc = $get('seatcfg_'.$gk.'_g'.$n.'_sprc');
			$gdir  = $get('seatcfg_'.$gk.'_g'.$n.'_dir');
			$gpos  = $get('seatcfg_'.$gk.'_g'.$n.'_pos');
			$gempty     = $get('seatcfg_'.$gk.'_g'.$n.'_empty');
			$gempty_cho = $get('seatcfg_'.$gk.'_g'.$n.'_empty_cho');
			if ($gspr === '' && $gsprc === '' && $gdir === '' && $gpos === '') continue;
			$p = 'GROUP'.$n.'-';
			if ($gspr  !== '') $out .= $p.'SeatsPerRow = '.$gspr."\n";
			if ($gsprc !== '') $out .= $p.'SeatsPerRowChowky = '.$gsprc."\n";
			if ($gdir  !== '') $out .= $p.'SeatDirection = '.$gdir."\n";
			if ($gpos  !== '') $out .= $p.'ChowkyPosition = '.$gpos."\n";
			if ($gempty     !== '') $out .= $p.'EmptySeats = '.$gempty."\n";
			if ($gempty_cho !== '') $out .= $p.'EmptyChowky = '.$gempty_cho."\n";
		}
		$out .= "\n";
	}
	return rtrim($out)."\n";
}

/* Validate the Seating Layout (always) and the Cell Settings (only when the
   centre has cells). */
function dh_ma_centre_form_validate($form, &$form_state)
{
	$in = $form_state['input'];

	// --- Seating Layout: need at least one side, then run the INI validator. ---
	$m_spr = trim(isset($in['seatcfg_male_spr'])   ? $in['seatcfg_male_spr']   : '');
	$f_spr = trim(isset($in['seatcfg_female_spr']) ? $in['seatcfg_female_spr'] : '');
	if ($m_spr === '' && $f_spr === '')
		form_set_error('seatcfg_male_spr', 'Seating Layout: enter the number of columns for Male (Main).');
	else
		foreach (dh_validate_seat_config(_dh_ma_seatcfg_ini($in)) as $err)
			form_set_error('seatcfg_male_spr', 'Seating Layout: '.$err);

	// --- Cell Settings: each gender needs cells defined (skipped for no-cell centres). ---
	if (empty($in['cs_has_cells']))
		return;
	foreach (array('male' => 'Male', 'female' => 'Female') as $gk => $lbl)
	{
		if (!empty($in['cellcfg_'.$gk.'_sep']))
		{
			if (trim(isset($in['cellcfg_'.$gk.'_old']) ? $in['cellcfg_'.$gk.'_old'] : '') === ''
			 && trim(isset($in['cellcfg_'.$gk.'_new']) ? $in['cellcfg_'.$gk.'_new'] : '') === '')
				form_set_error('cellcfg_'.$gk.'_old', "$lbl cell settings: enter the cells for old and/or new students.");
		}
		else if (trim(isset($in['cellcfg_'.$gk.'_cells']) ? $in['cellcfg_'.$gk.'_cells'] : '') === '')
			form_set_error('cellcfg_'.$gk.'_cells', "$lbl cell settings: enter which cells are available.");
	}
}


function dh_acco_handler()
{
	global $user;
	$centre_id = arg(1);
	include_once "sites/all/libraries/editor/php/DataTables.php";

	$editor = \DataTables\Editor::inst( $db, 'dh_center_setting_acco', 'csa_id' ); 
	 $editor->fields( \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_gender' )
	 		->options( function() { 
				$gender = array();
				$gender[] = array("value" => "M", "label" => "Male");
				$gender[] = array("value" => "F", "label" => "Female");
	 			return $gender; } )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Gender is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_section' )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Section is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_room' )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Room is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_deleted' ),	 
	    \DataTables\Editor\Field::inst( 'dh_center_setting_acco.csa_center' )
	    	->setValue( $centre_id )
	    )
    ->pkey('csa_id')
    ->on( 'preRemove', function() { return false; })
    ->where( 'csa_deleted', 0)
    ->where( 'csa_center', $centre_id)
    ->process( $_POST )
    ->json();
    exit;

}

function upcoming_courses( $centre_id )
{
	$now_minus_1month = date('Y-m-d',strtotime("-1 month"));
	$now = date('Y-m-d');
	$now_plus_1month = date('Y-m-d', strtotime("+1 month"));

	$q = "select td_key,td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' order by td_val2";
	$system_statuses = db_query($q)->fetchAllKeyed();
	unset($system_statuses['STATUS-LEFT']);
	unset($system_statuses['STATUS-CUSTOM']);
	unset($system_statuses['STATUS-ATTENDED']);

	$statuses = $system_statuses;
	//$statuses = array($system_statuses['STATUS-RECEIVED'] , $system_statuses['STATUS-CLARIFICATIONRESPONSE'], $system_statuses['STATUS-CONFIRMED'], $system_statuses['STATUS-CANCELLED']);
	$q = "select td_key from dh_type_detail where td_type='COURSE-STATUS'  order by td_val2";
	$s_res = db_query($q);
	while ($r = $s_res->fetchAssoc()) $statuses[] = $r['td_key'];

	//$where = " where a_center=$centre_id and  c.c_start >= '".$now."' and c.c_start <= '".$now_plus_1month."'";
	$c_hand = db_query("select c_id from dh_course where c_center=$centre_id  and c_start >= '$now' and c_deleted=0 order by c_start limit 4");
	$courses = array();
	while($r = $c_hand->fetchAssoc())
		$courses[$r['c_id']] = $r['c_id'];
	if (count($courses) > 0)
		$where = " where a_center=$centre_id and  c.c_id in (".implode(",", array_keys($courses) ).")";
	else
		$where = " where a_center=$centre_id and c.c_id = -1";
	$out = '<h2>Upcoming Courses</h2>';
	$out .= course_summary($centre_id, $where, $statuses);

	return $out;	
}




function received_applications( $centre_id )
{
	//$now_minus_1month = date('Y-m-d',strtotime("-1 month"));
	$now = date('Y-m-d');
	$SYS_R = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key in ('STATUS-RECEIVED', 'STATUS-CLARIFICATIONRESPONSE') ");
	$SYSTEM_RECEIVED = $SYS_R->fetchAssoc()['td_val1'];
	$SYSTEM_CLARIFICATIONRESPONSE = $SYS_R->fetchAssoc()['td_val1'];
	$out = '<div style="clear: left; display: block;"><br /><h2>Received/Response Applications</h2>';
	$q = "select c.c_id,c.c_name,a_gender,a_old, count(a_id) as 'tot' from dh_applicant a left join dh_course c on (a_course=c.c_id) where a_center=$centre_id and  c.c_start >= '".$now."' and a_status IN ('".$SYSTEM_RECEIVED."', '".$SYSTEM_CLARIFICATIONRESPONSE."') group by c.c_id,c.c_name, a_gender, a_old";
	$received = db_query($q);
	$received_courses = array();
	while( $r = $received->fetchAssoc())
	{
		$received_courses[$r['c_name'].'||'.$r['c_id']][$r['a_gender']][$r['a_old']] = $r['tot'];
	}
	$header = array('Course','New Male','Old Male', 'Total Male', '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;', 'New Female','Old Female', 'Total Female');
	$gender = array('M', 'F');
	$rows = array();
	foreach( $received_courses as $course => $gender_data )
	{
		$temp = explode("||", $course);
		$course_id = $temp[1];
		//$out .= 
		$r = array();
		$r[] = l($temp[0], "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '','g' => '')));
		foreach( $gender as $v )
		{
			$r[] = isset($gender_data[$v][0])?l($gender_data[$v][0],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '0','g' => $v))):"";
			$r[] = isset($gender_data[$v][1])?l($gender_data[$v][1],"search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '1','g' => $v))):"";
			$new = 0; $old = 0;
			if (isset($gender_data[$v][0]))
				$new = $gender_data[$v][0];
			if (isset($gender_data[$v][1]))
				$old = $gender_data[$v][1];
			if ( ($new + $old) )
				$r[] = "<b>".l($new + $old, "search-course/".$centre_id."/".$course_id, array("query" => array("s" => $SYSTEM_RECEIVED.",".$SYSTEM_CLARIFICATIONRESPONSE, 't' => '','g' => $v)))."</b>";
			else
				$r[] = "";
			$r[] = ' ';
		}
		$rows[] = $r;
	}
	$out .= theme('table', array('header' => $header, 'rows' => $rows));
	$out .= '</div>';
	return $out;
}

function dashboard( $centre_id )
{

	/*$mg = Mailgun::create('key-1dec8567ba03bd362ea0d719f554c1cb');

	$mg->messages()->send('nl.vipassana.co', [
	  'from'    => 'vinay@neuralit.com',
	  'to'      => 'vinay@vinay.in',
	  'subject' => 'My First test from MG',
	  'text'    => 'It is so simple to send a message.'
	]);
	*/
	$now_minus_1month = date('Y-m-d',strtotime("-1 month"));
	$now_plus_1month = date('Y-m-d', strtotime("+1 month"));

	$out = '';
	if (user_access('received applications block'))
		$out .= received_applications( $centre_id );

	if (user_access('upcoming courses block'))
		$out .= upcoming_courses($centre_id);
	return $out; 
}


function copy_letters_to_centre( $centre_id )
{
	global $user;
	$q = "insert into dh_letter (l_center, l_event, l_course_type, l_name, l_subject, l_body, l_sms, l_created_by, l_updated_by, l_updated) select '$centre_id', l_event, l_course_type, l_name, l_subject, l_body, l_sms, '".$user->uid."', '".$user->uid."', '".date("Y-m-d H:i:s")."' from dh_letter where l_center = 0";
	db_query($q);

	$q = "insert into dh_letter_fields (lf_center, lf_name, lf_value ) select '$centre_id', lf_name, lf_value from dh_letter_fields where lf_center = 0";
	db_query($q);


}


function cron_auto_cancel()
{
	$status_cancelled = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-CANCELLED'")->fetchField();
	$q = "select cs_center, cs_reconf_cancel, cs_preconf_days, cs_reconf_enable, cs_preconf_enable from dh_center_setting where (cs_preconf_enable=1 and cs_preconf_days > 0) or (cs_reconf_enable=1 and cs_reconf_cancel > 0)";
	$UID_SYSTEM = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();

	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$append = '';
		if ( $r['cs_reconf_enable'] && ($r['cs_reconf_cancel'] > 0) )
		{
			$send_rc_date = date('Y-m-d', strtotime("-".$r['cs_reconf_cancel']." day"));
			$append = " (a_status = 'ReConfirmation' and date_format(c_start, '%Y%m%d') > '".date("Ymd")."' and a_rc_date <= '$send_rc_date')";
		}

        $append1 = '';
        if ( $r['cs_preconf_enable'] && ($r['cs_preconf_days'] > 0) )
        {
            $send_date = date('Y-m-d', strtotime("-".$r['cs_preconf_days']." day"));
            $append1 = " (a_status='PreConfirmation' and date_format(c_start, '%Y%m%d') > '".date("Ymd")."' and a_pc_date <= '$send_date')";
        }

        $append_final = '';
        if($append && $append1)
            $append_final = " ($append or $append1)";
        elseif($append)
            $append_final = $append;
        elseif($append1)
            $append_final = $append1;

        if($append_final)
        {
            $q = "select a_id from dh_applicant left join dh_course on (a_course=c_id) where a_center=".$r['cs_center']." and $append_final";
            $res = db_query($q);
            while($row = $res->fetchAssoc())
            {
                try {
                    _update_status($row['a_id'], $status_cancelled, $UID_SYSTEM);
                    dh_send_letter('applicant', $row['a_id'], $status_cancelled );

                } catch (Exception $e) {
                    logit($r['cs_center'], 'CRON-AUTO-CANCEL', $row['a_id'], 'Problem '.$e->getMessage());
                }
            }
        }

	}
}


function cron_rc_expected($centre_id=0, $course_id=0, $expected_or_rc = 'expected')
{
	$status_expected = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-EXPECTED'")->fetchField();
	$status_reconf = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-RECONFIRMATION'")->fetchField();
	$UID_SYSTEM = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();
	$q = "select cs_center, cs_reconf_days, cs_expected_days from dh_center_setting where ";
	if ($expected_or_rc == 'expected')
		$q .= " cs_expected_enable=1 and cs_expected_days > 0 ";
	else
		$q .= " cs_reconf_enable=1 and cs_reconf_days > 0 ";

	if ($centre_id > 0 )
		$q .=  " and cs_center = '$centre_id'";
	$result = db_query($q);
	$new_status = ($expected_or_rc=='expected')?$status_expected:$status_reconf;
	while($r = $result->fetchAssoc())
	{
		$end_date = date('Y-m-d', strtotime("+".$r[ ($expected_or_rc=='expected')?'cs_expected_days':'cs_reconf_days']." day"));
		//$end_rc_date = date('Y-m-d', strtotime("+".$r['cs_reconf_days']." day"));

		$q = "select c_id from dh_course where c_center='".$r['cs_center']."' and c_deleted=0 and c_cancelled = 0";
		if ($course_id > 0 )
			$q .=  " and c_id = '$course_id'";
		else
			$q .= " and c_start='$end_date'";
		$res_c = db_query($q);
		while($r_c = $res_c->fetchAssoc())
		{
			logit($r['cs_center'], 'CRON-RC-EXPECTED', $r_c['c_id'], 'Running cron for course start date '.$end_date);
			$q = "select a_id from dh_applicant where a_course=".$r_c['c_id']." and a_status = 'Confirmed'";
			$res = db_query($q);
			while($row = $res->fetchAssoc())
			{
				try {
					_update_status($row['a_id'], $new_status, $UID_SYSTEM);
					dh_send_letter('applicant', $row['a_id'], $new_status );
					
				} catch (Throwable $e) {

					sleep(5);

					try {
						dh_send_letter('applicant', $row['a_id'], $new_status );
					} catch (Throwable $e) {
						logit($r['cs_center'], 'CRON-RC-EXPECTED', $row['a_id'], 'Problem '.$e->getMessage());	
						sleep(5);

					}
				}

			}			
		}
	}	
}



