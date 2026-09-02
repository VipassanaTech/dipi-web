<?php

/**
 * AT read-only course viewing.
 *
 * Lets an Assistant Teacher view the centre courses they are mapped to serve
 * (Confirmed, Conducting/Assisting in dh_course_teacher) and drill into the
 * course's applicants — WITHOUT any edit ability.
 *
 * Design: dedicated read-only routes in the AT portal, scoped per-course via
 * dh_course_teacher (the portal's own identity/scoping model), reusing
 * dh_manageapp's pure-read renderers/workers (dh_application_view,
 * generate_course_pdf, create_d11_report_pdf) but NONE of its (write-capable)
 * routes. Everything here is gated by the 'at view courses' permission plus the
 * per-course loader; the AT holds no dh_manageapp write permissions, so no
 * mutation is reachable.
 *
 * Routes + permission are registered in dh_atportal.module.
 */


/**
 * Resolve the logged-in AT (username = t_code.t_gender) to an active t_id.
 * Returns the teacher id, or FALSE if the current user is not an active AT.
 */
function _at_view_teacher_id() {
  global $user;
  if (empty($user->name)) {
    return FALSE;
  }
  return db_query(
    "select t_id from dh_teacher where t_status='Active' and CONCAT(t_code,'.',t_gender)=:n limit 1",
    array(':n' => $user->name)
  )->fetchField();
}


/**
 * Menu loader for %at_view_course.
 *
 * Grants access to a course ONLY if the logged-in AT is Confirmed on it as
 * Conducting or Assisting (trainees excluded) and the course is live. Returns
 * an assoc row (course meta) used by the page callbacks, or FALSE (=> 404).
 */
function at_view_course_load($course_id) {
  if (!is_numeric($course_id)) {
    return FALSE;
  }
  $tid = _at_view_teacher_id();
  if (!$tid) {
    return FALSE;
  }

  $q = "select
      co.c_id        as course,
      co.c_center    as center,
      co.c_name      as course_name,
      co.c_start     as start_date,
      co.c_end       as end_date,
      ce.c_name      as centre_name,
      td.td_val1     as course_type,
      ct.ct_type     as my_role,
      ct.ct_group    as my_group
    from dh_course_teacher ct
    left join dh_course      co on ct.ct_course=co.c_id
    left join dh_center      ce on co.c_center=ce.c_id
    left join dh_type_detail td on co.c_course_type=td.td_id
    where ct.ct_teacher=:tid
      and ct.ct_course=:cid
      and ct.ct_status='Confirmed'
      and ct.ct_type in ('Conducting','Assisting')
      and co.c_deleted=0
      and co.c_cancelled=0
    limit 1";

  $row = db_query($q, array(':tid' => $tid, ':cid' => $course_id))->fetchAssoc();
  return $row ? $row : FALSE;
}


/**
 * TRUE if the given applicant's course is one the logged-in AT is Confirmed to
 * serve (Conducting/Assisting). Shared by the %at_view_app loader here and by
 * dh_manageapp's app_id_load() fallback, so the reused registrar pages
 * (application-view, applicant-reviews, add-review, show-application) become
 * reachable for an AT — scoped to their own courses only.
 */
function _at_view_applicant_in_my_courses($app_id) {
  if (!is_numeric($app_id)) {
    return FALSE;
  }
  $tid = _at_view_teacher_id();
  if (!$tid) {
    return FALSE;
  }
  $course = db_query("select a_course from dh_applicant where a_id=:id", array(':id' => $app_id))->fetchField();
  if (!$course) {
    return FALSE;
  }
  $ok = db_query(
    "select count(*) from dh_course_teacher
     where ct_teacher=:tid and ct_course=:c
       and ct_status='Confirmed' and ct_type in ('Conducting','Assisting')",
    array(':tid' => $tid, ':c' => $course)
  )->fetchField();

  return ($ok > 0);
}


/**
 * Menu loader for %at_view_app. Returns the numeric app id if the applicant is
 * in one of the AT's courses, else FALSE (=> 404).
 */
function at_view_app_load($app_id) {
  return _at_view_applicant_in_my_courses($app_id) ? $app_id : FALSE;
}


/**
 * Page: "My Courses" — list the courses this AT is mapped to serve.
 */
