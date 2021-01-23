<?php

/**
 * @file
 * Common pages for the Media WYSIWYG module.
 */

/**
 * Form callback for embedding media.
 *
 * Allows the user to pick a view mode (format) and various other settings for a
 * media instance. The values from this form follows the media token schema and
 * its values are mapped directly to media tokens.
 */
function media_wysiwyg_format_form($form, &$form_state, $file) {
  $form_state['file'] = $file;

  // Build instance settings from token schema defaults and anything from query
  // parameters (ajax).
  $token_schema = media_wysiwyg_schema_token();
  $instance_settings = array_map(function ($property) {
    return $property['default'];
  }, $token_schema);

  if (isset($_GET['instance_settings'])) {
    $instance_settings = array_merge($instance_settings, drupal_json_decode($_GET['instance_settings']));
  }
  // For new media instances this array is the entire file entity form,
  // including file type ('type'), which conflict with the 'type' property of
  // our media token. Adjust our settings to adhere to the media token schema.
  $instance_settings['type'] = 'media';
  $instance_settings['fid'] = $file->fid;
  $instance_settings['file_type'] = $file->type;

  // Build view mode options, assert that we have a valid view mode in instance
  // settings and build a rendered preview of each one of them.
  $view_mode_options = media_wysiwyg_get_file_view_mode_options($file);
  if (!count($view_mode_options)) {
    throw new Exception('Unable to continue, no available formats for displaying media.');
  }
  if (!isset($view_mode_options[$instance_settings['view_mode']])) {
    reset($view_mode_options);
    $instance_settings['view_mode'] = key($view_mode_options);
  }
  if (!isset($form_state['storage']['view_mode_previews'])) {
    $form_state['storage']['view_mode_previews'] = array();
    foreach ($view_mode_options as $view_mode => $view_mode_label) {
      $view_mode_preview = media_wysiwyg_get_file_without_label($file, $view_mode, array('wysiwyg' => TRUE));
      $form_state['storage']['view_mode_previews'][$view_mode] = drupal_render($view_mode_preview);
    }
  }
  // Add the previews by reference so that they can easily be altered by
  // changing $form['#formats'].
  $form['#formats'] = &$form_state['storage']['view_mode_previews'];

  // Add the required libraries, JavaScript, settings and CSS for the form.
  $us = drupal_get_path('module', 'media_wysiwyg');
  $js_settings = array('media' => array('formatFormFormats' => &$form_state['storage']['view_mode_previews']));
  $form['#attached']['library'][] = array('media', 'media_base');
  $form['#attached']['library'][] = array('system', 'form');
  $form['#attached']['css'][] = $us . '/css/media_wysiwyg.css';
  $form['#attached']['js'][] = $us . '/js/media_wysiwyg.format_form.js';
  $form['#attached']['js'][] = array('data' => $js_settings, 'type' => 'setting');

  $form['title'] = array(
    '#markup' => t('Embedding %filename', array('%filename' => $file->filename)),
  );
  $preview = media_get_thumbnail_preview($file);
  $form['preview'] = array(
    '#type' => 'markup',
    '#markup' => drupal_render($preview),
  );

  // Everything under $form['options'] will get passed to the WYSIWYG
  // environment and used as media instance settings which in turn are
  // serialized to actual media tokens.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('options'),
  );
  // Add the non-editable variables of media tokens.
  foreach (array('type', 'fid', 'file_type') as $property) {
    $form['options'][$property] = array(
      '#type' => 'hidden',
      '#value' => $instance_settings[$property],
    );
  }

  // @TODO: Display more verbose information about which formatter and what it
  // does.
  $form['options']['view_mode'] = array(
    '#type' => 'select',
    '#title' => t('Display as'),
    '#options' => $view_mode_options,
    '#default_value' => $instance_settings['view_mode'],
    '#description' => t('Choose the type of display you would like for this
      file. Please be aware that files may display differently than they do when
      they are inserted into an editor.'),
  );
  if (variable_get('media_wysiwyg_alignment', FALSE)) {
    $form['options']['alignment'] = array(
      '#type' => 'select',
      '#title' => t('Alignment'),
      '#options' => $token_schema['alignment']['options'],
      '#description' => t('Choose how you would like the media to be aligned with surrounding content.'),
      '#default_value' => $instance_settings['alignment'],
    );
  }
  $form['options']['instance_fields'] = array(
    '#type' => 'radios',
    '#title' => t("Instance field values"),
    '#options' => $token_schema['instance_fields']['options'],
    '#description' => t("<em>Global</em> will use the values from the file itself. <em>Override</em> will set custom values for this instance only, based on the file's global field values.<br /><strong>Be aware:</strong> Using <em>Global</em> will cause later edits on the file will affect this instance as well."),
    '#default_value' => $instance_settings['instance_fields'],
  );
  // Add fields from the file, so that we can override them if necessary.
  $fields = media_wysiwyg_filter_field_parser($instance_settings, $file);
  foreach ($fields as $field_name => $field_value) {
    $file->{$field_name} = $field_value;
  }
  $form['options']['fields'] = array();
  field_attach_form('file', $file, $form['options']['fields'], $form_state);
  foreach (field_info_instances('file', $file->type) as $field_name => $field_instance) {
    $info = field_info_field($field_name);
    $allow = !empty($field_instance['settings']['wysiwyg_override']);
    // Only single valued fields can be overridden normally, unless Media is
    // configured otherwise with "media_wysiwyg_wysiwyg_override_multivalue".
    if ($allow && $info['cardinality'] != 1) {
      $allow = variable_get('media_wysiwyg_wysiwyg_override_multivalue', FALSE);
    }
    $form['options']['fields'][$field_name]['#access'] = $allow;
    if ($allow) {
      $form['options']['fields'][$field_name]['#states'] = array(
        'visible' => array(
          ':input[name="instance_fields"]' => array('value' => "override"),
        ),
      );
    }
  }

  // Field to attach external url's to files for linking.
  if (variable_get('media_wysiwyg_external_link', FALSE)) {
    if ($file->type == 'image') {
      $form['options']['external_url'] = array(
        '#type' => 'textfield',
        '#title' => t('Link Image'),
        '#description' => t('Enter a URL to turn the image into a link.'),
        '#maxlength' => 2048,
        '#default_value' => $instance_settings['external_url'],
      );
    }
  }

  // Add view mode preview.
  media_wysiwyg_format_form_view_mode($form, $form_state, $file, $instance_settings);

  // Similar to a form_alter, but we want this to run first so that
  // media.types.inc can add the fields specific to a given type (like alt tags
  // on media). If implemented as an alter, this might not happen, making other
  // alters not be able to work on those fields.
  // @todo: We need to pass in existing values for those attributes.
  drupal_alter('media_wysiwyg_format_form_prepare', $form, $form_state, $file);

  return $form;
}

