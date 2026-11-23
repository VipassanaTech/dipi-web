<?php
// [LetterDetailURL] is gone: the merge field is no longer filled or offered, and the clean-up script
// (deploy/2026-11-23-remove-letter-detail-url.php) takes it out of letter texts without leaving half sentences.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-detail-url.php
require __DIR__ . '/lib.php';
define('LDU_FUNCTIONS_ONLY', TRUE);
require __DIR__ . '/../deploy/2026-11-23-remove-letter-detail-url.php';

echo "SMS text and centre fields (plain text)\n";
$cases = array(
	array("Dear [FirstName], we require some clarification. Please click here for more details - [LetterDetailURL] - [CentreName]",
		"Dear [FirstName], we require some clarification. - [CentreName]", 'lead-in and field out, sign-off kept'),
	array("Your Confirmation No - [ConfirmationNo] for the course at [CentreName]. For Details [LetterDetailURL]",
		"Your Confirmation No - [ConfirmationNo] for the course at [CentreName].", 'field at the end'),
	array("from [StartDate] to [EndDate]. For more details - [LetterDetailURL] ",
		"from [StartDate] to [EndDate].", 'trailing space'),
	array("at [CentreName]. For Details [LetterDetailURL] With Metta, Registrar",
		"at [CentreName]. With Metta, Registrar", 'next sentence kept'),
	array("at [CentreName]. For Details [LetterDetailURL]. Please confirm IMMEDIATELY.",
		"at [CentreName]. Please confirm IMMEDIATELY.", 'full stop after the field goes too'),
	array("at [CentreName]. For Details [LetterDetailURL] [UserInput]",
		"at [CentreName]. [UserInput]", 'another field after it kept'),
	array("at [CentreName]. Please carefully go through the information at [LetterDetailURL] and reconfirm your participation by writing to reg.centre@gmail.com or by phone.",
		"at [CentreName]. Please carefully go through the information and reconfirm your participation by writing to reg.centre@gmail.com or by phone.",
		'sentence that carries on after the field: kept, only "at [field]" goes (email address with a dot untouched)'),
	array("Your conf. no - [ConfirmationNo]. For more details - [LetterDetail]\r\nIf you are not able to attend, please cancel.",
		"Your conf. no - [ConfirmationNo].\r\nIf you are not able to attend, please cancel.", 'broken spelling [LetterDetail]; line break kept'),
	array("at [CentreName]. For Details [LetterDetailURL", "at [CentreName].", 'no closing bracket'),
	array("at [CentreName]. For Details [LetterDetailURLYour]", "at [CentreName].", '[LetterDetailURLYour]'),
	array("ता:  [StartDate] - [EndDate]  | ज्यादा जानकारी के लिए  [LetterDetailURL\r\n\r\n",
		"ता:  [StartDate] - [EndDate]\r\n\r\n", 'Hindi after "|": lead-in and "|" out'),
	array("आपका अनुमोदन क्रमांक [ConfirmationNo] है। सविस्तर जानकारी के लिए [LetterDetailURL]",
		"आपका अनुमोदन क्रमांक [ConfirmationNo] है।", 'Hindi after a full stop (।)'),
	array("Your application has been cancelled. We hope that you are able to attend the course next time .[LetterDetailURL] ",
		"Your application has been cancelled. We hope that you are able to attend the course next time .", 'full stop glued to the field'),
	array("Dear [FirstName] [LastName], យើងសូមការបញ្ជាក់មួយចំនួន សូមចុចនៅទីនេះសម្រាប់ការលំអិត - [LetterDetailURL] - [CentreName]",
		"Dear [FirstName] [LastName], យើងសូមការបញ្ជាក់មួយចំនួន - [CentreName]", 'Khmer lead-in with no full stop before it'),
	array("Сайн байна уу,\r\nTaнд мэдэгдье.\r\n\r\nДэлгэрэнгүй мэдээлэл болон холбогдох шаардлагуудтай танилцахын тулд дараах холбоос дээр дарна уу:\r\n\r\n[LetterDetailURL]",
		"Сайн байна уу,\r\nTaнд мэдэгдье.\r\n\r\n", 'field on its own line under "...link:" (Mongolian): that long line goes too'),
	array("Dear [FirstName] [LastName],\r\nសម្រាប់ពាក្យស្នើសុំរបស់លោកអ្នក យើងសូមការបញ្ជាក់មួយចំនួន សូមចុចនៅទីនេះសម្រាប់ការលំអិត- [LetterDetailURL]\r\n- [CentreName]",
		"Dear [FirstName] [LastName],\r\nសម្រាប់ពាក្យស្នើសុំរបស់លោកអ្នក យើងសូមការបញ្ជាក់មួយចំនួន\r\n- [CentreName]", 'Khmer: the clarification sentence stays, only the lead-in goes'),
	array("Dear [FirstName],\r\nWe have emailed some information to help you prepare for the upcoming Vipassana course [LetterDetailURL]",
		"Dear [FirstName],\r\nWe have emailed some information to help you prepare for the upcoming Vipassana course", 'a long real sentence stays, only the field goes'),
	array("Dear [FirstName], see [LetterDetailURL] for the details of [CourseName]",
		"Dear [FirstName], see for the details of [CourseName]", 'inside a sentence with other fields: only the field'),
	array("More details - [LetterDetailURL]", "", 'centre field that was only the link becomes empty'),
	array("Dear [FirstName], no link here.", "Dear [FirstName], no link here.", 'no field: unchanged'),
	array("For details [LetterDetailURL]. Thanks. For more details - [LetterDetailURL] - [CentreName]", "Thanks. - [CentreName]", 'two fields'),
);
foreach ($cases as $c)
	t_eq(ldu_clean_text($c[0]), $c[1], $c[2]);