function at_view_my_courses() {
  drupal_set_title('');
  $tid = _at_view_teacher_id();
  if (!$tid) {
    drupal_set_message(t('Only an active Assistant Teacher can view courses.'), 'error');
    drupal_goto('at-portal');
  }

  $q = "select
      co.c_id     as course,
      ce.c_name   as centre_name,
      td.td_val1  as course_type,
      co.c_start  as start_date,
      co.c_end    as end_date,
      ct.ct_type  as my_role
    from dh_course_teacher ct
    left join dh_course      co on ct.ct_course=co.c_id
    left join dh_center      ce on co.c_center=ce.c_id
    left join dh_type_detail td on co.c_course_type=td.td_id
    where ct.ct_teacher=:tid
      and ct.ct_status='Confirmed'
      and ct.ct_type in ('Conducting','Assisting')
      and co.c_deleted=0
      and co.c_cancelled=0
    order by co.c_start desc";

  $res = db_query($q, array(':tid' => $tid));

  // Split into upcoming/current (not yet ended) vs past (already ended).
  $today = date('Y-m-d');
  $up = array();
  $past = array();
  foreach ($res as $r) {
    if ($r->end_date >= $today) { $up[] = $r; } else { $past[] = $r; }
  }
  $up = array_reverse($up); // query is start-desc; upcoming reads best soonest-first

  drupal_add_css(drupal_get_path('module', 'dh_atportal') . '/css/at-view.css');
  drupal_add_css(libraries_get_path('editor') . '/DataTables/datatables.min.css');
  drupal_add_js(libraries_get_path('editor') . '/DataTables/datatables.min.js');

  $out  = '<div class="at-view">';
  $out .= '<div class="at-page-head"><h1>' . t('My Courses') . '</h1>'
        . '<p class="at-sub">' . t('Courses you are confirmed to serve as a Conducting or Assisting teacher — view only.') . '</p></div>';
  $out .= '<div style="margin:2px 0 14px"><a class="at-back" href="' . url('at-portal') . '">' . t('← Back to AT Portal') . '</a></div>';
  $out .= _at_courses_accordion('at-up', t('Upcoming & Current'), $up, TRUE);
  $out .= _at_courses_accordion('at-past', t('Past courses'), $past, FALSE);
  $out .= '<a class="at-back" href="' . url('at-portal') . '">' . t('← Back to AT Portal') . '</a>';
  $out .= '</div>';

  $js = "(function(\$){\n"
    . "  \$(function(){\n"
    . "    var cfg = {paging:true, pageLength:10, lengthChange:true, autoWidth:false, columnDefs:[{orderable:false, targets:5}], language:{search:'Filter:'}};\n"
    . "    var dts = [];\n"
    . "    if (\$('#at-up-tbl').length)   dts.push(\$('#at-up-tbl').DataTable(\$.extend({}, cfg, {order:[[2,'asc']]})));\n"
    . "    if (\$('#at-past-tbl').length) dts.push(\$('#at-past-tbl').DataTable(\$.extend({}, cfg, {order:[[2,'desc']]})));\n"
    . "    \$('.at-acc-head').on('click', function(){\n"
    . "      var acc = \$(this).closest('.at-acc');\n"
    . "      var box = \$('#' + \$(this).attr('data-target'));\n"
    . "      acc.toggleClass('is-open');\n"
    . "      box.slideToggle(140, function(){ for (var i=0;i<dts.length;i++){ try{ dts[i].columns.adjust(); }catch(e){} } });\n"
    . "    });\n"
    . "  });\n"
    . "})(jQuery);\n";
  drupal_add_js($js, array('type' => 'inline', 'scope' => 'footer'));

  return $out;
}

/**
 * A role chip: Conducting (filled) vs Assisting (sand outline).
 */
function _at_role_badge($role) {
  $r = trim($role);
  $cls = (strcasecmp($r, 'Assisting') === 0) ? 'at-role-assisting' : 'at-role-conducting';
  return '<span class="at-role ' . $cls . '">' . check_plain($r) . '</span>';
}

/**
 * The course-identity card shown atop the hub and the applicant list.
 */
