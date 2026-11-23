<?php
// dh_name_case(): a name saved as "First letter capital, rest small", for every alphabet.
// The old ucwords(strtolower()) knew only A-Z, so 'NGUYỄN' was saved as 'NguyỄn'.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-name-case.php
require __DIR__ . '/lib.php';

// Accented capitals after the first letter: the bug this fixes.
t_eq(dh_name_case('NGUYỄN VĂN'), 'Nguyễn Văn', 'Vietnamese in capitals');
t_eq(dh_name_case('ĐỨC'), 'Đức', 'Vietnamese Đ word in capitals');
t_eq(dh_name_case('ИВАН ПЕТРОВ'), 'Иван Петров', 'Cyrillic in capitals');

// Same as before for plain English letters.
t_eq(dh_name_case('MOHAN KUMAR'), 'Mohan Kumar', 'English in capitals');
t_eq(dh_name_case('mohan kumar'), 'Mohan Kumar', 'English in small letters');
t_eq(dh_name_case('mOHAN'), 'Mohan', 'mixed case');
t_eq(dh_name_case("D'SOUZA"), "D'souza", 'apostrophe: same as before');
t_eq(dh_name_case('MCDONALD'), 'Mcdonald', 'Mc: same as before');
t_eq(dh_name_case('ram  kumar'), 'Ram  Kumar', 'two spaces kept');

// Already right: unchanged.
t_eq(dh_name_case('Nguyễn'), 'Nguyễn', 'Vietnamese typed normally');
t_eq(dh_name_case('ÉLODIE'), 'Élodie', 'accented first letter');

// One small change: a capital after a hyphen too.
t_eq(dh_name_case('ANNE-MARIE'), 'Anne-Marie', 'hyphen: capital after it');

// Scripts with no capitals: unchanged.
t_eq(dh_name_case('मोहन कुमार'), 'मोहन कुमार', 'Devanagari unchanged');

// Nothing in, nothing out.
t_eq(dh_name_case(''), '', 'empty');
t_eq(dh_name_case(NULL), '', 'NULL');
t_done();
