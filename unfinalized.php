<?php

if (isset($_SERVER) && ( isset($_SERVER['REMOTE_ADDR']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}

define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

if ( $argc < 2)
{
   echo "Usage: php ".$argv[0]." <center_id> <course_id>\n";
   exit(1);
}

$center_id = $argv[1];
$course_id = $argv[2];

if ( !is_numeric($center_id) || !is_numeric($course_id) )
{
   echo "center_id and course_id should be numeric\n";
   exit(1);
}


$res = db_query("select c_id, c_finalized from dh_course where c_id=:c_id and c_center=:c_center", array(':c_id'=>$course_id, ':c_center' => $center_id))->fetchAssoc();

if(!$res || !($res['c_id']))
  exit("Course Id not found, exiting!\n");

if(!($res['c_finalized']))
  exit("Course is not finalized yet, exiting!\n");

$course_id = $res['c_id'];


db_query("delete from dh_course_stat where cs_course=:cs_course", array(':cs_course' => $course_id));

db_query("delete from dh_student_course where sc_course=:sc_course", array(':sc_course' => $course_id));

$res = db_query("update dh_course set c_finalized=0 where c_id=:c_id and c_center=:c_center", array(':c_id'=>$course_id, ':c_center' => $center_id));

