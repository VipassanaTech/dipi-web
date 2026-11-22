<?php
/**
 * Seat label start (22 Nov 2026). For one course, each grid (main / chowky) of
 * each section of the seating plans can start its labels later: style 1 from a
 * letter (I1, J1 ...), style 2 from a column number (A7, A8 ...), the numbered
 * style from a first seat number (101, 102 ...). Kept in dh_seat_label_start as
 * an offset (0 = the normal start). Design: private Claude Doc
 * "Seat Label Start and App Plan Mirror - Design".
 */

/**
 * Column letters to their index, the inverse of dh_seat_letter():
 * A = 0, Z = 25, AA = 26. FALSE unless 1 to 3 letters.
 */
function dh_seat_letter_index($text)
{
	$t = strtoupper(trim((string) $text));
	if (!preg_match('/^[A-Z]{1,3}$/', $t))
		return FALSE;
	$n = 0;
	for ($i = 0; $i < strlen($t); $i++)
		$n = $n * 26 + (ord($t[$i]) - 64);
	return $n - 1;
}

/**
 * What a person typed as a grid's start, as the offset to save. Empty = 0 (the
 * normal start). Style 1 takes a letter (I -> 8). Style 2 takes a column number
 * and the numbered style a first seat number, both 1 or more (7 -> 6, 101 -> 100).
 * FALSE when the text does not fit the style.
 */
function dh_seat_label_start_parse($conv, $text)
{
	$t = trim((string) $text);
	if ($t === '')
		return 0;
	if ((int) $conv == 1)
		return dh_seat_letter_index($t);
	if (!preg_match('/^[0-9]{1,5}$/', $t) || (int) $t < 1)
		return FALSE;
	return (int) $t - 1;
}

/** An offset as a person would type it: '' for the normal start, style 1 a letter, else a number. */
function dh_seat_label_start_text($conv, $start)
{
	$start = (int) $start;
	if ($start <= 0)
		return '';
	return ((int) $conv == 1) ? dh_seat_letter($start) : (string) ($start + 1);
}

/** What to type, for an error message after "Main grid: ". */
function dh_seat_label_start_hint($conv)
{
	$conv = (int) $conv;
	if ($conv == 1)
		return 'type a letter, for example I. Leave it empty for the normal start.';
	if ($conv == 2)
		return 'type a column number of 1 or more, for example 7. Leave it empty for the normal start.';
	return 'type a seat number of 1 or more, for example 101. Leave it empty for the normal start.';
}
