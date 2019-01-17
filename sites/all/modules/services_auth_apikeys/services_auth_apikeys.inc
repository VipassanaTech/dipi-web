<?php
/**
 * @file
 * Include file for the services_auth_apikeys module.
 */

/**
 * Access callback for Services authentication.
 *
 * @see hook_services_authentication_info()
 */
function _services_auth_apikeys_authenticate_call($settings, $method, $args) {
  $endpoint = array();
  // Grep the specific settings for this method
  if (!empty($method['endpoint']['services_auth_apikeys'])) {
    $endpoint += array_filter($method['endpoint']['services_auth_apikeys']);
  }
  // If the authentication is disabled for this method, do nothing.
  if (!empty($endpoint['disable_authentication'])) {
    return;
  }

  // Get the settings for this authentication.
  $authentication_settings = _services_auth_apikeys_get_authentication_settings($settings);

  $apikeys['api_key'] = $authentication_settings['api_key']['identifier'];
  $apikeys['token'] = $authentication_settings['token']['identifier'];
  if (!empty($authentication_settings['extra_key']) && $authentication_settings['extra_key']['enabled']) {
    $apikeys['extra_key'] = $authentication_settings['extra_key']['identifier'];
  }
  // Get the API keys sent by services request by they parameter type.
  $missing_apikeys = array();
  foreach ($apikeys as $apikey => $id) {
    switch ($authentication_settings[$apikey]['parameter_type']) {
      case 'get':
        $params = $_GET;
        $param_key = $id;
        break;
      case 'post':
        $params = $_GET;
        $param_key = $id;
        break;
      case 'http_header':
      default:
        $params = $_SERVER;
        $param_key = 'HTTP_' . str_replace('-', '_', $id);
        break;
    }
    if (empty($params[$param_key])) {
      $missing_apikeys[$apikey] = $id;
    }
    else {
      $apikeys_values[$apikey] = $params[$param_key];
    }
  }
  // Return messages for missing API keys.
  if (!empty($missing_apikeys)) {
    return t('The service requires data for @apikeys.', array('@apikeys' => implode(', ', $missing_apikeys)));
  }
  // Get the user id for the API keys sent.
  $service_endpoint = services_get_server_info('endpoint');
  $uid = services_auth_apikeys_load_by_apikeys($apikeys_values, $service_endpoint, TRUE);
  if (!$uid) {
    // Invalid API keys.
    return t('The API keys sent are invalid.');
  }
  else {
    $user = user_load($uid);
    // Check if the user exist.
    if (!$user) {
      return t('The user does not exist anymore.');
    }
    // Check if the user has access to the API keys authentication.
    if (!user_access('services use api keys authentication', $user)) {
      return t('Access denied.') . ' ' . t('The user with these credentials does not have permissions to the services.');
    }
    // Check the access to API keys authentication for this use if it is set.
    if (!empty($endpoint['access'])) {
      $deny_access = FALSE;
      foreach ($user->roles as $role) {
        if (!empty($endpoint['access'][$role . ' deny access'])) {
          $deny_access = TRUE;
        }
      }
      if ($deny_access) {
        return t('Access denied.') . ' ' . t('The user with these credentials does not have permissions to the services.');
      }
    }
    // Login for the API keys owner.
    if ($authentication_settings['user_login'] || !empty($endpoint['user_login'])) {
      $GLOBALS['user'] = $user;
    }
  }
}

/**
 * Default settings callback for Services authentication.
 *
 * @see hook_services_authentication_info()
 */
function _services_auth_apikeys_default_security_settings() {
  return array(
    'api_key' => array(
      'identifier' => 'API-KEY',
      'generate' => FALSE,
      'parameter_type' => 'http_header',
    ),
    'token' => array(
      'identifier' => 'TOKEN',
      'generate' => TRUE,
      'parameter_type' => 'http_header',
    ),
    'extra_key' => array(
      'enabled' => FALSE,
      'identifier' => 'EXTRA-KEY',
      'generate' => FALSE,
      'parameter_type' => 'http_header',
    ),
    'user_login' => FALSE,
  );
}

/**
 * Settings callback for Services authentication.
 *
 * @see hook_services_authentication_info()
 */
