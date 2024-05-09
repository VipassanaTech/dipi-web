<?php


function dh_create_edit_bulk_mail_schedule($bulk_mail_name, $where, $letter, $schedule_date, $criteria, $bm_id=0)
{
  global $user;
  $q = "select max(a_id) as id from
    dh_applicant
    left join dh_applicant_course ac on (a_id=ac_applicant)
    left join dh_applicant_attended aa on (a_id=aa_applicant)
    left join dh_applicant_extra ae on (a_id=ae_applicant)
    left join dh_course c on (a_course=c.c_id)
    left join dh_country co on (a_country=co.c_code)
    left join dh_city ci on a_city=ci.c_id
    left join dh_state s on (a_state=s.s_code and a_country=s.s_country)
    left join dh_center ce on (a_center=ce.c_id)
    where
    $where
    group by a_email";

  $q_count = "select count(distinct a_email) as id from
    dh_applicant
    left join dh_applicant_course ac on (a_id=ac_applicant)
    left join dh_applicant_attended aa on (a_id=aa_applicant)
    left join dh_applicant_extra ae on (a_id=ae_applicant)
    left join dh_course c on (a_course=c.c_id)
    left join dh_country co on (a_country=co.c_code)
    left join dh_city ci on a_city=ci.c_id
    left join dh_state s on (a_state=s.s_code and a_country=s.s_country)
    left join dh_center ce on (a_center=ce.c_id)
    where $where";

  $bm['bm_name'] = $bulk_mail_name;
  $bm['bm_center'] = arg(1);
  $bm['bm_count'] = db_query($q_count)->fetchField();
  $bm['bm_query'] = $q;
  $bm['bm_letter'] = $letter;
  $bm['bm_schedule_date'] = date('Y-m-d H:i:s', strtotime($schedule_date));

  unset($criteria['form_build_id'], $criteria['form_token'], $criteria['form_id'], $criteria['bulk-mail'], $criteria['db_type'], $criteria['op'], $criteria['bulk_mail_name'], $criteria['letters'], $criteria['bulk-mail-schedule'], );
  foreach ($criteria as $key => $value) {
    if(!$criteria[$key])
      unset($criteria[$key]);
  }

  $bm['bm_criteria'] = json_encode($criteria);
  // $criteria = theme('table', array('header' => array('Field', 'Value'), 'rows' => $temp));
  // var_dump($criteria);

  if($bm_id)
  {
    $bm['bm_updated'] = date('Y-m-d H:i:s');
    $bm['bm_updated_by'] = $user->uid;
    db_update('dh_bulk_mail')->fields($bm)->condition('bm_id', $bm_id)->execute();
  }
  else
  {
    $bm['bm_created_by'] = $user->uid;
    $bm['bm_updated_by'] = $user->uid;
    db_insert('dh_bulk_mail')->fields($bm)->execute();
  }

}


