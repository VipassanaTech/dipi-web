<?php

/**
 * Announcements: stored in dh_announcement, listed newest first on an archive page,
 * and shown on a dashboard (centre Manage page / AT Portal) for
 * announcement_dashboard_days days after they are posted.
 * Design: docs/superpowers/specs/2026-09-24-announcements-design.md
 */

function _dh_announcement_audiences()
{
	return array('centre' => 'Centre staff', 'at' => 'Assistant Teachers');
}

function _dh_announcement_days()
{
	$days = (int) variable_get('announcement_dashboard_days', 10);
	return ($days > 0) ? $days : 10;
}

// Menu loader for %announcement_id: an existing, not-deleted announcement.
function announcement_id_load($id)
{
	if (!is_numeric($id))
		return false;
	$found = db_query("select an_id from dh_announcement where an_id = :id and an_deleted = 0", array(':id' => $id))->fetchField();
	return $found ? $id : false;
}

function _dh_announcement_css()
{
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/announcements.css');
}

// Bodies are admin-written CKEditor HTML. Strip scripts, on* handlers and style
// attributes, then balance the tags so a bad paste can't break the page around it.
function _dh_announcement_body_safe($html)
{
	return _filter_htmlcorrector(filter_xss_admin($html));
}

function _dh_announcement_box($r)
{
	return '<div class="dh-ann-item">'
		.'<div class="dh-ann-title">'.check_plain($r->an_title)
		.'<span class="dh-ann-date">'.date('j M Y', strtotime($r->an_posted)).'</span></div>'
		.'<div class="dh-ann-body">'._dh_announcement_body_safe($r->an_body).'</div>'
		.'</div>';
}

