<?php
/**
 * Merge-field checker for letters.
 *
 * DIPI fills a merge field only when it matches exactly: [FirstName] works, but
 * [firstname], [First Name] and [First<b>Name</b>] do not, and anything it does not
 * know goes to the student as written. These rules are used by:
 *   - the letter editor (clear mistakes are refused on save; hidden formatting is fixed),
 *   - the letters list (a problem badge per letter) and the letter page (the list),
 *   - the letter preview and the send step (fields still unfilled after merging),
 *   - the Manage Fields editor (field names and values).
 *
 * A "combined" field puts [CourseType] or [ApplicantType] inside another field, for
 * example [Time-[CourseType]]: DIPI first fills the inner field ("10 Day Course") and
 * then uses the centre's own field [Time-10 Day Course]. If the centre has no such
 * field for a course type it runs, those students get "[Time-Teenager Course]".
 */

/** Square-bracket token parts: any text or tag without brackets. */
define('DH_LT_PART', '(?:[^\[\]<]|<[^>\[\]]*>)');

/** Course fields that can be used inside another field, e.g. [Time-[CourseType]], [Reg-bus-[StartDateNum]]. */
function dh_letter_course_fields()
{
	return array('[CourseType]', '[CourseName]', '[CourseDescription]', '[StartDate]', '[EndDate]', '[StartDateNum]', '[EndDateNum]');
}

/**
 * The values a combined field's inner fields can take, to check that the centre has a
 * letter field for each. Only [CourseType] (and [ApplicantType]): every course type the
 * letter is used for. Any other course field ([StartDateNum], [CourseName], ...): the
 * centre's courses from two weeks ago to a year ahead.
 * @return array label => array(inner field => value), or NULL when the inner fields are
 *   not course fields / [ApplicantType] (then there is nothing to check).
 */
function dh_letter_combined_values($centre, $event, $ctype, $inners)
{
	$course = array_values(array_intersect($inners, dh_letter_course_fields()));
	$apptype = in_array('[ApplicantType]', $inners);
	if (count($course) + ($apptype ? 1 : 0) < count(array_unique($inners)))
		return NULL;
	$rows = array('' => array());
	if ($course == array('[CourseType]') || array_unique($course) == array('[CourseType]'))
	{
		$rows = array();
		foreach (dh_letter_course_types_to_cover($centre, $event, $ctype) as $name)
			$rows[$name] = array('[CourseType]' => $name);
	}
	elseif ($course)
	{
		$rows = array();
		foreach (dh_letter_courses_to_cover($centre, $event, $ctype) as $c)
			$rows[date('j M Y', strtotime($c['c_start'])).' '.$c['type']] = array(
				'[CourseType]' => $c['type'], '[CourseName]' => $c['c_name'], '[CourseDescription]' => trim($c['c_description']),
				'[StartDate]' => date("jS F Y", strtotime($c['c_start'])), '[EndDate]' => date("jS F Y", strtotime($c['c_end'])),
				'[StartDateNum]' => date("d-m-Y", strtotime($c['c_start'])), '[EndDateNum]' => date("d-m-Y", strtotime($c['c_end'])));
	}
	if (!$apptype)
		return $rows;
	$out = array();
	foreach ($rows as $label => $vals)
		foreach (dh_letter_applicant_types() as $t)
			$out[trim($label.' '.$t)] = $vals + array('[ApplicantType]' => $t);
	return $out;
}

/** Fields that only AT review letters fill (dh_get_letter sets them for AT letters only). */
function dh_letter_at_only_fields()
{
	return array('[RAuth]' => 1, '[AAuth]' => 1);
}

/** Letter statuses whose letters go to an AT, so [RAuth]/[AAuth] are filled. */
function dh_letter_is_at_event($event)
{
	return in_array($event, array('R-ATReview', 'A-ATReview'));
}

/**
 * Known fields for a centre: DIPI's own fields plus the centre's letter fields.
 * @return array token => 'dipi' | 'centre'
 */
