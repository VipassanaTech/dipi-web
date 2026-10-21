<?php
/**
 * @file
 * The Dipi App download page (/dipi-app) and the APK download (/dipi-app/download),
 * for the people who can use the app (dcv_api_access: centre team with 'access zero day' and
 * at least one of the Male / Female permissions, teachers with 'dcv teacher access').
 *
 * The APK is not in git: it sits in the private files folder, private://dipi-app/.
 * A new release = copy its APK there and change dcv_app_release().
 */

/** The release offered for download. */
function dcv_app_release() {
  return array(
    'version' => '1.1.6',
    'date'    => '2026-10-12',
    'file'    => 'DipiApp-1.1.6.apk',
    'sha256'  => '84124b8f87a2efbfa19553897aadb8d8d352329b1cc328a9aed66de9d1c7eb50',
  );
}

/**
 * The APK's uri, or FALSE when it is missing or is not the file named in
 * dcv_app_release() (checked by its sha256, so a wrong upload is never offered).
 */
function dcv_app_file() {
  $rel = dcv_app_release();
  $uri = 'private://dipi-app/' . $rel['file'];
  if (!is_file($uri) || hash_file('sha256', $uri) !== $rel['sha256']) {
    return FALSE;
  }
  return $uri;
}

/** Page callback: /dipi-app. */
function dcv_app_page() {
  drupal_set_title(t('Dipi App'));
  drupal_add_css(drupal_get_path('module', 'dh_courseviewer') . '/css/dipi-app.css');
  $rel = dcv_app_release();
  $uri = dcv_app_file();

  $out  = '<div class="dipi-app">';
  $out .= '<p class="dipi-app-lead">' . t('The Dipi App shows the seating plan and the students of a running course on an Android tablet or phone. After the course is loaded, it works without internet.') . '</p>';

  $out .= '<div class="dipi-app-box">';
  $out .= '<div class="dipi-app-name">' . t('Dipi App @v', array('@v' => $rel['version'])) . '</div>';
  if ($uri) {
    $out .= '<div class="dipi-app-meta">' . t('Released @d', array('@d' => date('j F Y', strtotime($rel['date'])))) . ' &middot; ' . format_size(filesize($uri)) . '</div>';
  }
  $out .= '<div class="dipi-app-meta">' . t('For Android 6.0 or newer. Not for iPhone or iPad.') . '</div>';
  if ($uri) {
    $out .= l(t('Download Dipi App'), 'dipi-app/download', array('attributes' => array('class' => array('dipi-app-button'))));
  }
  else {
    $out .= '<p class="dipi-app-missing">' . t('The download is not available just now. Please try again later.') . '</p>';
  }
  $out .= '</div>';

  $out .= '<h3>' . t('Install') . '</h3><ol>'
    . '<li>' . t('Open this page on the Android tablet or phone and tap <b>Download Dipi App</b>.') . '</li>'
    . '<li>' . t('When the download is done, tap the file <b>@f</b> to open it.', array('@f' => $rel['file'])) . '</li>'
    . '<li>' . t('If Android asks, allow your browser to install apps (<b>Install unknown apps</b>). Then go back and tap <b>Install</b>.') . '</li>'
    . '<li>' . t('If a <b>Play Protect</b> message comes, choose to install anyway (you may need to tap <b>More details</b> first).') . '</li>'
    . '<li>' . t('Open <b>Dipi App</b> and log in with your Dipi username and password.') . '</li>'
    . '</ol>';

  $out .= '<h3>' . t('Update') . '</h3>'
    . '<p>' . t('When a new version comes, download it from this page and install it in the same way. The course on the tablet stays.') . '</p>';

  // Help: the centre wiki for the centre team, the AT Portal wiki for teachers.
  $help = array();
  if (user_access('access zero day')) {
    $help[] = l(t('Dipi App help for centres'), 'wiki-dipi-app');
  }
  if (user_access('dcv teacher access')) {
    $help[] = l(t('Dipi App help for teachers'), 'at-wiki-dipi-app');
  }
  if ($help) {
    $out .= '<h3>' . t('How to use the app') . '</h3><p>' . implode(' &nbsp;|&nbsp; ', $help) . '</p>';
  }

  $out .= '</div>';
  return $out;
}

/** Page callback: /dipi-app/download — sends the APK. */
function dcv_app_download() {
  $uri = dcv_app_file();
  if (!$uri) {
    drupal_set_message(t('The download is not available just now. Please try again later.'), 'error');
    drupal_goto('dipi-app');
  }
  $rel = dcv_app_release();
  watchdog('dipi_app', 'Downloaded Dipi App @v', array('@v' => $rel['version']), WATCHDOG_INFO);
  file_transfer($uri, array(
    'Content-Type'        => 'application/vnd.android.package-archive',
    'Content-Disposition' => 'attachment; filename="' . $rel['file'] . '"',
    'Content-Length'      => filesize($uri),
    'Cache-Control'       => 'private, no-cache',
  ));
}
