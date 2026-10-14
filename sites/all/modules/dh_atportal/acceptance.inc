<?php
/**
 * Teacher acceptance letters.
 *
 * Every Active teacher must accept the Assistant Teacher acceptance letter (ATAL). An Active
 * teacher who is Centre Teacher of a centre (an active CT row in dh_teacher_role) must also
 * accept the Centre Teacher acceptance letter (CTAL). Until they have accepted the current
 * version of each, their AT Portal login is sent to at-portal/acceptance, one letter at a
 * time, in English or Hindi. Afterwards they can read the letters again at
 * at-portal/acceptance/letters.
 *
 * Tables (dh_manageapp deploy/2026-10-08-teacher-acceptance.sql): dh_acceptance_doc holds
 * the letter texts (loaded from outside git, this repo is public) and dh_teacher_acceptance
 * records who accepted which letter, when, and in which language.
 */

/* Letter names, per language. */
function dh_acceptance_doc_names()
{
	return array(
		'ATAL' => array('en' => 'Assistant Teacher Acceptance Letter', 'hi' => 'सहायक आचार्य स्वीकृति पत्र'),
		'CTAL' => array('en' => 'Centre Teacher Acceptance Letter', 'hi' => 'केंद्र आचार्य स्वीकृति पत्र'),
	);
}

/* Page texts, per language. */
function dh_acceptance_ui($lang)
{
	$ui = array(
		'en' => array(
			'intro' => 'Before you continue to the AT Portal, please read this letter carefully and accept it.',
			'signed' => 'You may already have signed the printed AT Acceptance Letter in the past, and the CT Acceptance Letter if you are a Centre Teacher. This is the digital version of the same letter, to be accepted here once.',
			'training' => 'As an existing AT, you have already completed the three-step AT training, so the undertaking in this letter to complete it within one year does not apply to you.',
			'step' => 'Letter @n of @t',
			'accept' => 'I Accept',
			'need_tick' => 'Please tick the box to accept the letter.',
			'done' => 'Thank you. Your acceptance has been recorded.',
			'all_done' => 'You have accepted all your letters.',
			'not_teacher' => 'These letters are for Active teachers. Your login is not linked to an Active teacher, so there is nothing to accept.',
			'logout' => 'Log out',
			'my_letters' => 'My Acceptance Letters',
			'dashboard' => 'AT Portal Dashboard',
			'letter' => 'Letter',
			'status' => 'Status',
			'read' => 'Read',
			'accepted_on' => 'Accepted on @date (@lang)',
			'not_accepted' => 'Not accepted yet',
			'accept_now' => 'Accept now',
			'you_accepted' => 'You accepted this letter on @date (@lang).',
			'print' => 'Print',
			'back' => 'Back to My Acceptance Letters',
		),
		'hi' => array(
			'intro' => 'एटी पोर्टल पर आगे बढ़ने से पहले, कृपया यह पत्र ध्यान से पढ़ें और इसे स्वीकार करें।',
			'signed' => 'हो सकता है कि आपने पहले छपे हुए सहायक आचार्य स्वीकृति पत्र पर, और यदि आप केंद्र आचार्य हैं तो केंद्र आचार्य स्वीकृति पत्र पर भी, हस्ताक्षर किए हों। यह उसी पत्र का डिजिटल रूप है, जिसे यहां एक बार स्वीकार करना है।',
			'training' => 'आप पहले से सहायक आचार्य हैं और तीन चरणों वाला सहायक आचार्य प्रशिक्षण पूरा कर चुके हैं, इसलिए इस पत्र में इसे एक वर्ष के भीतर पूरा करने का वचन आप पर लागू नहीं होता।',
			'step' => 'पत्र @n / @t',
			'accept' => 'मैं स्वीकार करता/करती हूं',
			'need_tick' => 'पत्र स्वीकार करने के लिए कृपया बॉक्स पर टिक करें।',
			'done' => 'धन्यवाद। आपकी स्वीकृति दर्ज कर ली गई है।',
			'all_done' => 'आपने अपने सभी पत्र स्वीकार कर लिए हैं।',
			'not_teacher' => 'ये पत्र सक्रिय (Active) आचार्यों के लिए हैं। आपका लॉगिन किसी सक्रिय आचार्य से जुड़ा नहीं है, इसलिए कुछ भी स्वीकार करना नहीं है।',
			'logout' => 'लॉग आउट',
			'my_letters' => 'मेरे स्वीकृति पत्र',
			'dashboard' => 'एटी पोर्टल डैशबोर्ड',
			'letter' => 'पत्र',
			'status' => 'स्थिति',
			'read' => 'पढ़ें',
			'accepted_on' => '@date को स्वीकार किया (@lang)',
			'not_accepted' => 'अभी स्वीकार नहीं किया',
			'accept_now' => 'अभी स्वीकार करें',
			'you_accepted' => 'आपने यह पत्र @date को स्वीकार किया (@lang)।',
			'print' => 'प्रिंट करें',
			'back' => 'मेरे स्वीकृति पत्र पर वापस जाएं',
		),
	);
	return $ui[$lang];
}