function dh_letter_known_fields($centre)
{
	$cache = &drupal_static(__FUNCTION__, array());
	if (!isset($cache[$centre]))
	{
		$known = array_fill_keys(_dh_letter_builtin_fields(), 'dipi');
		$names = db_query("select lf_name from dh_letter_fields where lf_center = :c and lf_deleted = 0", array(':c' => $centre))->fetchCol();
		foreach ($names as $n)
			if (!isset($known['['.$n.']']))
				$known['['.$n.']'] = 'centre';
		$cache[$centre] = $known;
	}
	return $cache[$centre];
}

/**
 * Course types a combined [..[CourseType]..] field must cover for this letter: the
 * letter's own type, or (for an "All" letter) every type the centre ran in the last year
 * or has coming up. That includes types with their own letter for the same status: the
 * All letter can still be picked for them by hand (prod 2026-10-09: ~200 such emails in
 * six months). The template centre 0, and a centre with no courses yet, use every type
 * run anywhere in the last year.
 * @return array type id => type name
 */
function dh_letter_course_types_to_cover($centre, $event, $course_type)
{
	$names = &drupal_static(__FUNCTION__.'_names');
	if ($names === NULL)
		$names = db_query("select td_id, td_val1 from dh_type_detail where td_type = 'COURSE-TYPE'")->fetchAllKeyed();
	if ($course_type)
		return isset($names[$course_type]) ? array($course_type => $names[$course_type]) : array();

	$runs = &drupal_static(__FUNCTION__.'_runs', array());
	if (!isset($runs[$centre]))
	{
		$q = "select distinct c_course_type from dh_course where c_deleted = 0 and c_start >= curdate() - interval 365 day";
		$ids = $centre ? db_query($q." and c_center = :c", array(':c' => $centre))->fetchCol() : array();
		if (!$ids)
		{
			$all = &drupal_static(__FUNCTION__.'_all');
			if ($all === NULL)
				$all = db_query($q)->fetchCol();
			$ids = $all;
		}
		$runs[$centre] = $ids;
	}
	$out = array();
	foreach ($runs[$centre] as $id)
		if (isset($names[$id]))
			$out[$id] = $names[$id];
	asort($out);
	return $out;
}

/**
 * The centre's courses from two weeks ago to a year ahead that a letter is used for (its
 * course type, or for an "All" letter every type).
 */