/**
 * Add ajax preview when selecting view mode in wysiwyg editor.
 */
function media_wysiwyg_format_form_view_mode(&$form, $form_state, $file, $instance_settings) {
  // Find view mode for this media item, either from form-submitted values or
  // the already verified mode from instance_settings.
  $view_mode = empty($form_state['values']['view_mode']) ? $instance_settings['view_mode'] : $form_state['values']['view_mode'];

  $link_options = array(
    'attributes' => array(
      'class' => 'button',
      'title' => t('Use for replace fox or edit file fields.'),
    ),
    'query' => drupal_get_destination(),
  );
  if (!empty($_GET['render'])) {
    $link_options['query']['render'] = $_GET['render'];
  }

  $form['preview'] = array();
  $form['preview']['#prefix'] = '<div class="media-preview-group"><div class="media-item"><div class="media-thumbnail">';
  $form['preview']['#suffix'] = '</div><div class="label-wrapper"><label class="media-filename">' . check_plain($file->filename) . '</label></div></div><div class="edit-file-link">' . l(t('Edit file'), 'file/' . $file->fid . '/edit', $link_options) . '</div></div>';
  $form['preview']['thumbnail'] = file_view_file($file, $view_mode);
  $form['preview']['thumbnail']['#prefix'] = '<div id="media-preview">';
  $form['preview']['thumbnail']['#suffix'] = '</div>';

  if (!isset($form['options']['view_mode']['#default_value'])) {
    $form['options']['view_mode']['#default_value'] = $view_mode;
  }
  $form['options']['view_mode']['#ajax'] = array(
    'callback' => 'media_wysiwyg_format_form_preview',
    'wrapper' => 'media-preview',
  );

  $wysiwyg_view_mode = db_query('SELECT view_mode FROM {media_view_mode_wysiwyg} WHERE type = :type', array(':type' => $file->type))->fetchField();
  $view_modes = media_wysiwyg_get_wysiwyg_allowed_view_modes($file);
  $formats = $options = array();

  foreach ($view_modes as $view_mode => $view_mode_info) {
    $options[$view_mode] = $view_mode_info['label'];

    if (!empty($wysiwyg_view_mode)) {
      $element = media_wysiwyg_get_file_without_label($file, $wysiwyg_view_mode, array('wysiwyg' => TRUE));
    }
    else {
      $element = media_wysiwyg_get_file_without_label($file, $view_mode, array('wysiwyg' => TRUE));
    }

    $formats[$view_mode] = drupal_render($element);
  }

  $form['#formats'] = $formats;
  $form['options']['view_mode']['#options'] = $options;
}

/**
 * AJAX callback to select portion of format form to be updated with a preview.
 *
 * @param array $form
 *   An associative array containing the structure of the form.
 * @param array $form_state
 *   An associative array containing the current state of the form.
 *
 * @return array
 *   The preview form item.
 */
function media_wysiwyg_format_form_preview($form, $form_state) {
  return $form['preview']['thumbnail'];
}

/**
 * Confirmation wrapper form around upgrading media tokens.
 */
function media_wysiwyg_upgrade_content_tokens($form, &$form_state) {
  return confirm_form(
    $form,
    t("Media token upgrade"),
    'admin/reports/status',
    t("<p>This will inspect and upgrade all inserted media tokens in all text fields on all entities on the site where files are used. Make sure you have a recent backup of your database before you perform this operations. The text fields are individually upgraded directly in storage and no new revisions created for the touched entities. The post upgrade summary will only report the <em>number</em> of affected entities, not list them all.</p> <p><strong>Are you ready for this upgrade?</strong></p>")
  );
}

/**
 * Upgrade media tokens in content.
 *
 * Over time the format of media tokens will change and this function will
 * assert that existing tokens in content will remain up to date with todays
 * standard.
 *
 * It examines the {file_usage} table and searches all entities for filtered
 * text fields with macros. All found media tokens will be upgraded to the
 * latest token format version and the individual text fields will be updated in
 * db directly, omitting all hooks otherwise run during entity update.
 */
function media_wysiwyg_upgrade_content_tokens_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/reports/status';
  $batch = array(
    'title' => t("Upgrading media tokens"),
    'operations' => array(
      array(
        'media_wysiwyg_upgrade_content_tokens_batch',
        array(),
      ),
    ),
    'finished' => 'media_wysiwyg_upgrade_content_tokens_finish',
    'file' => drupal_get_path('module', 'media_wysiwyg') . '/includes/media_wysiwyg.upgrade.inc',
  );
  batch_set($batch);
}
