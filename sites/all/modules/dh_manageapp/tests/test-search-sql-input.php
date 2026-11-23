<?php
// Search pages only let known values into their database question:
//  - search-course/{centre}/{course}?s=&g=&t= : status words, gender M/F, old/new 0/1 (anything else is ignored)
//  - search-app form, "Least/Max number of ..." boxes: whole numbers only
//  - search-lc/{status}: only the LC review statuses
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-search-sql-input.php
require __DIR__ . '/lib.php';
$GLOBALS['user'] = user_load(1); // sees both genders, like a registrar with male + female access

echo "Status words from the address (?s=)\n";
t_eq(dh_search_status_list('Confirmed'), array('Confirmed'), 'one status');
t_eq(dh_search_status_list('Clarification-Response,Received'), array('Clarification-Response', 'Received'), 'two statuses (comma)');
t_eq(dh_search_status_list('R-ATReview'), array('R-ATReview'), 'LC status with hyphen');
t_eq(dh_search_status_list(''), array(), 'empty');
t_eq(dh_search_status_list("Confirmed') union select name, pass from users -- "), array(), 'SQL in the status is dropped');
t_eq(dh_search_status_list("Confirmed,Received' or '1'='1"), array('Confirmed'), 'only the bad part is dropped');
t_eq(dh_search_status_list('<script>alert(1)</script>'), array(), 'HTML/script is dropped');
t_eq(dh_search_status_list(array('Confirmed')), array(), 'an array (?s[]=) is ignored');

echo "Gender and old/new from the address (?g=, ?t=)\n";
t_eq(dh_search_choice('M', array('M', 'F')), 'M', 'gender M');
t_eq(dh_search_choice('F', array('M', 'F')), 'F', 'gender F');
t_eq(dh_search_choice("M' or '1'='1", array('M', 'F')), '', 'SQL in gender is ignored');
t_eq(dh_search_choice(array('M'), array('M', 'F')), '', 'array gender is ignored');
t_eq(dh_search_choice('1', array('0', '1')), '1', 'old student 1');
t_eq(dh_search_choice('0', array('0', '1')), '0', 'new student 0');
t_eq(dh_search_choice('1 or 1=1', array('0', '1')), '', 'SQL in old/new is ignored');
t_eq(dh_search_choice(NULL, array('0', '1')), '', 'missing');

echo "search-course filter (the database question it builds)\n";
$f = dh_search_course_filter(64802, array('s' => 'Clarification-Response,Received', 'g' => 'M', 't' => '1'));
t_eq($f['where'], " a_course=64802 and a_status IN ('Clarification-Response','Received')  and a_gender ='M'  and a_old ='1' ", 'normal link: same question as before');
t_eq($f['msg'], 'Status: Clarification-Response,Received, Gender: MaleOld', 'normal link: same heading as before');
$f = dh_search_course_filter(64802, array('s' => '', 't' => '', 'g' => ''));
t_eq($f['where'], ' a_course=64802', 'all statuses');
t_eq($f['msg'], 'Status: All', 'all statuses heading');
$f = dh_search_course_filter(64802, array('s' => 'Confirmed', 'g' => 'F', 'at' => 's'));
t_eq($f['where'], " a_course=64802 and a_type='Sevak' and a_status IN ('Confirmed')  and a_gender ='F' ", 'servers link');
$f = dh_search_course_filter(64802, array('s' => '', 'g' => 'M', 't' => '0', 'd' => 's'));
t_eq($f['type'], 'student', 'student list (d=s)');
t_eq($f['where'], " sc_course=64802 and s_gender ='M'  and s_first ='1' ", 'student list: same question as before');
$bad = dh_search_course_filter(64802, array('s' => "Confirmed') union select 1,2,3 -- ", 'g' => "M' or '1'='1", 't' => '1 or 1=1'));
t_eq($bad['where'], ' a_course=64802', 'injected values: nothing of them reaches the question');
t_ok(strpos($bad['msg'], 'union') === FALSE, 'injected values: not shown in the heading');
$x = dh_search_course_filter(64802, array('s' => '<img src=x onerror=alert(1)>'));
t_ok(strpos($x['msg'], '<') === FALSE, 'script in the status is not put on the page');

echo "Gender access still forced for users who see one gender\n";
$one = (object) array('uid' => 999999, 'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user'));
$GLOBALS['user'] = $one;
drupal_static_reset('user_access');
$f = dh_search_course_filter(64802, array('s' => '', 'g' => 'M'));
t_ok(strpos($f['where'], "a_gender ='F'") !== FALSE && strpos($f['where'], "'M'") === FALSE, 'user without male access gets F only, whatever ?g= says');
$GLOBALS['user'] = user_load(1);
drupal_static_reset('user_access');

echo "Search form: Least/Max number boxes\n";
t_eq(dh_search_count_where(array('least_10_day' => '2', 'max_service' => '5')), " and ac.ac_10d >= 2  and ac.ac_service <= 5 ", 'numbers: same question as before');
t_eq(dh_search_count_where(array('least_10_day' => '0', 'max_60_day' => '')), '', 'empty and 0 are skipped, as before');
t_eq(dh_search_count_where(array('least_stp' => '1 or 1=1', 'max_20_day' => "3) union select 1 -- ")), " and ac.ac_stp >= 1  and ac.ac_20d <= 3 ", 'SQL in a number box: only the number is used');
t_eq(dh_search_count_where(array('least_30_day' => 'abc')), " and ac.ac_30d >= 0 ", 'text in a number box becomes 0');
t_eq(dh_search_count_where(array()), '', 'nothing filled');
$src = file_get_contents(__DIR__ . '/../inc/search.inc');
t_ok(!preg_match('/\{\$storage\[\'(least|max)_/', $src), 'no number box is pasted into the question any more');

echo "search-lc/{status}\n";
t_eq(dh_search_lc_status('r-atreview'), 'R-ATReview', 'R-AT Review link');
t_eq(dh_search_lc_status('a-atreview'), 'A-ATReview', 'A-AT Review link');
t_eq(dh_search_lc_status('R-ATTransfer'), 'R-ATTransfer', 'R-AT Transfer');
t_eq(dh_search_lc_status("r-atreview') or ('1'='1"), '', 'SQL in the address is refused');
t_eq(dh_search_lc_status('Confirmed'), '', 'not an LC status');
t_eq(dh_search_lc_status(''), '', 'empty');
t_done();
