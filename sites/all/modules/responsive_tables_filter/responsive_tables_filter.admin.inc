<?php

/**
 * @file
 * Make tables responsive, when filter is enabled for the field.
 */

/**
 * Form callback to control module settings.
 *
 * Allows on/off for responsiveness on Drupal Views.
 */
function responsive_tables_filter_admin_form($form, &$form_state) {
  $form['introductory_text'] = array(
    '#markup' => '<p>' . t('Enable responsive tables in WYSIWYG fields by going to !text_formats and checking "Make tables responsive" for the desired text format(s).', array('!text_formats' => l(t('Text Formats'), '/admin/config/content/formats'))) . '</p><hr />',
  );
  $form[RESPONSIVE_TABLES_FILTER_EVERY_PAGE] = array(
    '#type' => 'checkbox',
    '#title' => t('Load CSS and Javascript on every page'),
    '#description' => t('This is useful if you use Tablesaw in content other than nodes or Views tables (e.g., blocks or Panel displays). However this unconditionally increases the asset weight of page loads, which may be a performance drawback for sites that do not aggregate CSS/JS.'),
    '#default_value' => variable_get(RESPONSIVE_TABLES_FILTER_EVERY_PAGE, FALSE),
  );
  $form[RESPONSIVE_TABLES_FILTER_CUSTOM_CSS] = array(
    '#type' => 'textfield',
    '#title' => t('Path to custom CSS'),
    '#description' => t('It may be necessary to override the module-provided CSS if, for example, your theme uses different media breakpoints. Copy tablesaw/css/tablesaw.stackonly.css from this module and modify as necessary, then provide the path, relative to the Drupal root directory, of the new file. This allows you to continue to update this module without maintaining a patched version.'),
    '#default_value' => variable_get(RESPONSIVE_TABLES_FILTER_CUSTOM_CSS, ''),
  );
  $form[RESPONSIVE_TABLES_FILTER_FILES] = array(
    '#type' => 'checkbox',
    '#title' => t('Apply to file table display'),
    '#description' => t('This only applies when you have set a file field display format to "Table of files".'),
    '#default_value' => variable_get(RESPONSIVE_TABLES_FILTER_FILES, TRUE),
  );
  $form[RESPONSIVE_TABLES_FILTER_VIEWS] = array(
    '#type' => 'checkbox',
    '#title' => t('Apply to Views'),
    '#description' => t('When checked, Views table displays will be responsive.'),
    '#default_value' => variable_get(RESPONSIVE_TABLES_FILTER_VIEWS, TRUE),
  );
  $form[RESPONSIVE_TABLES_FILTER_ADMIN] = array(
    '#type' => 'checkbox',
    '#title' => t('Apply to Views in administration theme'),
    '#description' => t('When checked, responsive components will load for the administration theme.'),
    '#default_value' => variable_get(RESPONSIVE_TABLES_FILTER_ADMIN, FALSE),
    '#states' => array(
      'disabled' => array(
        ':input[name="' . RESPONSIVE_TABLES_FILTER_VIEWS . '"]' => array('checked' => FALSE),
      ),
    ),
  );
  $form['#validate'] = ['responsive_tables_filter_config_form_validate'];
  return system_settings_form($form);
}

/**
 * Custom form validation.
 */
function responsive_tables_filter_config_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  if (!empty($values[RESPONSIVE_TABLES_FILTER_CUSTOM_CSS])) {
    $path = DRUPAL_ROOT . '/' . $values[RESPONSIVE_TABLES_FILTER_CUSTOM_CSS];
    if (!file_exists($path)) {
      form_set_error(RESPONSIVE_TABLES_FILTER_CUSTOM_CSS, t('No file exists at %path', array('%path' => $path)));
    }
  }
}