function _at_course_card($c) {
  return '<div class="at-course-card">'
    . '<span class="at-cc-centre">' . check_plain($c['centre_name']) . '</span>'
    . '<span class="at-cc-type">' . check_plain($c['course_type']) . '</span>'
    . '<span class="at-cc-dates">' . check_plain($c['start_date']) . ' – ' . check_plain($c['end_date']) . '</span>'
    . '<span class="at-cc-tags">' . _at_role_badge($c['my_role']) . '<span class="at-pill">' . t('View only') . '</span></span>'
    . '</div>';
}

/**
 * One collapsible section wrapping a courses DataTable (or an empty note).
 */
function _at_courses_accordion($id, $title, $rows, $open) {
  $caret = '<svg class="at-caret" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 6l4 4 4-4"/></svg>';
  $h = '<div class="at-acc' . ($open ? ' is-open' : '') . '">'
    . '<button type="button" class="at-acc-head" data-target="' . $id . '">' . $caret
    . '<span>' . check_plain($title) . '</span>'
    . '<span class="at-count">' . count($rows) . '</span></button>'
    . '<div class="at-acc-body" id="' . $id . '"' . ($open ? '' : ' style="display:none"') . '>';
  if (empty($rows)) {
    $h .= '<div class="at-empty">' . t('None.') . '</div>';
  }
  else {
    $h .= '<div class="at-tablewrap"><table id="' . $id . '-tbl" class="at-table display" style="width:100%">'
      . '<thead><tr><th>' . t('Centre') . '</th><th>' . t('Course type') . '</th><th>' . t('Start') . '</th><th>' . t('End') . '</th><th>' . t('My role') . '</th><th></th></tr></thead><tbody>';
    foreach ($rows as $r) {
      $h .= '<tr>'
        . '<td>' . check_plain($r->centre_name) . '</td>'
        . '<td>' . check_plain($r->course_type) . '</td>'
        . '<td class="at-col-date">' . check_plain($r->start_date) . '</td>'
        . '<td class="at-col-date">' . check_plain($r->end_date) . '</td>'
        . '<td>' . _at_role_badge($r->my_role) . '</td>'
        . '<td>' . l(t('View →'), 'at-portal/my-course/' . $r->course, array('attributes' => array('class' => array('at-btn')))) . '</td>'
        . '</tr>';
    }
    $h .= '</tbody></table></div>';
  }
  $h .= '</div></div>';
  return $h;
}


/**
 * Page: read-only hub for a single course — an identity card + a grid of tiles
 * linking to each read-only view.
 */
function at_view_course_hub($c) {
  drupal_set_title('');
  $cid = $c['course'];
  drupal_add_css(drupal_get_path('module', 'dh_atportal') . '/css/at-view.css');

  // Simple monochrome line icons (inherit currentColor).
  $sv = 'viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"';
  $ic = array(
    'users'   => '<svg ' . $sv . '><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>',
    'grid'    => '<svg ' . $sv . '><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/></svg>',
    'teacher' => '<svg ' . $sv . '><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>',
    'cell'    => '<svg ' . $sv . '><path d="M3 21h18"/><path d="M5 21V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2v16"/><path d="M15 12h.01"/></svg>',
    'report'  => '<svg ' . $sv . '><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/><path d="M8 13h8M8 17h8M8 9h2"/></svg>',
  );
  $tiles = array(
    array('applicants', 'users',   t('Applicants'),     t('all applicants')),
    array('seating',    'grid',    t('Seating Plan'),   ''),
    array('teachers',   'teacher', t('Teacher List'),   ''),
    array('cells',      'cell',    t('Cell List'),      ''),
    array('report',     'report',  t('Course Summary'), t('Day-11 report')),
  );

  $out  = '<div class="at-view">';
  $out .= _at_course_card($c);
  $out .= '<div style="margin:-6px 0 14px"><a class="at-back" href="' . url('at-portal/my-courses') . '">' . t('← Back to My Courses') . '</a></div>';
  $out .= '<div class="at-tiles">';
  foreach ($tiles as $t) {
    $sub = $t[3] ? '<span class="at-tile-sub">' . check_plain($t[3]) . '</span>' : '';
    $out .= '<a class="at-tile" href="' . url('at-portal/my-course/' . $cid . '/' . $t[0]) . '">'
      . '<span class="at-tile-ico">' . $ic[$t[1]] . '</span>'
      . '<span class="at-tile-label">' . check_plain($t[2]) . $sub . '</span></a>';
  }
  $out .= '</div>';
  $out .= '<a class="at-back" href="' . url('at-portal/my-courses') . '">' . t('← Back to My Courses') . '</a>';
  $out .= '</div>';
  return $out;
}