function dh_acceptance_lang_names()
{
	return array('en' => 'English', 'hi' => 'हिन्दी');
}

/* The Active teacher behind an AT Portal login (user name "<t_code>.<t_gender>"), or FALSE. */
function dh_acceptance_teacher($account = NULL)
{
	if (!$account)
		$account = $GLOBALS['user'];
	$cache = &drupal_static(__FUNCTION__, array());
	if (!$account->uid)
		return FALSE;
	if (!array_key_exists($account->uid, $cache))
	{
		$p = strrpos($account->name, '.');
		$cache[$account->uid] = ($p === FALSE) ? FALSE : db_query("select t_id, t_f_name, t_l_name, t_gender from dh_teacher where t_code = :c and t_gender = :g and t_status = 'Active' limit 1",
			array(':c' => substr($account->name, 0, $p), ':g' => substr($account->name, $p + 1)))->fetchObject();
	}
	return $cache[$account->uid];
}

/* Current version of each loaded letter: doc => version (the loader keeps one version active). */
function dh_acceptance_versions()
{
	$v = &drupal_static(__FUNCTION__);
	if (!isset($v))
		$v = db_query("select ad_doc, max(ad_version) from dh_acceptance_doc where ad_active = 1 group by ad_doc")->fetchAllKeyed();
	return $v;
}

/* Letters this teacher must accept, in order: the ATAL, then the CTAL for a Centre Teacher. */
function dh_acceptance_required($t_id)
{
	$docs = array('ATAL');
	if (db_query("select 1 from dh_teacher_role where tr_teacher = :t and tr_role = 'CT' and tr_deleted = 0 limit 1", array(':t' => $t_id))->fetchField())
		$docs[] = 'CTAL';
	return array_values(array_intersect($docs, array_keys(dh_acceptance_versions())));
}

/* This teacher's acceptances of the current letter versions: doc => row. */
function dh_acceptance_accepted($t_id)
{
	$out = array();
	foreach (dh_acceptance_versions() as $doc => $ver)
	{
		$r = db_query("select * from dh_teacher_acceptance where ta_teacher = :t and ta_doc = :d and ta_version = :v",
			array(':t' => $t_id, ':d' => $doc, ':v' => $ver))->fetchObject();
		if ($r)
			$out[$doc] = $r;
	}
	return $out;
}

/* Letters this teacher still has to accept, in order. */
function dh_acceptance_pending($t_id)
{
	return array_values(array_diff(dh_acceptance_required($t_id), array_keys(dh_acceptance_accepted($t_id))));
}

/* One letter in one language (current version), or FALSE. */
function _dh_acceptance_doc($doc, $lang)
{
	$v = dh_acceptance_versions();
	if (!isset($v[$doc]))
		return FALSE;
	return db_query("select * from dh_acceptance_doc where ad_doc = :d and ad_version = :v and ad_lang = :l",
		array(':d' => $doc, ':v' => $v[$doc], ':l' => $lang))->fetchObject();
}

/* Language chosen with ?lang=en|hi, remembered for the session. English by default. */
function _dh_acceptance_lang()
{
	if (isset($_GET['lang']) && in_array($_GET['lang'], array('en', 'hi')))
		$_SESSION['dh_acceptance_lang'] = $_GET['lang'];
	return isset($_SESSION['dh_acceptance_lang']) ? $_SESSION['dh_acceptance_lang'] : 'en';
}

function _dh_acceptance_lang_switch($lang, $path)
{
	$out = '<div class="dh-accept-lang">';
	foreach (dh_acceptance_lang_names() as $l => $name)
		$out .= l($name, $path, array('query' => array('lang' => $l), 'attributes' => array('class' => $l == $lang ? array('dh-on') : array(), 'lang' => $l)));
	return $out.'</div>';
}

function _dh_acceptance_letter_html($d)
{
	return '<div class="dh-letter" lang="'.$d->ad_lang.'"><h2>'.check_plain($d->ad_title).'</h2>'.filter_xss_admin($d->ad_body).'</div>';
}

