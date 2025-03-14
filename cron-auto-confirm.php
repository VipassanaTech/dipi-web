<?php


if (isset($_SERVER) && ( isset($_SERVER['REMOTE_ADDR']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}


$lock_file = "cron-auto-confirm.lock";
$f = fopen($lock_file, 'w') or die ("Cannot create/open lock file $lock_file, exiting!\n");

if (!flock($f, LOCK_EX | LOCK_NB))
{
  die ("not able to lock $lock_file, exiting!\n");
}



define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// setting auto confirm for global pagoda courses
db_query("update dh_course set c_auto_confirm=1 where c_center=308 and c_start>curdate() and c_auto_confirm=0");

// auto confirming/rejecting applications for courses set to autoconfirm on

$q = "select a_id, a_course, a_f_name, a_l_name, a_email, a_status, a_dob, a_zip from dh_applicant a left join dh_course c on (a.a_course=c.c_id) where c.c_start >= CURDATE() and c.c_auto_confirm=1 and a.a_status in ('Received') and a_type='Student' limit 50";
$result = db_query($q);
while ($row = $result->fetchAssoc()) 
{
   echo $row['a_id']." - ".$row['a_f_name']." ".$row['a_l_name']."\n";
   $check = "select count(a_id) from dh_applicant where a_course=".$row['a_course']." and a_f_name= :fname and a_l_name = :lname and a_email = :email and a_status in ('Confirmed', 'Expected', 'ReConfirmation')";
   $ret = db_query($check, array(':fname' => $row['a_f_name'], ':lname' => $row['a_l_name'], ':email' => $row['a_email']))->fetchField();
   try 
   {
      if ($ret <= 0)
      {
	      echo "inside ret <= 0\n";
        if ($row['a_email'] <> '')
          $append = " and a_email='".$row['a_email']."'";
        elseif ($row['a_zip'] <> '')
          $append = " and a_zip='".$row['a_zip']."'";
        else
          $append = " and a_dob='".$row['a_dob']."'";

	echo "append: $append\n";
        $attended = db_query("select count(*) from dh_applicant left join dh_course on a_course=c_id left join dh_type_detail on td_id=c_course_type where a_f_name=:a_f_name and a_l_name=:a_l_name and td_val3 not in ('1-Day') and a_attended='1' $append", array(':a_f_name' => $row['a_f_name'], ':a_l_name' => $row['a_l_name']))->fetchField();
	echo "attended: $attended\n";
        if($attended >=1)
        {
         update_status_external($row['a_id'], 'Confirmed');
        }
        else
        {
          _update_status($row['a_id'], "Clarification");
          dh_send_letter('applicant', $row['a_id'], "Clarification", "7933");
        }
      }
      else
      {
         update_status_external($row['a_id'], 'Rejected');
         logit($row['a_center'], 'Auto-Reject', $row['a_id'], 'Auto rejected due to duplicate applications');
      }
      
   } catch (Exception $e) 
   {
      //   
   }
}



?>