// Dashboard block: this audience's announcements from the last N days, newest first,
// plus a link to the archive (always shown, so old announcements stay findable).
function dh_announcements_block($audience, $archive_path)
{
	_dh_announcement_css();
	$since = date('Y-m-d H:i:s', REQUEST_TIME - _dh_announcement_days() * 86400);
	$rows = db_query("select an_id, an_title, an_body, an_posted from dh_announcement
		where an_audience = :a and an_deleted = 0 and an_posted >= :since
		order by an_posted desc, an_id desc", array(':a' => $audience, ':since' => $since))->fetchAll();

	// Same compact look as the old important_notice banner: one green box per
	// announcement, sized to its text, "📣 New: <body>". Title + date are in the tooltip.
	$out = '<div class="dh-ann dh-ann-dash">';
	foreach ($rows as $r)
		$out .= '<div class="dh-ann-item dh-ann-compact" title="'.check_plain($r->an_title.' ('.date('j M Y', strtotime($r->an_posted)).')').'">'
			.'<span class="dh-ann-new">&#128227; <b>'.t('New').':</b></span> '
			.'<div class="dh-ann-body">'._dh_announcement_body_safe($r->an_body).'</div>'
			.'</div>';

	$links = l(t('All announcements'), $archive_path);
	if (user_access('administer manageapp'))
		$links .= ' &middot; '.l(t('Manage'), 'announcements/manage');
	return $out.'<div class="dh-ann-links">'.$links.'</div></div>';
}

function dh_announcements_archive_centre($centre_id)
{
	$name = db_query("select c_name from dh_center where c_id = :c", array(':c' => $centre_id))->fetchField();
	return _dh_announcements_archive('centre', l(t('Back to Manage @name', array('@name' => $name)), 'centre/'.$centre_id));
}

// Main-menu entry point (no centre context, so no back link).
function dh_announcements_archive_centre_all()
{
	return _dh_announcements_archive('centre', '');
}

function dh_announcements_archive_at()
{
	return _dh_announcements_archive('at', l(t('Back to AT Portal'), 'at-portal'));
}

function _dh_announcements_archive($audience, $back_link)
{
	_dh_announcement_css();
	drupal_set_title(t('Announcements'));
	$query = db_select('dh_announcement', 'a')->extend('PagerDefault')->limit(25);
	$query->fields('a', array('an_id', 'an_title', 'an_body', 'an_posted'))
		->condition('a.an_audience', $audience)
		->condition('a.an_deleted', 0)
		->orderBy('a.an_posted', 'DESC')
		->orderBy('a.an_id', 'DESC');
	$rows = $query->execute()->fetchAll();

	// A narrow reading column, with a year heading whenever the year changes.
	$out = '<div class="dh-ann-archive-wrap">';
	if ($back_link != '')
		$out .= '<h2>'.$back_link.'</h2>';
	if (user_access('administer manageapp'))
		$out .= '<p>'.l(t('Manage announcements'), 'announcements/manage').'</p>';
	$out .= '<div class="dh-ann dh-ann-archive">';
	if (!$rows)
		$out .= '<p>'.t('No announcements yet.').'</p>';
	$year = '';
	foreach ($rows as $r)
	{
		if (substr($r->an_posted, 0, 4) != $year)
		{
			$year = substr($r->an_posted, 0, 4);
			$out .= '<h3 class="dh-ann-year">'.$year.'</h3>';
		}
		$out .= _dh_announcement_box($r);
	}
	return $out.'</div>'.theme('pager').'</div>';
}

// Admin list (site admins only).
function dh_announcements_manage()
{
	_dh_announcement_css();
	$audiences = _dh_announcement_audiences();
	$days = _dh_announcement_days();
	$filter = (isset($_GET['audience']) && isset($audiences[$_GET['audience']])) ? $_GET['audience'] : '';

	$out = '<p>'.l('+ '.t('New announcement'), 'announcements/manage/add', array('attributes' => array('class' => array('btn', 'btn-primary')))).'</p>';
	$show = array(($filter == '') ? '<b>All</b>' : l(t('All'), 'announcements/manage'));
	foreach ($audiences as $key => $label)
		$show[] = ($filter == $key) ? '<b>'.$label.'</b>' : l($label, 'announcements/manage', array('query' => array('audience' => $key)));
	$out .= '<p>'.t('Show').': '.implode(' | ', $show).'</p>';
	// Help: how long announcements stay on the dashboards, and where to change it.
	$out .= '<div class="alert alert-info dh-ann-help">'
		.t('A new announcement shows on the dashboard (the centre Manage page for centre staff, the AT Portal for Assistant Teachers) for <b>@d days</b> after it is posted. After that it is only in the archive (All announcements).',
			array('@d' => $days))
		.'<br>'.t('To change the number of days, open !link and change <b>Announcements: days to show on dashboards</b>.',
			array('!link' => l(t('Manage Applicant Settings'), 'admin/dh_manageapp')))
		.'</div>';

	$where = "a.an_deleted = 0";
	$args = array();
	if ($filter != '')
	{
		$where .= " and a.an_audience = :a";
		$args[':a'] = $filter;
	}
	$res = db_query("select a.*, u.name as updated_name from dh_announcement a
		left join users u on u.uid = a.an_updated_by
		where $where order by a.an_posted desc, a.an_id desc", $args);

	$rows = array();
	foreach ($res as $r)
	{
		$until = strtotime($r->an_posted) + $days * 86400;
		$rows[] = array(
			date('j M Y', strtotime($r->an_posted)),
			isset($audiences[$r->an_audience]) ? $audiences[$r->an_audience] : check_plain($r->an_audience),
			check_plain($r->an_title),
			($until > REQUEST_TIME) ? t('Showing until @d', array('@d' => date('j M Y', $until))) : t('Archive only'),
			$r->an_updated ? check_plain($r->updated_name).', '.date('j M Y', strtotime($r->an_updated)) : '&mdash;',
			l(t('Edit'), 'announcements/manage/'.$r->an_id.'/edit').' | '.l(t('Delete'), 'announcements/manage/'.$r->an_id.'/delete'),
		);
	}
	$header = array(t('Posted'), t('Audience'), t('Title'), t('Dashboard'), t('Last edited'), '');
	$out .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No announcements yet.')));
	return $out;
}

function dh_announcement_form($form, &$form_state, $an_id = null)
{
	$row = null;
	if ($an_id)
		$row = db_query("select * from dh_announcement where an_id = :id and an_deleted = 0", array(':id' => $an_id))->fetchObject();
	drupal_set_title($row ? t('Edit announcement') : t('New announcement'));
	drupal_add_js(libraries_get_path('ckeditor').'/ckeditor.js');

	$form['back'] = array('#markup' => '<h2>'.l(t('Back to Manage Announcements'), 'announcements/manage').'</h2>');
	$form['an_id'] = array('#type' => 'value', '#value' => $row ? $row->an_id : 0);
	$form['an_audience'] = array('#type' => 'radios', '#title' => t('Audience'), '#required' => true,
		'#options' => _dh_announcement_audiences(), '#default_value' => $row ? $row->an_audience : 'centre',
		'#description' => t('Centre staff see it on the centre Manage page; Assistant Teachers see it on the AT Portal dashboard.'));
	$form['an_title'] = array('#type' => 'textfield', '#title' => t('Title'), '#required' => true,
		'#maxlength' => 200, '#size' => 80, '#default_value' => $row ? $row->an_title : '');
	// Plain textarea; CKEditor is attached to it in the inline JS below (as in letters.inc).
	$form['an_body'] = array('#type' => 'textarea', '#title' => t('Message'), '#required' => true,
		'#rows' => 10, '#default_value' => $row ? $row->an_body : '');
	$form['submit'] = array('#type' => 'submit', '#value' => $row ? t('Update announcement') : t('Post announcement'));

	// Compact toolbar. No colour/image buttons: styles are stripped on display, and
	// announcements link to wiki pages instead of embedding images.
	drupal_add_js('
		(function ($) {
			$(document).ready(function () {
				if (window.CKEDITOR) {
					if (CKEDITOR.instances["edit-an-body"]) { CKEDITOR.instances["edit-an-body"].destroy(true); }
					CKEDITOR.replace("edit-an-body", {
						height: 220,
						resize_enabled: true,
						removePlugins: "elementspath",
						toolbar: [
							{ name: "basicstyles", items: ["Bold", "Italic", "Underline", "-", "RemoveFormat"] },
							{ name: "paragraph", items: ["BulletedList", "NumberedList"] },
							{ name: "links", items: ["Link", "Unlink"] },
							{ name: "document", items: ["Source"] }
						]
					});
				}
			});
		})(jQuery);
	', 'inline');
	return $form;
}

function dh_announcement_form_validate($form, &$form_state)
{
	$v = $form_state['values'];
	$audiences = _dh_announcement_audiences();
	if (!isset($audiences[$v['an_audience']]))
		form_set_error('an_audience', t('Choose an audience.'));
	// CKEditor submits "<p>&nbsp;</p>" for an empty editor; treat that as empty.
	$text = html_entity_decode(strip_tags($v['an_body']), ENT_QUOTES, 'UTF-8');
	if (trim(str_replace("\xc2\xa0", ' ', $text)) === '')
		form_set_error('an_body', t('Message is required.'));
}

function dh_announcement_form_submit($form, &$form_state)
{
	global $user;
	$v = $form_state['values'];
	$fields = array('an_audience' => $v['an_audience'], 'an_title' => trim($v['an_title']), 'an_body' => $v['an_body']);
	if ($v['an_id'])
	{
		// Editing never moves an_posted, so the dashboard period is not restarted.
		$fields['an_updated'] = date('Y-m-d H:i:s');
		$fields['an_updated_by'] = $user->uid;
		db_update('dh_announcement')->fields($fields)->condition('an_id', $v['an_id'])->execute();
		$posted = db_query("select an_posted from dh_announcement where an_id = :id", array(':id' => $v['an_id']))->fetchField();
	}
	else
	{
		$posted = date('Y-m-d H:i:s');
		$fields['an_posted'] = $posted;
		$fields['an_created_by'] = $user->uid;
		db_insert('dh_announcement')->fields($fields)->execute();
	}

	$where = ($v['an_audience'] == 'at') ? t('AT Portal dashboard') : t('centre Manage page');
	$until = strtotime($posted) + _dh_announcement_days() * 86400;
	if ($until > REQUEST_TIME)
		drupal_set_message(t('Announcement saved. It shows on the @where until @date, and always in the archive.',
			array('@where' => $where, '@date' => date('j M Y', $until))));
	else
		drupal_set_message(t('Announcement saved. Its dashboard period is over, so it shows only in the archive.'));
	$form_state['redirect'] = 'announcements/manage';
}

function dh_announcement_delete_form($form, &$form_state, $an_id)
{
	$title = db_query("select an_title from dh_announcement where an_id = :id", array(':id' => $an_id))->fetchField();
	$form['an_id'] = array('#type' => 'value', '#value' => $an_id);
	return confirm_form($form, t('Delete announcement "@t"?', array('@t' => $title)), 'announcements/manage',
		t('It will disappear from the dashboard and the archive.'), t('Delete'), t('Cancel'));
}

function dh_announcement_delete_form_submit($form, &$form_state)
{
	global $user;
	db_update('dh_announcement')
		->fields(array('an_deleted' => 1, 'an_updated' => date('Y-m-d H:i:s'), 'an_updated_by' => $user->uid))
		->condition('an_id', $form_state['values']['an_id'])
		->execute();
	drupal_set_message(t('Announcement deleted.'));
	$form_state['redirect'] = 'announcements/manage';
}
