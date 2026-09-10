<?php
// sites/all/modules/dh_manageapp/inc/bulk-mail-common.inc
// Shared recipient/criteria/enqueue helpers used by BOTH centre bulk email
// (bulk-mail.inc) and admin bulk email (admin-bulk-mail.inc).

/**
 * Build the recipient SELECT (one representative a_id per distinct a_email) and
 * the recipient count for a given WHERE fragment. The WHERE decides scope:
 * centre passes "a_center='X' and ...", admin passes "a_center in (...)"/"1=1".
 */
function _dh_bulk_mail_joins() {
  return "
    dh_applicant
    left join dh_applicant_course ac on (a_id=ac_applicant)
    left join dh_applicant_attended aa on (a_id=aa_applicant)
    left join dh_applicant_extra ae on (a_id=ae_applicant)
    left join dh_course c on (a_course=c.c_id)
    left join dh_country co on (a_country=co.c_code)
    left join dh_city ci on a_city=ci.c_id
    left join dh_state s on (a_state=s.s_code and a_country=s.s_country)
    left join dh_center ce on (a_center=ce.c_id)";
}

function dh_bulk_mail_build_recipient_query($where) {
  $joins = _dh_bulk_mail_joins();
  $query = "select max(a_id) as id from $joins where $where group by a_email";
  $count = db_query("select count(distinct a_email) as id from $joins where $where")->fetchField();
  return array('query' => $query, 'count' => (int) $count);
}

/**
 * Read-only recipient preview (one row per distinct email): name, email, centre.
 * Used by the admin bulk-email targeting page to show a small sample instead of the
 * full interactive search-results table.
 */
function dh_bulk_mail_preview_recipients($where, $limit = 25) {
  $joins = _dh_bulk_mail_joins();
  $q = "select concat(ifnull(a_f_name,''),' ',ifnull(a_l_name,'')) as name, a_email, ce.c_name as centre from $joins where $where group by a_email order by a_email limit " . (int) $limit;
  return db_query($q);
}

/**
 * Clean raw search form values into a JSON-ready criteria snapshot:
 * implode multi-selects, drop internal keys, drop empties.
 */
function dh_bulk_mail_clean_criteria($criteria) {
  foreach (array('status', 'lang_discourse', 'state', 'country') as $k) {
    if (isset($criteria[$k]) && is_array($criteria[$k])) {
      $criteria[$k] = implode(', ', $criteria[$k]);
    }
  }
  unset(
    $criteria['form_build_id'], $criteria['form_token'], $criteria['form_id'],
    $criteria['bulk-mail'], $criteria['db_type'], $criteria['op'],
    $criteria['bulk_mail_name'], $criteria['letters'],
    $criteria['bulk-mail-schedule'], $criteria['bm_id'],
    $criteria['abm_centres']
  );
  foreach ($criteria as $key => $value) {
    if (!$criteria[$key]) {
      unset($criteria[$key]);
    }
  }
  return json_encode($criteria);
}

/**
 * Run a recipient query and push one 'mail' queue message per recipient, then a
 * completion sentinel. $build_msg($applicant_id) returns the per-recipient array.
 */
function dh_bulk_mail_enqueue($query, callable $build_msg, array $done_msg) {
  $res = db_query($query);
  while ($r = $res->fetchAssoc()) {
    push_to_queue('Dipi', 'mail', json_encode($build_msg($r['id'])), 86400000);
  }
  push_to_queue('Dipi', 'mail', json_encode($done_msg), 86400000);
}