function dh_letter_courses_to_cover($centre, $event, $course_type)
{
	$cache = &drupal_static(__FUNCTION__, array());
	$key = "$centre|$event|$course_type";
	if (isset($cache[$key]))
		return $cache[$key];
	$types = dh_letter_course_types_to_cover($centre, $event, $course_type);
	$out = array();
	if ($centre && $types)
		foreach (db_query("select c.c_name, c.c_description, c.c_start, c.c_end, c.c_course_type from dh_course c
			where c.c_center = :c and c.c_deleted = 0 and c.c_start between curdate() - interval 15 day and curdate() + interval 365 day
			order by c.c_start", array(':c' => $centre)) as $r)
			if (isset($types[$r->c_course_type]))
				$out[] = array('c_name' => $r->c_name, 'c_description' => $r->c_description, 'c_start' => $r->c_start,
					'c_end' => $r->c_end, 'type' => $types[$r->c_course_type]);
	return $cache[$key] = $out;
}

/** Applicant types a combined [..[ApplicantType]..] field must cover. */
function dh_letter_applicant_types()
{
	return array('Student', 'Sevak');
}

/** The text a reader sees for a piece of letter HTML (tags removed, entities decoded). */
function _dh_letter_visible($s, $is_html)
{
	if (!$is_html)
		return $s;
	return str_replace("\xC2\xA0", ' ', html_entity_decode(strip_tags($s), ENT_QUOTES, 'UTF-8'));
}

/** Compare key for near-miss matching: letters and digits only, lower case. */
function _dh_letter_norm($s)
{
	return strtolower(preg_replace('/[^A-Za-z0-9]/', '', $s));
}

/**
 * Does the text inside [ ] look like a field name? One word ([Footer], [CovidAll_123]) or a
 * prefix field like the centres' own ([Detail-1 Day Course]). Other text in brackets, such
 * as "[ ]", "[1]", "[A]", "[ click ]", "[Same letter in Marathi]" or a place name, is ordinary
 * text and is left alone (unless it is close to a real field, see _dh_letter_near_field).
 */
function _dh_letter_field_like($inner)
{
	if ($inner !== trim($inner))
		return FALSE;
	if (preg_match('#^[A-Za-z][A-Za-z0-9_./()\-]{1,59}$#', $inner))
		return TRUE;
	return preg_match('#^[A-Za-z][A-Za-z0-9]*-[A-Za-z0-9(][A-Za-z0-9 _./()\-]{0,58}$#', $inner) && substr_count($inner, ' ') <= 4;
}

/** Could the text inside [ ] be a mistyped field? Short, made of letters, digits, spaces and marks. */
function _dh_letter_near_field($inner)
{
	return (bool) preg_match('#^\s*[A-Za-z][A-Za-z0-9 _./()\-]{0,59}\s*$#', $inner) && substr_count(trim($inner), ' ') <= 3;
}

/**
 * The closest known field to a wrong token, or NULL.
 * @return array(field, reason, sure) reason: 'case' | 'space' | 'spelling'; sure: safe to fix in one click
 */
function _dh_letter_suggest($token, $known)
{
	foreach ($known as $k => $_)
		if (strcasecmp($k, $token) === 0)
			return array($k, 'case', TRUE);
	$n = _dh_letter_norm($token);
	if ($n === '')
		return NULL;
	foreach ($known as $k => $_)
		if (_dh_letter_norm($k) === $n)
			return array($k, 'space', TRUE);
	if (strlen($n) < 5)
		return NULL;
	$best = NULL; $bd = 3;
	foreach ($known as $k => $_)
	{
		$d = levenshtein($n, _dh_letter_norm($k));
		if ($d < $bd) { $bd = $d; $best = $k; }
	}
	if ($best === NULL)
		return NULL;
	// sure enough for a one-click fix: one letter off, or two letters swapped ([Satrt Date]);
	// not e.g. [CourseDate] -> [CourseName], which only looks close
	$a = str_split($n); $b = str_split(_dh_letter_norm($best)); sort($a); sort($b);
	return array($best, 'spelling', $bd == 1 || $a == $b);
}

/** Tags found inside a token, kept after the fixed token so the HTML stays balanced. */
function _dh_letter_tags($raw)
{
	preg_match_all('/<[^>]*>/', $raw, $m);
	return implode('', $m[0]);
}

/**
 * Check one letter.
 *
 * @param int   $centre  The letter's centre (0 = the new-centre template).
 * @param array $letter  l_event, l_course_type, l_subject, l_body (HTML), l_sms.
 * @return array of problems, each:
 *   level   'error' (refused on save) | 'fixed' (hidden formatting; fixed on save) | 'warning'
 *           | 'field' (a mistake inside a letter field the letter uses; fixed in Manage Fields)
 *   part    'subject' | 'body' | 'sms' | 'value' | 'field' (then 'field' => the letter field)
 *   raw     the exact text in the letter
 *   msg     what is wrong, in plain words (HTML-safe)
 *   fix     replacement for raw, or NULL when only the centre can fix it
 *   missing (warnings only) the letter-field names the centre should add
 */
function dh_letter_check($centre, $letter)
{
	$known = dh_letter_known_fields($centre);
	$event = isset($letter['l_event']) ? $letter['l_event'] : '';
	$ctype = isset($letter['l_course_type']) ? (int) $letter['l_course_type'] : 0;
	if (dh_letter_is_at_event($event))
		$known += dh_letter_at_only_fields();
	$problems = array();
	$used = array(); // the centre's letter fields this letter uses (they are filled in too)
	foreach (array('subject' => 'l_subject', 'body' => 'l_body', 'sms' => 'l_sms', 'value' => 'lf_value') as $part => $key)
		if (isset($letter[$key]) && $letter[$key] !== '')
			_dh_letter_check_text($centre, $event, $ctype, $known, (string) $letter[$key], ($part == 'body' || $part == 'value'), $part, $problems, $used);

	// Merge fields work recursively: a letter field's value can use other fields (also
	// combined ones), and DIPI fills those too. Check every letter field the letter uses,
	// and the ones those use, for this letter's courses. Problems found there are fixed in
	// Manage Fields, not in the letter.
	$values = dh_letter_field_values($centre);
	$done = array();
	$queue = array_keys($used);
	while ($queue && count($done) < 200)
	{
		$f = array_shift($queue);
		if (isset($done[$f]) || !isset($values[$f]))
			continue;
		$done[$f] = 1;
		$sub = array(); $more = array();
		_dh_letter_check_text($centre, $event, $ctype, $known, $values[$f], TRUE, 'field', $sub, $more);
		foreach ($sub as $p)
		{
			$p['field'] = $f;
			$p['level'] = $p['level'] == 'warning' ? 'warning' : 'field';
			$problems[] = $p;
		}
		foreach ($more as $g => $_)
			if (!isset($done[$g]))
				$queue[] = $g;
	}
	return $problems;
}

/** The centre's letter fields: [Name] => value (the last one wins, as when sending). */
function dh_letter_field_values($centre)
{
	$cache = &drupal_static(__FUNCTION__, array());
	if (!isset($cache[$centre]))
	{
		$cache[$centre] = array();
		foreach (db_query("select lf_name, lf_value from dh_letter_fields where lf_center = :c and lf_deleted = 0", array(':c' => $centre))->fetchAllKeyed() as $n => $v)
			$cache[$centre]['['.$n.']'] = (string) $v;
	}
	return $cache[$centre];
}

/**
 * Check one text (a letter part or a letter field's value). Adds problems to $problems
 * (with 'part' => $part) and the centre's letter fields it uses to $used.
 */
function _dh_letter_check_text($centre, $event, $ctype, $known, $text, $is_html, $part, &$problems, &$used)
{
	$seen = array();
	$add = function ($p) use (&$problems, &$seen, $part) {
		$p['part'] = $part;
		$key = $p['level'].'|'.$p['raw'];
		if (isset($seen[$key]))
			return;
		$seen[$key] = 1;
		$problems[] = $p;
	};

	// 1. Combined fields: fields inside a field, e.g. [Time-[CourseType]] or [Reg-bus-[StartDateNum]]
	if (preg_match_all('/\[((?:'.DH_LT_PART.'|\['.DH_LT_PART.'+\])*)\]/u', $text, $mm, PREG_SET_ORDER))
	{
		foreach ($mm as $m)
		{
			if (strpos($m[1], '[') === FALSE)
				continue; // a single field, checked below
			$raw = $m[0];
			$tags = $is_html ? _dh_letter_tags($raw) : '';
			// even pieces: the text around the inner fields; odd pieces: the inner fields
			$pieces = preg_split('/(\['.DH_LT_PART.'+\])/u', $m[1], -1, PREG_SPLIT_DELIM_CAPTURE);
			foreach ($pieces as $i => $pc)
				$pieces[$i] = _dh_letter_visible($pc, $is_html);
			$inners = array();
			for ($i = 1; $i < count($pieces); $i += 2)
				$inners[] = $pieces[$i];
			$show = '['.implode('', $pieces).']';
			foreach ($inners as $f)
				if (!isset($known[$f]))
					continue 2; // an unknown inner field: checked (with a suggestion) as a single field below
			$outer = '';
			for ($i = 0; $i < count($pieces); $i += 2)
				$outer .= $pieces[$i];
			if (count($inners) == 1 && trim($outer) === '')
			{
				$add(array('level' => 'error', 'raw' => $raw, 'fix' => $inners[0].$tags,
					'msg' => t('@t has extra brackets. Use @f.', array('@t' => $show, '@f' => $inners[0]))));
				$text = str_replace($raw, ' ', $text);
				continue;
			}
			$combos = dh_letter_combined_values($centre, $event, $ctype, $inners);
			if ($combos === NULL)
				continue; // e.g. brackets around [FirstName] [LastName]: nothing to check as a whole
			$text = str_replace($raw, ' ', $text);

			$names = array(); $missing = array();
			foreach ($combos as $label => $vals)
			{
				$name = '';
				foreach ($pieces as $i => $pc)
					$name .= ($i % 2) ? $vals[$pc] : $pc;
				$names[$label] = $name;
				if (!isset($known['['.$name.']']))
					$missing[$label] = $name;
				elseif ($known['['.$name.']'] === 'centre')
					$used['['.$name.']'] = 1;
			}

			// Every one missing, but the centre has e.g. [Time-10 Day Course]: the outer text is mistyped.
			$fixed = NULL;
			if ($missing && count($missing) == count($names) && count($inners) == 1)
			{
				foreach ($combos as $label => $vals)
				{
					$v = $vals[$inners[0]];
					$sg = _dh_letter_suggest('['.$names[$label].']', $known);
					if ($v !== '' && $sg && $sg[1] != 'spelling' && ($pos = strpos($sg[0], $v)) !== FALSE)
					{
						$fixed = '['.substr($sg[0], 1, $pos - 1).$inners[0].substr($sg[0], $pos + strlen($v), -1).']';
						break;
					}
				}
			}
			if ($fixed !== NULL && $fixed !== $show)
				$add(array('level' => 'error', 'raw' => $raw, 'fix' => $fixed.$tags,
					'msg' => t('@t does not match your letter fields. Did you mean @f?', array('@t' => $show, '@f' => $fixed))));
			elseif ($is_html && strpos($raw, '<') !== FALSE)
				$add(array('level' => 'fixed', 'raw' => $raw, 'fix' => $show.$tags,
					'msg' => t('@t had hidden formatting inside it, so DIPI could not fill it.', array('@t' => $show))));
			if ($missing && $fixed === NULL)
			{
				$list = array_values(array_unique($missing));
				$more = count($list) > 8 ? ' '.t('and @n more', array('@n' => count($list) - 8)) : '';
				$add(array('level' => 'warning', 'raw' => $raw, 'fix' => NULL, 'missing' => $list,
					'msg' => t('@t has no letter field for: @for. Those students will see the brackets. Add these in Manage Fields: @names',
						array('@t' => $show, '@for' => implode(', ', array_slice(array_keys($missing), 0, 8)).(count($missing) > 8 ? ' '.t('and @n more', array('@n' => count($missing) - 8)) : ''),
						'@names' => implode(', ', array_slice($list, 0, 8)).$more))));
			}
		}
	}

	// 2. Single fields: [Name]
	if (!preg_match_all('/\[('.DH_LT_PART.'{1,120})\]/u', $text, $mm, PREG_SET_ORDER))
		return;
	foreach ($mm as $m)
	{
		$raw = $m[0];
		$vis = _dh_letter_visible($m[1], $is_html);
		$tok = '['.$vis.']';
		$tags = $is_html ? _dh_letter_tags($raw) : '';
		if (isset($known[$tok]))
		{
			if ($known[$tok] === 'centre')
				$used[$tok] = 1;
			if ($raw !== $tok && strpos($raw, '<') !== FALSE)
				$add(array('level' => 'fixed', 'raw' => $raw, 'fix' => $tok.$tags,
					'msg' => t('@t had hidden formatting inside it, so DIPI could not fill it.', array('@t' => $tok))));
			continue;
		}
		$at_only = dh_letter_at_only_fields();
		if (isset($at_only[$tok]))
		{
			$add(array('level' => 'error', 'raw' => $raw, 'fix' => NULL,
				'msg' => t('@t only works in AT review letters (status R-ATReview or A-ATReview).', array('@t' => $tok))));
			continue;
		}
		$s = _dh_letter_near_field($vis) ? _dh_letter_suggest($tok, $known) : NULL;
		if ($s)
		{
			$why = array('case' => t('Capital letters must match exactly.'), 'space' => t('No spaces or extra marks inside a field.'), 'spelling' => t('Check the spelling.'));
			$add(array('level' => 'error', 'raw' => $raw, 'fix' => $s[2] ? $s[0].$tags : NULL,
				'msg' => t('@t is not a merge field. Did you mean @f? @why', array('@t' => $tok, '@f' => $s[0], '@why' => $why[$s[1]]))));
			continue;
		}
		// [EndDate 1.00pm]: the bracket was closed after the extra text instead of after the field
		foreach ($known as $k => $_)
		{
			$open = substr($k, 0, -1).' ';
			if (strlen($k) > 4 && strpos($tok, $open) === 0)
			{
				$fix = $k.substr($tok, strlen($open) - 1, -1);
				$add(array('level' => 'error', 'raw' => $raw, 'fix' => $fix.$tags,
					'msg' => t('@t: close the bracket right after the field name, like this: @f', array('@t' => $tok, '@f' => $fix))));
				continue 2;
			}
		}
		if (_dh_letter_field_like($vis))
			$add(array('level' => 'error', 'raw' => $raw, 'fix' => NULL,
				'msg' => t('@t is not a merge field. Square brackets [ ] are only for merge fields: for ordinary text use round brackets ( ). If it is your own field, add it in Manage Fields.', array('@t' => $tok))));
	}
}

/** Count problems by level. */
function dh_letter_check_counts($problems)
{
	$c = array('error' => 0, 'fixed' => 0, 'warning' => 0, 'field' => 0);
	foreach ($problems as $p)
		$c[$p['level']]++;
	return $c;
}

/**
 * Apply fixes to one part of a letter.
 * @param string $levels which problems to fix: array('fixed') on save, or array('error', 'fixed').
 */
function dh_letter_apply_fixes($text, $problems, $part, $levels = array('fixed'))
{
	foreach ($problems as $p)
		if ($p['part'] == $part && $p['fix'] !== NULL && in_array($p['level'], $levels))
			$text = str_replace($p['raw'], $p['fix'], $text);
	return $text;
}

/**
 * Field-like [..] tokens still in a merged text: fields DIPI could not fill. Tokens
 * that come from the filled-in values themselves (an address, the centre's comment)
 * are not counted.
 */
function dh_letter_unfilled($merged, $values_text, $is_html, $known = array())
{
	$out = array();
	// the visible text, and for HTML also the raw text (fields inside links, e.g. href="[ViewLink]")
	$texts = array(_dh_letter_visible($merged, $is_html));
	if ($is_html)
		$texts[] = $merged;
	foreach ($texts as $t)
		if (preg_match_all('/\[([^\[\]<>\r\n]{1,60})\]/u', $t, $mm))
			foreach ($mm[0] as $i => $tok)
				if (strpos($values_text, $tok) === FALSE && (_dh_letter_field_like($mm[1][$i])
						|| ($known && _dh_letter_near_field($mm[1][$i]) && _dh_letter_suggest($tok, $known))))
					$out[$tok] = 1;
	return array_keys($out);
}

/** Problem list as HTML (for the letter page and the editor). */
function dh_letter_problems_html($problems, $levels = array('error', 'warning'))
{
	$parts = array('subject' => t('Subject'), 'body' => t('Letter Body'), 'sms' => t('SMS Text'), 'value' => t('Value'), 'field' => t('Letter field'));
	$items = array();
	foreach ($problems as $p)
	{
		if (!in_array($p['level'], $levels))
			continue;
		$cls = $p['level'] == 'warning' ? 'lc-warn' : 'lc-err';
		$label = $p['part'] == 'field' ? t('Letter field @f', array('@f' => $p['field'])) : $parts[$p['part']];
		$items[] = '<li class="'.$cls.'"><span class="lc-part">'.$label.':</span> '.$p['msg'].'</li>';
	}
	return $items ? '<ul class="letter-check-list">'.implode('', $items).'</ul>' : '';
}
