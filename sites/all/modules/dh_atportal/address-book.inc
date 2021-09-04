<?php


function dh_atportal_address_book()
{
	$out = '<h1>AT Address Book</h1><ul><li><h2>'.l("Add Assistant Teacher", "at-portal/address-book/add").'</h2></li>'.
	'</ul>';
	return $out;
}

function dh_atportal_profile($form, &$form_state, $at_code)
{
	global $user;

	$form['a'] = array('#markup' => '<h2>Profile for '.$at_code.'</h2>');

	return $form;
}