/* Pages a teacher can open while a letter is pending: the letters, log out, and the first-login password step. */
function _dh_acceptance_path_open($path, $uid)
{
	return $path == 'at-portal/acceptance' || strpos($path, 'at-portal/acceptance/') === 0
		|| $path == 'user/logout' || strpos($path, 'user/reset/') === 0 || $path == "user/$uid/edit";
}

/**
 * Called from dh_atportal_init() on every page: an AT Portal login whose teacher has a letter
 * to accept is sent to the acceptance page.
 */
function dh_acceptance_gate()
{
	global $user;
	if (!$user->uid || $user->uid == 1 || drupal_is_cli() || !in_array('AT Portal', $user->roles))
		return;
	if (_dh_acceptance_path_open($_GET['q'], $user->uid))
		return;
	// Nothing was pending a moment ago: skip the checks for 10 minutes.
	if (isset($_SESSION['dh_acceptance_ok']) && $_SESSION['dh_acceptance_ok'] > REQUEST_TIME - 600)
		return;
	try
	{
		$t = dh_acceptance_teacher();
		$pending = $t ? dh_acceptance_pending($t->t_id) : array();
	}
	catch (Exception $e)
	{
		return;   // tables not set up yet: never lock teachers out
	}
	if (!$pending)
	{
		$_SESSION['dh_acceptance_ok'] = REQUEST_TIME;
		return;
	}
	unset($_GET['destination']);
	drupal_goto('at-portal/acceptance');
}

/* at-portal/acceptance: the next letter this teacher has to accept. */
function dh_acceptance_page()
{
	drupal_add_css(drupal_get_path('module', 'dh_atportal').'/css/acceptance.css');
	$lang = _dh_acceptance_lang();
	$ui = dh_acceptance_ui($lang);
	$names = dh_acceptance_doc_names();
	$t = dh_acceptance_teacher();
	if (!$t)
		return '<p>'.$ui['not_teacher'].'</p><p>'.l($ui['my_letters'], 'at-portal/acceptance/letters').'</p>';
	$required = dh_acceptance_required($t->t_id);
	$pending = dh_acceptance_pending($t->t_id);
	if (!$pending)
		return _dh_acceptance_lang_switch($lang, 'at-portal/acceptance').'<p>'.$ui['all_done'].'</p><p>'.l($ui['my_letters'], 'at-portal/acceptance/letters').' | '.l($ui['dashboard'], 'at-portal').'</p>';

	$doc = $pending[0];
	$d = _dh_acceptance_doc($doc, $lang);
	if (!$d)
		$d = _dh_acceptance_doc($doc, 'en');
	drupal_set_title($names[$doc][$lang]);
	$out = _dh_acceptance_lang_switch($lang, 'at-portal/acceptance');
	$out .= '<div class="dh-accept-intro"><p>'.$ui['intro'];
	if (count($required) > 1)
		$out .= ' <b>'.t($ui['step'], array('@n' => array_search($doc, $required) + 1, '@t' => count($required))).'</b>';
	$out .= '</p><p>'.$ui['signed'].'</p>';
	// The training undertaking is in the ATAL only (senior teacher's note, 2026-10-14).
	if ($doc == 'ATAL')
		$out .= '<p>'.$ui['training'].'</p>';
	$out .= '</div>';
	$out .= _dh_acceptance_letter_html($d);
	$form = drupal_get_form('dh_acceptance_form', $doc, $d->ad_version, $d->ad_lang, $d->ad_ack);
	$out .= drupal_render($form);
	$out .= '<p class="dh-accept-logout">'.l($ui['logout'], 'user/logout').'</p>';
	return $out;
}

function dh_acceptance_form($form, &$form_state, $doc, $version, $lang, $ack)
{
	$ui = dh_acceptance_ui($lang);
	$form['#attributes']['class'][] = 'dh-accept-form';
	$form['#dh_acceptance'] = array('doc' => $doc, 'version' => $version, 'lang' => $lang);
	$form['agree'] = array('#type' => 'checkbox', '#title' => check_plain($ack));
	$form['submit'] = array('#type' => 'submit', '#value' => $ui['accept']);
	return $form;
}

function dh_acceptance_form_validate($form, &$form_state)
{
	if (empty($form_state['values']['agree']))
		form_set_error('agree', dh_acceptance_ui($form['#dh_acceptance']['lang'])['need_tick']);
}