function _services_auth_apikeys_security_settings($settings, &$form_state) {
  $form = array();

  // API keys settings.
  $form['api_key'] = array(
    '#type' => 'fieldset',
    '#title' => t('API Key'),
  );
  $parameter_types = array(
    'http_header' => t('HTTP Header'),
    'get' => t('GET value'),
    'post' => t('POST value'),
  );
  $form['api_key']['parameter_type'] = array(
    '#type' => 'select',
    '#title' => t('Parameter type'),
    '#description' => t('How this API key should be sent in the request.'),
    '#options' => $parameter_types,
    '#default_value' => $settings['api_key']['parameter_type'],
  );
  $form['api_key']['identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Identifier'),
    '#description' => t('What identifier do you want to use in the Service API for this key.'),
    '#default_value' => $settings['api_key']['identifier'],
    '#required' => TRUE,
  );
  $form['api_key']['generate'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do you want this key to be generated by user?'),
    '#default_value' => $settings['api_key']['generate'],
  );

  $form['token'] = array(
    '#type' => 'fieldset',
    '#title' => t('Token'),
  );
  $form['token']['parameter_type'] = array(
    '#type' => 'select',
    '#title' => t('Parameter type'),
    '#description' => t('How this API key should be sent.'),
    '#options' => $parameter_types,
    '#default_value' => $settings['token']['parameter_type'],
  );
  $form['token']['identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Identifier'),
    '#description' => t('What identifier do you want to use in the Service API for this key.'),
    '#default_value' => $settings['token']['identifier'],
    '#required' => TRUE,
  );
  $form['token']['generate'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do you want this key to be generated by user?'),
    '#default_value' => $settings['token']['generate'],
  );

  $form['extra_key'] = array(
    '#type' => 'fieldset',
    '#title' => t('Extra Key'),
  );
  $form['extra_key']['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do you need an extra key for authentication?'),
    '#default_value' => $settings['extra_key']['enabled'],
  );
  $form['extra_key']['parameter_type'] = array(
    '#type' => 'select',
    '#title' => t('Parameter type'),
    '#description' => t('How this API key should be sent.'),
    '#options' => $parameter_types,
    '#default_value' => $settings['extra_key']['parameter_type'],
    '#states' => array(
      'visible' => array(
        '#edit-services-auth-apikeys-extra-key-enabled' => array('checked' => TRUE),
      ),
    ),
  );
  $form['extra_key']['identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Identifier'),
    '#description' => t('What identifier do you want to use in the Service API for this key.'),
    '#default_value' => $settings['extra_key']['identifier'],
    '#states' => array(
      'visible' => array(
        '#edit-services-auth-apikeys-extra-key-enabled' => array('checked' => TRUE),
      ),
    ),
  );
  $form['extra_key']['generate'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do you want this key to be generated by user?'),
    '#default_value' => $settings['extra_key']['generate'],
    '#states' => array(
      'visible' => array(
        '#edit-services-auth-apikeys-extra-key-enabled' => array('checked' => TRUE),
      ),
    ),
  );

  $form['user_login'] = array(
    '#type' => 'checkbox',
    '#title' => t('Do you want the API keys owner to login?'),
    '#default_value' => $settings['user_login'],
  );

  return $form;
}

function _services_auth_apikeys_controller_settings($settings, $controller, $endpoint, $class, $name) {
  $form = array();
  $endpoint_settings = _services_auth_apikeys_get_authentication_settings('');
  // Disable API keys authentication.
  // If disable is set do not display the other controller settings.
  $disable_class_name = drupal_clean_css_identifier('services_auth_apikeys'. $controller['callback'] . '-' . $class . '-' . $name . '-disable-authentication');
  $form['disable_authentication'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable API keys authentication?'),
    '#default_value' => isset($settings['disable_authentication']) ? $settings['disable_authentication'] : FALSE,
    '#attributes' => array('class' => array($disable_class_name)),
  );
  if (empty($endpoint_settings['user_login'])) {
    $form['user_login'] = array(
      '#type' => 'checkbox',
      '#title' => t('Do you want the API keys owner to login?'),
      '#default_value' => isset($settings['user_login']) ? $settings['user_login'] : FALSE,
      '#states' => array(
        'invisible' => array(
          ':input.' . $disable_class_name => array('checked' => TRUE),
        ),
        'visible' => array(
          ':input.' . $disable_class_name => array('checked' => FALSE),
        ),
      ),
    );
  }

  // Access per user roles.
  $user_roles = user_roles(TRUE, 'services use api keys authentication');
  $form['access'] = array(
    '#type' => 'fieldset',
    '#title' => t('Access'),
    '#tree' => TRUE,
    '#states' => array(
      'invisible' => array(
        ':input.' . $disable_class_name => array('checked' => TRUE),
      ),
    ),
    'visible' => array(
      ':input.' . $disable_class_name => array('checked' => FALSE),
    ),
  );
  foreach ($user_roles as $key => $role) {
    $form['access'][$role . ' deny access'] = array(
      '#type' => 'checkbox',
      '#title' => t('Deny access for %role', array('%role' => $role)),
      '#default_value' => isset($settings['access'][$role . ' deny access']) ? $settings['access'][$role . ' deny access'] : 'enable',
    );
  }

  return $form;
}