echo "Email body (HTML)\n";
$b = "<p>Your confirmation number is <strong>[ConfirmationNo]</strong>. You may view your complete application and course details at the link below:<br />\n\xF0\x9F\x91\x89 [LetterDetailURL]</p>\n<p>We look forward</p>";
t_eq(ldu_clean_html($b), "<p>Your confirmation number is <strong>[ConfirmationNo]</strong>.</p>\n<p>We look forward</p>", 'sentence pointing at the link goes');
$b = '<table><tr><td>ConfirmationNo</td><td>[ConfirmationNo]</td></tr><tr><td><span>LetterDetailURL</span></td><td class="x"><span>[LetterDetailURL]</span></td></tr><tr><td>ApplicantType</td><td>[ApplicantType]</td></tr></table>';
t_eq(ldu_clean_html($b), '<table><tr><td>ConfirmationNo</td><td>[ConfirmationNo]</td></tr><tr><td>ApplicantType</td><td>[ApplicantType]</td></tr></table>', 'table row with the field goes whole');
$b = '<p>[UserInput]<br />[AreaTeacher]<br />[LetterDetailURL]<br />[ApplicantType]<br /></p>';
t_eq(ldu_clean_html($b), '<p>[UserInput]<br />[AreaTeacher]<br />[ApplicantType]<br /></p>', 'list of fields: the field and its line break go');
t_eq(ldu_clean_html('<p>Hello [FirstName]</p>'), '<p>Hello [FirstName]</p>', 'no field: unchanged');

echo "Code: the field is gone\n";
t_ok(!in_array('[LetterDetailURL]', _dh_letter_builtin_fields()), 'not one of DIPI\'s own fields (Merge Fields box)');
$src = file_get_contents(__DIR__ . '/../inc/letters.inc');
t_ok(strpos($src, 'LetterDetailURL') === FALSE && strpos($src, 'l.php') === FALSE, 'letters.inc no longer fills it (no l.php link)');
$unfilled = _dh_letter_unfilled_all('', '', 'For Details [LetterDetailURL]', array(), 0);
t_ok(in_array('[LetterDetailURL]', $unfilled), 'a letter that still has it shows it as an unfilled field');
t_done();