function dh_acceptance_form_submit($form, &$form_state)
{
	global $user;
	$a = $form['#dh_acceptance'];
	$ui = dh_acceptance_ui($a['lang']);
	$t = dh_acceptance_teacher();
	$v = dh_acceptance_versions();
	// Record it only if this letter (this version) is still pending for this teacher.
	if ($t && isset($v[$a['doc']]) && $v[$a['doc']] === $a['version'] && in_array($a['doc'], dh_acceptance_pending($t->t_id)))
	{
		try
		{
			db_insert('dh_teacher_acceptance')->fields(array(
				'ta_teacher' => $t->t_id, 'ta_doc' => $a['doc'], 'ta_version' => $a['version'], 'ta_lang' => $a['lang'],
				'ta_accepted' => date('Y-m-d H:i:s'), 'ta_uid' => $user->uid, 'ta_ip' => substr(ip_address(), 0, 45),
			))->execute();
		}
		catch (PDOException $e)
		{
			// Already recorded (double submit): nothing to do.
		}
	}
	unset($_SESSION['dh_acceptance_ok']);
	if ($t && dh_acceptance_pending($t->t_id))
		$form_state['redirect'] = 'at-portal/acceptance';
	else
	{
		drupal_set_message($ui['done']);
		$form_state['redirect'] = 'at-portal';
	}
}

/* at-portal/acceptance/letters: the letters this teacher has to accept, with when they did. */
function dh_acceptance_letters_page()
{
	drupal_add_css(drupal_get_path('module', 'dh_atportal').'/css/acceptance.css');
	$lang = _dh_acceptance_lang();
	$ui = dh_acceptance_ui($lang);
	$names = dh_acceptance_doc_names();
	$langs = dh_acceptance_lang_names();
	drupal_set_title($ui['my_letters']);
	$t = dh_acceptance_teacher();
	$docs = $t ? dh_acceptance_required($t->t_id) : array_keys(dh_acceptance_versions());
	$accepted = $t ? dh_acceptance_accepted($t->t_id) : array();

	$out = _dh_acceptance_lang_switch($lang, 'at-portal/acceptance/letters');
	if (!$t)
		$out .= '<p>'.$ui['not_teacher'].'</p>';
	$rows = array();
	foreach ($docs as $doc)
	{
		$read = array();
		foreach ($langs as $l => $name)
			$read[] = l($name, "at-portal/acceptance/letter/$doc/$l", array('attributes' => array('lang' => $l)));
		if (isset($accepted[$doc]))
			$status = '<span class="dh-accept-ok">'.t($ui['accepted_on'], array('@date' => date('d M Y', strtotime($accepted[$doc]->ta_accepted)), '@lang' => $langs[$accepted[$doc]->ta_lang])).'</span>';
		elseif ($t)
			$status = $ui['not_accepted'].' &middot; '.l($ui['accept_now'], 'at-portal/acceptance');
		else
			$status = '';
		$rows[] = array(check_plain($names[$doc][$lang]).' ('.$doc.')', $status, implode(' &middot; ', $read));
	}
	$out .= theme('table', array('header' => array($ui['letter'], $ui['status'], $ui['read']), 'rows' => $rows, 'attributes' => array('class' => array('dh-accept-list'))));
	$out .= '<p>'.l($ui['dashboard'], 'at-portal').'</p>';
	return $out;
}

/* at-portal/acceptance/letter/{ATAL|CTAL}/{en|hi}: read a letter. */
function dh_acceptance_letter_page($doc, $lang)
{
	if (!in_array($lang, array('en', 'hi')) || !($d = _dh_acceptance_doc($doc, $lang)))
		return MENU_NOT_FOUND;
	drupal_add_css(drupal_get_path('module', 'dh_atportal').'/css/acceptance.css');
	$_SESSION['dh_acceptance_lang'] = $lang;
	$ui = dh_acceptance_ui($lang);
	$names = dh_acceptance_doc_names();
	$langs = dh_acceptance_lang_names();
	drupal_set_title($names[$doc][$lang]);

	$out = '<div class="dh-accept-lang">';
	foreach ($langs as $l => $name)
		$out .= l($name, "at-portal/acceptance/letter/$doc/$l", array('attributes' => array('class' => $l == $lang ? array('dh-on') : array(), 'lang' => $l)));
	$out .= '<a href="#" class="dh-accept-print" onclick="window.print(); return false;">'.$ui['print'].'</a></div>';
	$t = dh_acceptance_teacher();
	$accepted = $t ? dh_acceptance_accepted($t->t_id) : array();
	if (isset($accepted[$doc]))
		$out .= '<div class="dh-accept-done">'.t($ui['you_accepted'], array('@date' => date('d M Y', strtotime($accepted[$doc]->ta_accepted)), '@lang' => $langs[$accepted[$doc]->ta_lang])).'</div>';
	$out .= _dh_acceptance_letter_html($d);
	$out .= '<p class="dh-accept-back">'.l($ui['back'], 'at-portal/acceptance/letters').'</p>';
	return $out;
}