/**
 * Form callback for the user API keys management form.
 */
function services_auth_apikeys_user_form($form, &$form_state, $account) {
  // Get the services endpoints that use this authentication.
  $all_services_endpoints = services_endpoint_load_all();
  foreach ($all_services_endpoints as $services_endpoint) {
    if (!empty($services_endpoint->authentication['services_auth_apikeys'])) {
      $services_endpoints[$services_endpoint->name] = $services_endpoint;
    }
  }
  // Get the current API keys if they exist.
  $existing_apikeys = services_auth_apikeys_load_by_user($account->uid);

  // Generate the form, a API keys fieldset for every service endpoint.
  $form = array();
  foreach ($services_endpoints as $services_endpoint_name => $services_endpoint) {
    // Get the authentication settings.
    $authentication_settings = _services_auth_apikeys_get_authentication_settings($services_endpoint->authentication['services_auth_apikeys']);
    $form[$services_endpoint_name] = array(
      '#type' => 'fieldset',
      '#title' => t('Service endpoint %service', array('%service' => $services_endpoint_name)),
      '#tree' => TRUE,
    );
    $form[$services_endpoint_name]['service_endpoint'] = array(
      '#type' => 'value',
      '#value' => $services_endpoint_name,
    );
    if (empty($existing_apikeys[$services_endpoint_name])) {
      // The generate button is always shown if no API Keys exist.
      $form[$services_endpoint_name]['generate_apikeys'] = array(
        '#type' => 'submit',
        '#name' => $services_endpoint_name . '-generate-apikeys',
        '#value' => t('Generate new API Keys'),
        '#submit' => array('_services_auth_apikeys_generate_user_apikeys_service_endpoint_submit'),
      );
    }
    else {
      // Get the API keys.
      $apikeys = $existing_apikeys[$services_endpoint_name];
      // Parameter types labels - helper variable.
      $parameter_types = array(
        'http_header' => t('HTTP Header'),
        'get' => t('GET value'),
        'post' => t('POST value'),
      );
      $form[$services_endpoint_name]['apikeys_age'] = array(
        '#type' => 'item',
        '#title' => t('API keys age'),
        '#markup' => format_interval(REQUEST_TIME - $apikeys->updated),
      );

      $form[$services_endpoint_name]['current_api_key'] = array(
        '#type' => 'item',
        '#title' => t('API Key (%identifier - @parameter_type)', array('%identifier' => $authentication_settings['api_key']['identifier'], '@parameter_type' => $parameter_types[$authentication_settings['api_key']['parameter_type']])),
        '#markup' => '<code>' . $apikeys->api_key . '</code>',
        '#states' => array(
          'enabled' => array(
            '#edit-' . $services_endpoint_name . '-action-check' => array('checked' => TRUE),
          ),
        ),
      );
      $form[$services_endpoint_name]['current_token'] = array(
        '#type' => 'item',
        '#title' => t('Token (%identifier - @parameter_type)', array('%identifier' => $authentication_settings['token']['identifier'], '@parameter_type' => $parameter_types[$authentication_settings['token']['parameter_type']])),
        '#markup' => '<code>' . $apikeys->token . '</code>',
      );
      if ($authentication_settings['extra_key']['enabled']) {
        $form[$services_endpoint_name]['current_extra_key'] = array(
          '#type' => 'item',
          '#title' => t('Extra key (%identifier - @parameter_type)', array('%identifier' => $authentication_settings['extra_key']['identifier'], '@parameter_type' => $parameter_types[$authentication_settings['extra_key']['parameter_type']])),
          '#markup' => '<code>' . $apikeys->extra_key . '</code>',
        );
      }

      // Generate / Delete API keys buttons.
      // Warning with checkbox to inform the user before doing risky actions.
      $form[$services_endpoint_name]['warning'] = array(
        '#type' => 'item',
        '#title' => t('Warning'),
        '#markup' => t('Generating, editing or removing API keys may results in issues elsewhere. Make sure that all external systems that uses these API Keys are updated as well, as soon as a new API keys are generated.'),
      );
      // The remove/generate buttons are only shown if the checkbox is checked.
      $form[$services_endpoint_name]['action_check'] = array(
        '#type' => 'checkbox',
        '#title' => t('Operate on existing API keys.'),
        '#description' => t('Check this box if you want to remove or regenerate API keys.'),
      );

      $limit_validation_errors = array(array('uid'), array($services_endpoint_name, 'service_endpoint'));
      // The generate button is shown if api key generate option is set.
      if ($authentication_settings['api_key']['generate']) {
        $form[$services_endpoint_name]['generate_api_key'] = array(
          '#type' => 'submit',
          '#name' => $services_endpoint_name . '-generate-api-key',
          '#api_key' => 'api_key',
          '#value' => t('Generate new API Key (@identifier)', array('@identifier' => $authentication_settings['api_key']['identifier'])),
          '#limit_validation_errors' => $limit_validation_errors,
          '#submit' => array('_services_auth_apikeys_generate_user_apikeys_service_endpoint_submit'),
          '#states' => array(
            'visible' => array(
              '#edit-' . $services_endpoint_name . '-action-check' => array('checked' => TRUE),
            ),
          ),
        );
      }

      // The generate button is shown if token generate option is set.
      if ($authentication_settings['token']['generate']) {
        $form[$services_endpoint_name]['generate_token'] = array(
          '#type' => 'submit',
          '#name' => $services_endpoint_name . '-generate-token',
          '#api_key' => 'token',
          '#value' => t('Generate new token (@identifier)', array('@identifier' => $authentication_settings['token']['identifier'])),
          '#limit_validation_errors' => $limit_validation_errors,
          '#submit' => array('_services_auth_apikeys_generate_user_apikeys_service_endpoint_submit'),
          '#states' => array(
            'visible' => array(
              '#edit-' . $services_endpoint_name . '-action-check' => array('checked' => TRUE),
            ),
          ),
        );
      }

      // The generate button is shown if extra key generate option is set.
      if ($authentication_settings['extra_key']['enabled'] && $authentication_settings['extra_key']['generate']) {
        $form[$services_endpoint_name]['generate_extra_key'] = array(
          '#type' => 'submit',
          '#name' => $services_endpoint_name . '-generate-extra-key',
          '#api_key' => 'extra_key',
          '#value' => t('Generate new extra key (@identifier)', array('@identifier' => $authentication_settings['extra_key']['identifier'])),
          '#limit_validation_errors' => $limit_validation_errors,
          '#submit' => array('_services_auth_apikeys_generate_user_apikeys_service_endpoint_submit'),
          '#states' => array(
            'visible' => array(
              '#edit-' . $services_endpoint_name . '-action-check' => array('checked' => TRUE),
            ),
          ),
        );
      }

      // Delete (all) API keys button.
      $form[$services_endpoint_name]['delete'] = array(
        '#type' => 'submit',
        '#name' => $services_endpoint_name . '-delete',
        '#value' => t('Delete API keys'),
        '#limit_validation_errors' => $limit_validation_errors,
        '#submit' => array('_services_auth_apikeys_delete_user_apikeys_service_endpoint_submit'),
        '#states' => array(
          'visible' => array(
            '#edit-' . $services_endpoint_name . '-action-check' => array('checked' => TRUE),
          ),
        ),
      );

      // Edit API keys manually.
      // The API keys edit form textfields are only shown if the checkbox is checked.
      $form[$services_endpoint_name]['edit_check'] = array(
        '#type' => 'checkbox',
        '#title' => t('Edit manually'),
        '#description' => t('Check this box if you want to edit manually API keys.'),
        '#states' => array(
          'visible' => array(
            '#edit-' . $services_endpoint_name . '-action-check' => array('checked' => TRUE),
          ),
        ),
      );
      $form[$services_endpoint_name]['edit'] = array(
        '#type' => 'fieldset',
        '#title' => t('Are you really sure you want to edit API keys manually?'),
        '#tree' => TRUE,
        '#states' => array(
          'visible' => array(
            '#edit-' . $services_endpoint_name . '-edit-check' => array('checked' => TRUE),
          ),
        ),
      );
      $form[$services_endpoint_name]['edit']['api_key'] = array(
        '#type' => 'textfield',
        '#title' => t('API Key (%identifier - @parameter_type)', array('%identifier' => $authentication_settings['api_key']['identifier'], '@parameter_type' => $parameter_types[$authentication_settings['api_key']['parameter_type']])),
        '#default_value' => $apikeys->api_key,
        '#required' => TRUE,
        '#element_validate' => array('_services_auth_apikeys_edit_apikey_validate'),
      );
      $form[$services_endpoint_name]['edit']['token'] = array(
        '#type' => 'textfield',
        '#title' => t('API Key (%identifier - @parameter_type)', array('%identifier' => $authentication_settings['token']['identifier'], '@parameter_type' => $parameter_types[$authentication_settings['token']['parameter_type']])),
        '#default_value' => $apikeys->token,
        '#required' => TRUE,
        '#element_validate' => array('_services_auth_apikeys_edit_apikey_validate'),
      );

      if ($authentication_settings['extra_key']['enabled']) {
        $form[$services_endpoint_name]['edit']['extra_key'] = array(
          '#type' => 'textfield',
          '#title' => t('API Key (%identifier - @parameter_type)', array('%identifier' => $authentication_settings['extra_key']['identifier'], '@parameter_type' => $parameter_types[$authentication_settings['extra_key']['parameter_type']])),
          '#default_value' => $apikeys->extra_key,
          '#required' => TRUE,
          '#element_validate' => array('_services_auth_apikeys_edit_apikey_validate'),
        );
      }

      // Save edited API keys button.
      $form[$services_endpoint_name]['edit']['save'] = array(
        '#type' => 'submit',
        '#name' => $services_endpoint_name . '-edit',
        '#value' => t('Save API keys'),
        '#submit' => array('_services_auth_apikeys_save_user_apikeys_service_endpoint_submit'),
      );
    }
  }

  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $account->uid,
  );

  return $form;
}