function dh_show_bulk_mail_schedule()
{
  $centre_id = arg(1);

  drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
  drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

  drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
  drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
  drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
  drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");


  $res = db_query("select bm.*, l_name, uc.name as 'created_by', uu.name as 'updated_by' from dh_bulk_mail bm left join dh_letter l on l.l_id=bm.bm_letter left join users uc on bm_created_by=uc.uid left join users uu on bm_updated_by=uu.uid where l_center=$centre_id and bm_deleted=0 order by bm_id desc");
  $rows = array();
  while( $r = $res->fetchAssoc() )
  {
    if($r['bm_processed'] == 1)
    {
      $temp['name'] = l($r['bm_name'], "bulk-mail/$centre_id/{$r['bm_id']}/show-log");
    }
    else
    {
      $temp['name'] = $r['bm_name'];
    }
    $temp['count'] = $r['bm_count'];
    $temp['letter'] = $r['l_name'];
    $temp['schedule'] = date('Y-m-d', strtotime($r['bm_schedule_date']));;
    $temp['created_by'] = $r['created_by']."<br>".$r['bm_created'];
    $temp['updated_by'] = $r['updated_by']."<br>".$r['bm_updated'];
    $criteria = json_decode($r['bm_criteria']);
    foreach ($criteria as $key => $value)
    {
      $temp['criteria'][] = array($key, nl2br($value));
    }
    $temp['criteria'] = theme('table', array('header' => array('Field', 'Value'), 'rows' => $temp['criteria']));

    if($r['bm_processed'] == 2)
    {
      $temp['action'] = "No Action";
      $temp['status'] = "In Progress";
    }
    elseif($r['bm_processed'] == 1)
    {
      $temp['action'] = "No Action";
      $temp['status'] = "Completed";
    }
    elseif($r['bm_processed'] == 3)
    {
      $temp['action'] = l("Edit", "bulk-mail/$centre_id/".$r['bm_id']."/edit", array("attributes" => array("class" => array("bulk-mail-edit"))))."<br>".l("Delete", "bulk-mail/$centre_id/".$r['bm_id']."/delete", array("attributes" => array("class" => array("bulk-mail-del"))))."<br>".l("Unmute", "bulk-mail/$centre_id/".$r['bm_id']."/unmute", array("attributes" => array("class" => array("bulk-mail-um"))));
      $temp['status'] = "Mute";
    }
    else
    {
      $temp['action'] = l("Edit", "bulk-mail/$centre_id/".$r['bm_id']."/edit", array("attributes" => array("class" => array("bulk-mail-edit"))))."<br>".l("Delete", "bulk-mail/$centre_id/".$r['bm_id']."/delete", array("attributes" => array("class" => array("bulk-mail-del"))))."<br>".l("Mute", "bulk-mail/$centre_id/".$r['bm_id']."/mute", array("attributes" => array("class" => array("bulk-mail-m"))));
      $temp['status'] = "Pending";
    }


    $rows[] = $temp;
    unset($temp);
  }
  $rows = json_encode($rows);
  $out = '<h3>'.l("Back to Dashboard", "centre/".arg(1)).'</h3>';
  $out .= theme('table', array('rows' => array(), 'attributes' => array('id' => 'bulk-mail-schedule')));

  $js = '
  (function ($) {
      function do_datatable( tid, data )
      {
        var table = $(tid).DataTable({
          dom: "lfr<\"clear\">tip",
          data: data,
          "ordering": false,
          destroy: true,
          "oLanguage": {"sEmptyTable": "No Bulk-Mail for Center."},
          searching: false,
          "columnDefs": [
                      {
                          "targets": [ 0 ],
                          "searchable": false
                      }
              ],
              columns: [
                  { data: "name", "title" : "Batch-Name" },
                  { data: "status", "title" : "Status" },
                  { data: "count", "title" : "Batch-Count" },
                  { data: "letter", "title" : "Letter" },
                  { data: "schedule", "title" : "Schedule-Data" },
                  { data: "created_by", "title" : "Created" },
                  { data: "updated_by", "title" : "Updated" },
                  { data: "criteria", "title" : "Criteria" },
                  { data: "action", "title" : "Action" },
              ],
          });
          }


        $(document).ready(function(){
        var dataset = '.$rows.';
        do_datatable("#bulk-mail-schedule", dataset);

        $("#bulk-mail-schedule tbody").on("click", ".bulk-mail-del", function() {
          return confirm("Are you sure you want to delete Bulk-Mail ?");
        });

        $("#bulk-mail-schedule tbody").on("click", ".bulk-mail-m", function() {
          return confirm("Are you sure you want to mute Bulk-Mail ?");
        });

        $("#bulk-mail-schedule tbody").on("click", ".bulk-mail-um", function() {
          return confirm("Are you sure you want to unmute Bulk-Mail ?");
        });

      });
    })(jQuery);
  ';
  drupal_add_js($js, 'inline');

  return $out;
}


function dh_delete_bulk_mail_schedule()
{
  $bm_id = arg(2);
  $centre_id = arg(1);

  $invalid = db_query("select bm_id from dh_bulk_mail where (bm_processed in (1, 2) or bm_deleted=1) and bm_id=$bm_id")->fetchField();

  if($invalid)
  {
    drupal_set_message("Can't take action on deleted/completed/in-progress bulk-email.", "error");
  }
  else
  {
    db_query("update dh_bulk_mail set bm_deleted=1 where bm_id=$bm_id and bm_center=$centre_id");
    drupal_set_message("Deleted Bulk-Mail-$bm_id", "status");
  }

  drupal_goto("centre/$centre_id/bulk-mail-schedule");
}