/**
 * Page: applicants for a course — reuses dh_manageapp's search-results page
 * (the same table the registrar/view-only role uses). Rendered directly with a
 * course-scoped WHERE, so it shows ALL applicants (both genders). Edit actions
 * (change status, transfer, delete, export) stay hidden because the AT lacks
 * those permissions; the row's "(View)" detail link, the expandable child-row
 * tabs and "Add Review" work via app_id_load's AT-course fallback. Enabling
 * more options later is just a matter of granting the AT the relevant perm.
 */
function at_view_course_applicants($c) {
  if (!function_exists('dh_manageapp_search_results')) {
    return t('Applicant list is unavailable.');
  }
  global $user;
  drupal_set_title('');
  $cid = $c['course'];
  drupal_add_css(drupal_get_path('module', 'dh_atportal') . '/css/at-view.css');

  // Export filename mirrors the main app's convention.
  $fname = preg_replace('/\s+/', '-', $c['centre_name'] . '_' . $c['course_type'] . '_' . $c['start_date'] . '_to_' . $c['end_date']);

  $out  = '<div class="at-view">';
  $out .= _at_course_card($c);
  $out .= '<div style="margin:-8px 0 14px"><a class="at-back" href="' . url('at-portal/my-course/' . $cid) . '">' . t('← Back to course') . '</a>'
        . ' &nbsp;·&nbsp; <a class="at-back" href="' . url('at-portal/my-courses') . '">' . t('All my courses') . '</a></div>';
  $out .= '</div>';
  // The application list IS the registrar's search-results table, rendered
  // read-only (last arg) so no write actions show even for ATs who also hold
  // edit permissions via another role — only "Add Review" remains. Rendered
  // OUTSIDE .at-view so none of the AT-view styling touches it.
  $out .= dh_manageapp_search_results('applicant', "a_course=$cid", $c['center'], FALSE, $fname, TRUE, $user->uid);
  return $out;
}


/**
 * The reused day-zero renderers (teacher/cell/seating list) print their own
 * toolbar links back to the REGISTRAR pages — "← Course page" → /course/{c}/{co},
 * "Back to Dashboard" → /centre/{c}, and the sort toggles → /teacher-list/... —
 * none of which an AT can open. The renderers emit a full page and exit(), so we
 * can't post-process their HTML; instead print a small script first that, once
 * the DOM is ready, rewrites any such link for THIS course to its AT-portal
 * equivalent. Print / column-toggle controls (href="#") are left untouched.
 */
function _at_view_fix_links($centre, $course) {
  $c = (int) $centre;
  $co = (int) $course;
  print '<script>(function(){var C=' . $c . ',CO=' . $co . ';'
    . 'var map={"teacher-list":"teachers","cell-list":"cells","seating2":"seating","seating":"seating","group-seating":"seating","manager-list":""};'
    . 'function u(s){return "/at-portal/my-course/"+CO+(s?"/"+s:"");}'
    . 'function rw(){var a=document.getElementsByTagName("a");for(var i=0;i<a.length;i++){var h=a[i].getAttribute("href")||"";'
    . 'var m=h.match(/\\/course\\/(\\d+)\\/(\\d+)/);if(m&&+m[2]===CO){a[i].setAttribute("href",u(""));continue;}'
    . 'var mc=h.match(/\\/centre\\/(\\d+)(?:[?#]|$)/);if(mc&&+mc[1]===C){a[i].setAttribute("href",u(""));continue;}'
    . 'var ml=h.match(/\\/(teacher-list|cell-list|seating2|seating|group-seating|manager-list)\\/(\\d+)\\/(\\d+)(\\?[^#]*)?/);'
    . 'if(ml&&+ml[3]===CO){a[i].setAttribute("href",u(map[ml[1]])+(ml[4]||""));continue;}}}'
    . 'if(document.readyState!=="loading")rw();else document.addEventListener("DOMContentLoaded",rw);})();</script>';
}


