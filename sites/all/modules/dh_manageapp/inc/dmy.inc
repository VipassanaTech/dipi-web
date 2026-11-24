<?php
/**
 * Day / Month / Year dropdowns for a date in a Drupal form: DIPI's usual date input (like the course editor's,
 * js/editor.dmy.js), which works on every phone, unlike the calendar popup. Month is a name, so day and month
 * cannot be mixed up. After validation the form value is the date as 'Y-m-d' ('' when left empty and not
 * required). Tested by tests/test-course-report-dates.php.
 *
 * $opts:
 *   title       label above the dropdowns
 *   default     'Y-m-d' or '' (nothing chosen)
 *   required    TRUE: all three must be chosen
 *   years       array(first, last); default 4 years each side of this year. A default outside it is still offered.
 *   weight, description
 */
function dh_dmy_element($opts)
{
	$o = $opts + array('title' => '', 'default' => '', 'required' => FALSE, 'years' => array(date('Y') - 4, date('Y') + 4), 'weight' => 0, 'description' => '');
	list($first, $last) = $o['years'];
	$d = $m = $y = '';
	if (preg_match('/^(\d{4})-(\d{2})-(\d{2})$/', (string) $o['default'], $p) && checkdate((int) $p[2], (int) $p[3], (int) $p[1]))
	{
		$y = (int) $p[1];
		$m = (int) $p[2];
		$d = (int) $p[3];
		$first = min($first, $y);
		$last = max($last, $y);
	}
	$months = array('' => t('Month'));
	for ($i = 1; $i <= 12; $i++)
		$months[$i] = date('M', mktime(0, 0, 0, $i, 1, 2000));
	$label = '<label>' . check_plain($o['title']) . ($o['required'] ? ' <span class="form-required" title="' . t('This field is required.') . '">*</span>' : '') . '</label>';
	return array(
		'#type' => 'container',
		'#tree' => TRUE,
		'#weight' => $o['weight'],
		'#attributes' => array('class' => array('dh-dmy')),
		'#prefix' => '<div class="form-item dh-dmy-item">' . $label,
		'#suffix' => ($o['description'] !== '' ? '<div class="description">' . check_plain($o['description']) . '</div>' : '') . '</div>',
		'#element_validate' => array('dh_dmy_element_validate'),
		// Day, month, year side by side on any page (the theme otherwise makes every select full width).
		'#attached' => array('css' => array(array('type' => 'inline', 'data' =>
			'.dh-dmy{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin-bottom:10px}'
			. '.dh-dmy .form-item{margin:0}'
			. '.dh-dmy select{width:auto!important;display:inline-block;min-height:34px;font-size:15px;padding:2px 4px}'
			. '.dh-dmy .dh-dmy-day{min-width:4.5em}.dh-dmy .dh-dmy-month{min-width:5.5em}.dh-dmy .dh-dmy-year{min-width:5.5em}'))),
		'#dh_dmy_title' => $o['title'],
		'#dh_dmy_required' => (bool) $o['required'],
		'day' => array('#type' => 'select', '#options' => array('' => t('Day')) + drupal_map_assoc(range(1, 31)), '#default_value' => $d,
			'#attributes' => array('class' => array('dh-dmy-day'), 'aria-label' => t('Day'))),
		'month' => array('#type' => 'select', '#options' => $months, '#default_value' => $m,
			'#attributes' => array('class' => array('dh-dmy-month'), 'aria-label' => t('Month'))),
		'year' => array('#type' => 'select', '#options' => array('' => t('Year')) + drupal_map_assoc(range($first, $last)), '#default_value' => $y,
			'#attributes' => array('class' => array('dh-dmy-year'), 'aria-label' => t('Year'))),
	);
}

/** Turns the three dropdowns into 'Y-m-d' ('' when empty and not required), or a form error. */
function dh_dmy_element_validate($element, &$form_state)
{
	$v = drupal_array_get_nested_value($form_state['values'], $element['#parents']);
	$part = function ($k) use ($v) { return (is_array($v) && isset($v[$k]) && is_scalar($v[$k])) ? trim((string) $v[$k]) : ''; };
	$d = $part('day');
	$m = $part('month');
	$y = $part('year');
	$title = isset($element['#dh_dmy_title']) ? $element['#dh_dmy_title'] : '';
	if ($d === '' && $m === '' && $y === '')
	{
		if (!empty($element['#dh_dmy_required']))
			form_error($element, t('@title: please choose the day, month and year.', array('@title' => $title)));
		form_set_value($element, '', $form_state);
		return;
	}
	if (!ctype_digit($d) || !ctype_digit($m) || !ctype_digit($y) || !checkdate((int) $m, (int) $d, (int) $y))
	{
		form_error($element, t('@title: please choose a real date (day, month and year).', array('@title' => $title)));
		form_set_value($element, '', $form_state);
		return;
	}
	form_set_value($element, sprintf('%04d-%02d-%02d', $y, $m, $d), $form_state);
}