/**
 * Submit callback for generating the API Keys.
 */
function _services_auth_apikeys_generate_user_apikeys_service_endpoint_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $values = drupal_array_get_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -1));
  // Generate one api key.
  if (!empty($button['#api_key'])) {
    services_auth_apikeys_update_apikeys($form_state['values']['uid'], $values['service_endpoint'], array($button['#api_key']));
    $services_endpoint = services_endpoint_load($values['service_endpoint']);
    $authentication_settings = _services_auth_apikeys_get_authentication_settings($services_endpoint->authentication['services_auth_apikeys']);
    drupal_set_message(t('The %api_key API key for %service_endpoint service endpoint has been generated.', array('%api_key' => $authentication_settings[$button['#api_key']]['identifier'] . ' (' . $button['#api_key'] . ')', '%service_endpoint' => $values['service_endpoint'])));
  }
  // Generate all API keys.
  else {
    services_auth_apikeys_update_apikeys($form_state['values']['uid'], $values['service_endpoint']);
    drupal_set_message(t('The API keys for %service_endpoint service endpoint has been generated.', array('%service_endpoint' => $values['service_endpoint'])));
  }
}

/**
 * Submit callback for API keys deletion.
 */
function _services_auth_apikeys_delete_user_apikeys_service_endpoint_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $values = drupal_array_get_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -1));
  services_auth_apikeys_delete_apikeys($form_state['values']['uid'], $values['service_endpoint']);
  drupal_set_message(t('The API keys for %service_endpoint service endpoint has been deleted.', array('%service_endpoint' => $values['service_endpoint'])));
}

/*
 * API keys edit elements validation.
 */
function _services_auth_apikeys_edit_apikey_validate($element, &$form_state, $form) {
  if (!preg_match('/^[a-zA-Z0-9-_]{43}$/i', $element['#value'])) {
    form_error($element, t('The !element is not in the right format: 43 characters length, only letters, numbers, underscores and hyphens characters are allowed.', array('!element' => $element['#title'])));
  }
}

/**
 * Submit callback for updating manually the API Keys.
 */
function _services_auth_apikeys_save_user_apikeys_service_endpoint_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $values = drupal_array_get_nested_value($form_state['values'], array_slice($button['#array_parents'], 0, -2));
  // Build the API keys values from submitted values.
  $apikeys = array(
    'api_key' => $values['edit']['api_key'],
    'token' => $values['edit']['token'],
  );
  if (!empty($values['edit']['extra_key'])) {
    $apikeys['extra_key'] = $values['edit']['extra_key'];
  }
  // Update edited API keys.
  services_auth_apikeys_update_apikeys($form_state['values']['uid'], $values['service_endpoint'], array_keys($apikeys), $apikeys);
  drupal_set_message(t('The API keys for %service_endpoint service endpoint has been updated.', array('%service_endpoint' => $values['service_endpoint'])));
}