/**
 * Page: read-only Seating Plan. Reuses dh_manageapp's renderer directly with
 * the AT's centre/course.
 *
 * IMPORTANT: we do NOT call zeroize_new_course_data() (which DELETEs
 * dh_applicant_course rows for new students) and we never pass the ?r=1
 * regenerate/allocate branch — this is a pure read of the current saved plan,
 * so the teacher always sees the registrar's latest state.
 */
function at_view_course_seating($c) {
  if (!function_exists('dh_generate_seating_plan')) {
    return t('Seating plan is unavailable.');
  }
  $course = $c['course'];
  // Mirror the main app's combined-seating pairing (read-only).
  $other = db_query("select c_combined_seat_course from dh_course where c_id=:c", array(':c' => $course))->fetchField();
  $arg = ($other > 0) ? array($course, $other) : $course;

  // Read-only: strip the registrar's editing chrome. Every interactive control
  // (add/remove/blank column, the Save button, drag handles and hints) carries
  // the `no-print` class, so hiding it yields the static print-style plan. The
  // renderer emits a full page and exit()s, so this style is printed first.
  print '<style>.no-print{display:none !important;}'
      . '.at-seat-bar{font:13px/1.4 -apple-system,Segoe UI,Roboto,sans-serif;margin:8px 10px;padding:8px 12px;'
      . 'background:#e8f1f9;border:1px solid #d8e3ee;border-radius:8px;display:flex;gap:12px;align-items:center}'
      . '.at-seat-bar a{color:#23527c;font-weight:600;text-decoration:none}.at-seat-bar a:hover{text-decoration:underline}'
      . '.at-seat-bar .vo{margin-left:auto;color:#5a6675;font-weight:600;text-transform:uppercase;font-size:12px;letter-spacing:.03em}'
      . '@media print{.at-seat-bar{display:none}}</style>';
  print '<div class="at-seat-bar">'
      . l(t('← Back to course'), 'at-portal/my-course/' . $course)
      . '<span class="vo">' . t('View only') . '</span></div>';

  _at_view_fix_links($c['center'], $course);
  return dh_generate_seating_plan($c['center'], $arg, '', TRUE);
}


/**
 * Page: read-only Teacher List. Reuses dh_manageapp's renderer. No zeroize,
 * no writes.
 */
function at_view_course_teachers($c) {
  if (!function_exists('dh_generate_teacher_list')) {
    return t('Teacher list is unavailable.');
  }
  $seating = (isset($_REQUEST['seating']) && $_REQUEST['seating'] == 1) ? 1 : 0;
  _at_view_fix_links($c['center'], $c['course']);
  return dh_generate_teacher_list($c['center'], $c['course'], $seating);
}


/**
 * Page: read-only Cell List. Reuses dh_manageapp's renderer. No zeroize, and
 * never the ?r=1 (generate_cell_list) write branch.
 */
function at_view_course_cells($c) {
  if (!function_exists('dh_cell_list')) {
    return t('Cell list is unavailable.');
  }
  _at_view_fix_links($c['center'], $c['course']);
  return dh_cell_list($c['center'], $c['course'], 0);
}


/**
 * Page: read-only full applicant detail. Reuses dh_manageapp's pure-read
 * renderer (Print button only; no edit controls).
 */
function at_view_applicant_detail($app_id) {
  global $user;
  $back = '<p>' . l(t('Back'), 'javascript:history.back()', array('external' => TRUE)) . '</p>';
  if (!function_exists('dh_application_view')) {
    return t('Applicant view is unavailable.');
  }
  // Pass the AT's uid so the tag widget shows the AT's private tags + system
  // tags (not centre tags).
  return $back . dh_application_view($app_id, $user->uid) . $back;
}


/**
 * Page: Course Summary (Day-11) report PDF. Reuses the dh_manageapp worker.
 */
function at_view_course_report($c) {
  if (!function_exists('create_d11_report_pdf')) {
    return t('Report is unavailable.');
  }
  $pdf = create_d11_report_pdf($c['course']);
  header("Content-type: application/pdf");
  header("Content-Disposition: inline; filename=Day11-Report.pdf");
  $ret = s3_get_file(variable_get('s3_bucket_name', ''), $pdf, 'stream');
  if (!$ret['success']) {
    watchdog('S3PDF', $ret['message']);
  }
  print($ret['data']);
  exit();
}
