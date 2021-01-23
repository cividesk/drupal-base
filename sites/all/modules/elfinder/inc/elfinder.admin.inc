<?php

/**
 * @file
 * file manager admin settings page
 */


/**
 * Settings form callback
 */
function elfinder_admin() {

  $obj = drupal_get_form('elfinder_d7_admin_form');
  return drupal_render($obj);
}

/**
 * Settings form definition for D7
 */
function elfinder_d7_admin_form($form_id, &$form_state) {
  return elfinder_admin_form($form_state);
}

/**
 * Settings form definition
 */
function elfinder_admin_form(&$form_state) {
  global $user, $language;

  $form['filesystem_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('File system settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['filesystem_settings']['filesystem_root'] = array(
    '#type' => 'radios',
    '#title' => t('Root directory'),
    '#default_value' => variable_get('elfinder_settings_filesystem_root', 'system'),
    '#options' => array(
      'system' => t('Use system defaults (@systemroot)', array('@systemroot' => elfinder_default_directory_path())),
      'custom' => t('Specify custom path'),
    ),
  );

  $form['filesystem_settings']['filesystem_root_custom'] = array(
    '#prefix' => '<div class="custom-container">',
    '#suffix' => '</div>',
    '#type' => 'textfield',
    '#title' => t('Custom path'),
    '#default_value' => variable_get('elfinder_settings_filesystem_root_custom', ''),
    '#description' => t('Custom filesystem root path.') . '<br/>' . t('Available tokens: <code>%files</code> (base path, eg: <code>/</code>), <code>%name</code> (current username, eg: <code>@u</code>, <b>NOTE:</b> it is not unique - users can have same username, so better to combine it with user id value), <code>%uid</code> (current user id, eg: <code>@uid</code>), <code>%lang</code> (current language code, eg: <code>@lang</code>), plus all tokens provided by token module', array('@u' => $user->name, '@uid' => $user->uid, '@lang' => $language->language)),
  );

  $form['filesystem_settings']['filesystem_url_custom'] = array(
    '#prefix' => '<div class="custom-container">',
    '#suffix' => '</div>',
    '#type' => 'textfield',
    '#title' => t('Custom URL'),
    '#default_value' => variable_get('elfinder_settings_filesystem_url_custom', ''),
    '#description' => t('Custom filesystem url.') . '<br/>' . t('Available tokens: <code>%files</code> (base path, eg: <code>/</code>), <code>%name</code> (current username, eg: <code>@u</code>, <b>NOTE:</b> it is not unique - users can have same username, so better to combine it with user id value), <code>%uid</code> (current user id, eg: <code>@uid</code>), <code>%lang</code> (current language code, eg: <code>@lang</code>), plus all tokens provided by token module', array('@u' => $user->name, '@uid' => $user->uid, '@lang' => $language->language)),
  );

  $form['filesystem_settings']['mime_detect'] = array(
    '#type' => 'radios',
    '#title' => t('File type detection'),
    '#default_value' => variable_get('elfinder_settings_filesystem_mimedetect', 'auto'),
    '#options' => array(
      'auto' => t('Automatical detection'),
    ),
  );

  if (function_exists('finfo_open')) {
    $form['filesystem_settings']['mime_detect']['#options']['finfo'] = t('php finfo');
  }

  if (function_exists('mime_content_type')) {
    $form['filesystem_settings']['mime_detect']['#options']['php'] = t('php mime_content_type()');
  }

  $form['filesystem_settings']['mime_detect']['#options']['linux'] = t('file -ib (linux)');
  $form['filesystem_settings']['mime_detect']['#options']['bsd'] = t('file -Ib (bsd)');
  $form['filesystem_settings']['mime_detect']['#options']['internal'] = t('By file extension (built-in)');
  $form['filesystem_settings']['mime_detect']['#options']['drupal'] = t('Drupal API');
  
  
  $form['filesystem_settings']['file_url_type'] = array(
    '#type' => 'radios',
    '#title' => t('Selected file url type'),
    '#default_value' => variable_get('elfinder_settings_filesystem_fileurl', 'true'),
    '#options' => array(
      'true' => t('Absolute'),
      'false' => t('Relative'),
    ),
  );  

  $form['filesystem_settings']['file_perm'] = array(
    '#type' => 'textfield',
    '#title' => t('Created file permissions'),
    '#default_value' => variable_get('elfinder_settings_filesystem_fileperm', '0666'),
    '#size' => 4,
  );

  $form['filesystem_settings']['dir_perm'] = array(
    '#type' => 'textfield',
    '#title' => t('Created directory permissions'),
    '#default_value' => variable_get('elfinder_settings_filesystem_dirperm', '0777'),
    '#size' => 4,
  );

  $form['thumbnail_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Image settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  
  $form['thumbnail_settings']['tmbsize'] = array(
    '#type' => 'textfield',
    '#title' => t('Thumbnail size'),
    '#default_value' => variable_get('elfinder_settings_thumbnail_size', '48'),
    '#size' => 4,
  );

  $form['thumbnail_settings']['tmbdirname'] = array(
    '#type' => 'textfield',
    '#title' => t('Thumbnail directory name'),
    '#default_value' => variable_get('elfinder_settings_thumbnail_dirname', 'tmb'),
    '#size' => 10,
  );

  $form['thumbnail_settings']['imglib'] = array(
    '#type' => 'radios',
    '#title' => t('Image manipulation library'),
    '#default_value' => variable_get('elfinder_settings_thumbnail_imglib', 'auto'),
    '#options' => array(
      'auto' => t('Automatical detection'),
      'imagick' => t('Image Magick'),
      'mogrify' => t('Mogrify'),
      'gd'  => t('GD'),
    ),
  );

  $form['misc_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Miscellaneous settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  
  $form['misc_settings']['rememberlastdir'] = array(
    '#type' => 'radios',
    '#title' => t('Remember last opened directory'),
    '#default_value' => variable_get('elfinder_settings_misc_rememberlastdir', 'true'),
    '#options' => array(
      'true' => t('Yes'),
      'false' => t('No'),
    ),
    '#description' => t('Creates a cookie. Disable if you have issues with caching.'),
  );

  $form['misc_settings']['usesystemjquery'] = array(
    '#type' => 'radios',
    '#title' => t('Use system jQuery'),
    '#default_value' => variable_get('elfinder_settings_misc_usesystemjquery', 'true'),
    '#options' => array(
      'true' => t('Yes'),
      'false' => t('No'),
    ),
    '#description' => t('Use system jQuery and jQuery UI when possible. Disable if you have issues with UI.'),
  );

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  $form['#submit'][] = 'elfinder_admin_submit';
  
  return $form;
}

/**
 * Save form data
 */
function elfinder_admin_submit($form, &$form_state) {
  variable_set('elfinder_settings_thumbnail_size', $form_state['values']['tmbsize']);
  variable_set('elfinder_settings_thumbnail_dirname', $form_state['values']['tmbdirname']);

  if ($form_state['values']['filesystem_root'] == 'custom') {
    if ($form_state['values']['filesystem_root_custom'] != '') {
      variable_set('elfinder_settings_filesystem_root_custom', $form_state['values']['filesystem_root_custom']);
      variable_set('elfinder_settings_filesystem_root', $form_state['values']['filesystem_root']);
    }

    variable_set('elfinder_settings_filesystem_url_custom', $form_state['values']['filesystem_url_custom']);
  }
  else {
    variable_set('elfinder_settings_filesystem_root', $form_state['values']['filesystem_root']);
  }

  variable_set('elfinder_settings_filesystem_mimedetect', $form_state['values']['mime_detect']);
  variable_set('elfinder_settings_filesystem_fileurl', $form_state['values']['file_url_type']);
  variable_set('elfinder_settings_thumbnail_imglib', $form_state['values']['imglib']);
  variable_set('elfinder_settings_filesystem_fileperm', $form_state['values']['file_perm']);
  variable_set('elfinder_settings_filesystem_dirperm', $form_state['values']['dir_perm']);
  variable_set('elfinder_settings_misc_rememberlastdir', $form_state['values']['rememberlastdir']);
  variable_set('elfinder_settings_misc_usesystemjquery', $form_state['values']['usesystemjquery']);
  
  drupal_set_message(t('Changes have been saved.'));
}

/**
 * Validate form data
 */
function elfinder_admin_form_validate($form, &$form_state) {
  $tmbsize = $form_state['values']['tmbsize'];

  if ($form_state['values']['filesystem_root'] == 'custom') {
    $customroot = $form_state['values']['filesystem_root_custom'];

    if ($customroot == '') {
      form_set_error('filesystem_root_custom', t('Filesystem root cannot be empty'));
    }

    $customroot = elfinder_parse_path_tokens($customroot);

    // checking directory accessibility by creating test file
    if (!elfinder_prepare_directory($customroot)) {
      form_set_error('filesystem_root_custom', t('Filesystem root is not writable'));
    }
  }

  if (!preg_match('/^\d{3,4}$/', $form_state['values']['file_perm'])) {
    form_set_error('file_perm', t('Created file permissions should be a numeric'));
  }

  if (!preg_match('/^\d{3,4}?$/', $form_state['values']['dir_perm'])) {
    form_set_error('dir_perm', t('Created directory permissions should be a numeric'));
  }

  if ($tmbsize && !is_numeric($tmbsize)) {
    form_set_error('tmbsize', t('Thumbnail size should be a number'));
  }
}