function dh_mute_bulk_mail_schedule()
{
  $bm_id = arg(2);
  $centre_id = arg(1);

  $invalid = db_query("select bm_id from dh_bulk_mail where (bm_processed in (1, 2) or bm_deleted=1) and bm_id=$bm_id")->fetchField();

  if($invalid)
  {
    drupal_set_message("Can't take action on deleted/completed/in-progress bulk-email.", "error");
  }
  else
  {
    db_query("update dh_bulk_mail set bm_processed=3 where bm_id=$bm_id and bm_center=$centre_id");
    drupal_set_message("Muted Bulk-Mail-$bm_id", "status");
  }

  drupal_goto("centre/$centre_id/bulk-mail-schedule");
}

function dh_unmute_bulk_mail_schedule()
{
  $bm_id = arg(2);
  $centre_id = arg(1);

  $invalid = db_query("select bm_id from dh_bulk_mail where (bm_processed in (1, 2) or bm_deleted=1) and bm_id=$bm_id")->fetchField();

  if($invalid)
  {
    drupal_set_message("Can't take action on deleted/completed/in-progress bulk-email.", "error");
  }
  else
  {
    db_query("update dh_bulk_mail set bm_processed=0 where bm_id=$bm_id and bm_center=$centre_id");
    drupal_set_message("Muted Bulk-Mail-$bm_id", "status");
  }

  drupal_goto("centre/$centre_id/bulk-mail-schedule");
}


function dh_edit_bulk_mail_schedule()
{
  $bm_id = arg(2);
  $centre_id = arg(1);

  $invalid = db_query("select bm_id from dh_bulk_mail where (bm_processed in (1, 2) or bm_deleted=1) and bm_id=$bm_id")->fetchField();

  if($invalid)
  {
    drupal_set_message("Can't take action on deleted/completed/in-progress bulk-email.", "error");
    drupal_goto("centre/$centre_id/bulk-mail-schedule");
  }
  else
  {
    $res = db_query("select bm_criteria, bm_name, bm_letter, date(bm_schedule_date) as schedule_date from dh_bulk_mail where bm_id=$bm_id and bm_center=$centre_id")->fetchAssoc();
    $criteria = (array)json_decode($res['bm_criteria']);
    $criteria['bulk_mail_name'] = $res['bm_name'];
    $criteria['letters'] = $res['bm_letter'];
    $criteria['bulk-mail-schedule'] = $res['schedule_date'];

    $out = drupal_get_form('dh_manageapp_search_form', $criteria);
    return $out;
  }
}


function dh_show_log_bulk_mail_schedule()
{
  $bm_id = arg(2);
  $centre_id = arg(1);

  drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
  drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

  drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
  drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
  drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
  drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");


  $res = db_query("select bml_applicant, bml_name, bml_email, bml_phone_mobile, bml_created from dh_bulk_mail_log where bml_bulk_mail=$bm_id and bml_center=$centre_id");
  $rows = array();
  while( $r = $res->fetchAssoc() )
  {
    $temp['name'] = l($r['bml_name'], "show-application/{$r['bml_applicant']}");
    $temp['email'] = $r['bml_email'];
    $temp['phone_mobile'] = $r['bml_phone_mobile'];
    $temp['created'] = $r['bml_created'];

    $rows[] = $temp;
    unset($temp);
  }
  $rows = json_encode($rows);
  $out = '<h3>'.l("Back to Bulk-Mail-Schedule", "centre/".arg(1)."/bulk-mail-schedule").' | '.l("Back to Dashboard", "centre/".arg(1)).'</h3>';
  $out .= theme('table', array('rows' => array(), 'attributes' => array('id' => 'bulk-mail-log')));

  $js = '
  (function ($) {
      function do_datatable( tid, data )
      {
        var table = $(tid).DataTable({
          dom: "lfr<\"clear\">tip",
          data: data,
          "ordering": false,
          destroy: true,
          "oLanguage": {"sEmptyTable": "No Bulk-Mail Logs."},
          searching: false,
              columns: [
                  { data: "name", "title" : "Applicant" },
                  { data: "email", "title" : "Email" },
                  { data: "phone_mobile", "title" : "Phone-Mobile" },
                  { data: "created", "title" : "Created" },
              ],
          });
          }


        $(document).ready(function(){
        var dataset = '.$rows.';
        do_datatable("#bulk-mail-log", dataset);
      });
    })(jQuery);
  ';
  drupal_add_js($js, 'inline');

  return $out;
}
