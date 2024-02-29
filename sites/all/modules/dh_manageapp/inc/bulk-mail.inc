<?php


function dh_bulk_mail_schedule($bulk_mail_name, $where, $letter, $schedule_date, $criteria)
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

  $q_count = "select count(*) from ($q) a";
  $count = db_query($q_count)->fetchField();


  $schedule_date = date('Y-m-d H:i:s', strtotime($schedule_date));
  db_query("insert into dh_bulk_mail (bm_name, bm_count, bm_query, bm_letter, bm_schedule_date, bm_user) values (:name, :count, :query, :letter, :date, :user)", array(":name"=>$bulk_mail_name, ":count"=>$count, ":query"=>$q, ":letter"=>$letter, ":date"=>$schedule_date, ":user"=>$user->uid, ));
}


function dh_center_bulk_mail_schedule()
{
  $centre_id = arg(1);

  drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
  drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

  drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
  drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
  drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
  drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");


  $res = db_query("select bm.*, l_center, l_name, name from dh_bulk_mail bm left join dh_letter l on l.l_id=bm.bm_letter left join users u on bm.bm_user=u.uid where l_center=$centre_id order by bm_id desc");
  $rows = array();
  while( $r = $res->fetchAssoc() )
  {
    $temp['id'] = $r['bm_id'];
    $temp['name'] = $r['bm_name'];
    $temp['count'] = $r['bm_count'];
    $temp['letter'] = $r['l_name'];
    $temp['schedule'] = date('Y-m-d', strtotime($r['bm_schedule_date']));;
    $temp['user'] = $r['name'];

    $rows[] = $temp;
    unset($temp);
  }
  $rows = json_encode($rows);
  $out = theme('table', array('rows' => array(), 'attributes' => array('id' => 'bulk-mail-schedule')));

  $js = '
  (function ($) {
      function do_datatable( tid, data )
      {
        var table = $(tid).DataTable({
          dom: "lfr<\"clear\">tip",
          data: data,
          "ordering": false,
          destroy: true,
          "oLanguage": {"sEmptyTable": "No AT Applications for the Course."},
          searching: false,
          "columnDefs": [
                      {
                          "targets": [ 0 ],
                          "searchable": false
                      }
              ],
              columns: [
                  { data: "id", "title" : "ID" },
                  { data: "name", "title" : "Batch-Name" },
                  { data: "count", "title" : "Batch-Count" },
                  { data: "letter", "title" : "Letter" },
                  { data: "schedule", "title" : "Schedule-Data" },
                  { data: "user", "title" : "User" },
              ],
          });
          }


        $(document).ready(function(){
        var dataset = '.$rows.';
        do_datatable("#bulk-mail-schedule", dataset);
      });
    })(jQuery);
  ';
  drupal_add_js($js, 'inline');

  return $out;
}
