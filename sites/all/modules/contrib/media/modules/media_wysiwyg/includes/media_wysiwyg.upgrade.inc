<?php

/**
 * @file
 * Code related to media token upgrades.
 */

/**
 * Batch API callback.
 *
 * @see media_wysiwyg_upgrade_content_tokens()
 */
function media_wysiwyg_upgrade_content_tokens_batch(&$context) {
  $items_per_run = 20;

  $query = db_select('file_managed', 'fm');
  $query->innerJoin('file_usage', 'fu', 'fm.fid = fu.fid');
  $query->fields('fu', array('type', 'id'))
    ->condition('fu.module', 'media')
    ->distinct();
  if (empty($context['results']['!entities_total'])) {
    // The countQuery() returns a cloned SelectQuery object, so we can happily
    // execute it without disturbing the original query. Also, the keys in the
    // 'results' entry are prefixed with '!' so the $results variable can be
    // sendt directly to t() in the batch finished callback.
    $context['results']['!entities_total'] = $query->countQuery()->execute()->fetchField();
    $context['results']['!entities_upgraded'] = 0;
    $context['results']['!tokens_total'] = 0;
    $context['results']['!tokens_upgraded'] = 0;

    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_type'] = '';
    $context['sandbox']['current_id'] = 0;
    $context['sandbox']['run'] = 0;
  }
  $query->orderBy('fu.type')
    ->orderBy('fu.id')
    ->range($context['sandbox']['run'] * $items_per_run, $items_per_run);
  foreach ($query->execute() as $usage) {
    $context['sandbox']['progress']++;
    $context['sandbox']['current_id'] = $usage->id;
    $context['sandbox']['current_type'] = $usage->type;
    $context['message'] = t("Upgrading tokens: Processing entity # !progress/!total. Last examined: !entity_type/!entity_id", array(
      '!progress' => $context['sandbox']['progress'],
      '!total' => $context['results']['!entities_total'],
      '!entity_type' => $usage->type,
      '!entity_id' => $usage->id,
    ));
    try {
      $report = media_wysiwyg_upgrade_entity_tokens($usage->type, $usage->id);
    }
    catch (Exception $e) {
      watchdog('media', "Failed to upgrade tokens in entity %entity_id with id %entity_id", array('%entity_type' => $usage->type, '%entity_id' => $usage->id), WATCHDOG_WARNING);
    }
    $context['results']['!tokens_total'] += $report['found'];
    $context['results']['!tokens_upgraded'] += $report['upgraded'];
    if ($report['upgraded']) {
      $context['results']['!entities_upgraded']++;
    }
  }
  if ($context['sandbox']['progress'] < $context['results']['!entities_total']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['results']['!entities_total'];
  }
  $context['sandbox']['run']++;
}

/**
 * Batch API finish callback.
 *
 * @see media_wysiwyg_upgrade_content_tokens()
 */
function media_wysiwyg_upgrade_content_tokens_finish($success, $results, $operations) {
  if ($success) {
    $results['!version'] = MEDIA_WYSIWYG_TOKEN_VERSION;
    drupal_set_message(t("Media token version !version upgrade summary: <ul><li>Entities inspected: !entities_total</li> <li>Media tokens found: !tokens_total.</li> <li>Tokens upgraded: !tokens_upgraded</li> <li>Entities affected: !entities_upgraded</li></ul>", $results));
    variable_set('media_wysiwyg_token_version', MEDIA_WYSIWYG_TOKEN_VERSION);
    // Instead of using the costly drupal_flush_all_caches() we flush relevant
    // core caches and invoke hook_flush_caches() to allow content cache modules
    // (entitycache) to flush whatever needed.
    $core = array('cache', 'cache_filter', 'cache_page', 'cache_block');
    $cache_tables = array_merge(module_invoke_all('flush_caches'), $core);
    foreach ($cache_tables as $table) {
      cache_clear_all('*', $table, TRUE);
    }
  }
  else {
    drupal_set_message(t('Media token upgrade failed. See system log'), 'warning');
  }
}

/**
 * Upgrade a single media token to latest version.
 *
 * @param array $instance_settings
 *   The media instance settings, aka tag_info.
 */
function media_wysiwyg_upgrade_token(array &$instance_settings) {
  $version = variable_get('media_wysiwyg_token_version', '');
  if (version_compare($version, '3.0', '<')) {
    media_wysiwyg_aggregate_alignment($instance_settings);
  }
  if (version_compare($version, '4.0', '<')) {
    media_wysiwyg_token_upgrade_40($instance_settings);
  }
  // Successive upgrades follows here.
}

/**
 * Upgrade media tokens in filtered text fields for a given entity.
 *
 * Only upgraded text fields will be updated in storage, omitting the costly
 * entity_save(). I.e. no new entity revisions.
 *
 * @param string $entity_type
 *   The entity type.
 * @param int $id
 *   The entity ID to scan for media tokens in.
 *
 * @return array
 *   The number of found and upgraded tokens in entity, keyed by 'found' and
 *   'upgraded'.
 */
function media_wysiwyg_upgrade_entity_tokens($entity_type, $id) {
  $report = array(
    'found' => 0,
    'upgraded' => 0,
  );

  // Assert that the entity type has a valid controller class to avoid getting
  // php parse errors during token upgrades. See issue #2950150.
  $type_info = entity_get_info($entity_type);
  if (empty($type_info['controller class'])) {
    return $report;
  }
  if (!($entity = entity_load($entity_type, array($id)))) {
    return $report;
  };
  $entity = reset($entity);

  list(,, $bundle) = entity_extract_ids($entity_type, $entity);

  // Map between storage engines and the fields for it that will be updated.
  $storages = array();
  foreach (media_wysiwyg_filter_fields_with_text_filtering($entity_type, $entity) as $field_name) {
    $langcode = field_language($entity_type, $entity, $field_name);
    if (!isset($entity->{$field_name}[$langcode])) {
      continue;
    }
    $field = field_info_instance($entity_type, $field_name, $bundle);
    $field = field_info_field_by_id($field['field_id']);
    $field_id = $field['id'];
    foreach ($entity->{$field_name}[$langcode] as &$field_item) {
      if (empty($field_item['value'])) {
        continue;
      }
      preg_match_all(MEDIA_WYSIWYG_TOKEN_REGEX, $field_item['value'], $matches);
      foreach ($matches[0] as $tag_orig) {
        $tag_orig = str_replace(array('[[', ']]'), '', $tag_orig);
        $tag_info = drupal_json_decode($tag_orig);
        if (!isset($tag_info['type']) || $tag_info['type'] != 'media') {
          continue;
        }
        $report['found']++;
        // Perform the actual upgrade.
        media_wysiwyg_upgrade_token($tag_info);
        $tag_new = drupal_json_encode($tag_info);
        if (strcmp($tag_orig, $tag_new) == 0) {
          // No changes. Don't bother saving this.
          continue;
        }
        $report['upgraded']++;
        $tag_orig = '[[' . $tag_orig . ']]';
        $tag_new = '[[' . $tag_new . ']]';
        // The field_item is available by reference, so it will updated in the
        // entity directly. If several identical tokens exists within this text
        // value they will all be replaced here, and the next match iteration
        // will not perform any replacement as it the search string will not be
        // found. No big deal and no need to add a special case for this.
        $field_item['value'] = str_replace($tag_orig, $tag_new, $field_item['value']);
        $storages[$field['storage']['type']][$field_id] = $field_id;
      }
    }
  }
  // Write updated tokens to storage.
  foreach ($storages as $storage => $fields) {
    $storage_info = field_info_storage_types($storage);
    module_invoke($storage_info['module'], 'field_storage_write', $entity_type, $entity, FIELD_STORAGE_UPDATE, $fields);
  }
  return $report;
}

/**
 * Find and aggregate alignment in media tag.
 *
 * Version < 3.0 upgrade. Aggregate and uniform various methods for aligning
 * media to one setting. In the process it removes all 'old' methods, like
 * inline style floats and the align attribute. The final alignment is stored in
 * $tag_info['alignment'] and is returned.
 *
 * @param array $tag_info
 *   JSON-decoded media macro.
 *
 * @return string
 *   The alignment of the media, either 'left', 'center', 'right' or '' if
 *   unset.
 */
function media_wysiwyg_aggregate_alignment(array &$tag_info) {
  $alignment = '';
  if (!isset($tag_info['attributes']) || !is_array($tag_info['attributes'])) {
    return $alignment;
  }
  $attributes = &$tag_info['attributes'];
  // Media alignment handling: Transform alignment using either the 'align' or
  // 'style' attributes to configuration. Inline style has higher priority over
  // the align attribute, so we check the 'align' attribute first.
  if (isset($attributes['align'])) {
    $alignment = $attributes['align'];
    unset($attributes['align']);
  }

  // Extract from inline style (float).
  if (!empty($attributes['style'])) {
    $css_properties = media_wysiwyg_parse_css_declarations($attributes['style']);
    if (isset($css_properties['float'])) {
      $alignment = $css_properties['float'];
      unset($css_properties['float']);
      $attributes['style'] = media_wysiwyg_stringify_css_declarations($css_properties);

      if (!$attributes['style']) {
        unset($attributes['style']);
      }
    }
  }

  // Check for alignment in media alignment classes.
  $alignment_options = array('left', 'center', 'right');
  if (!empty($attributes['class'])) {
    $class_was_exploded = FALSE;
    if (!is_array($attributes['class'])) {
      $attributes['class'] = explode(' ', $attributes['class']);
      $class_was_exploded = TRUE;
    }
    $alignment_classes = array_map(function ($item) {
      return 'media-wysiwyg-align-' . $item;
    }, $alignment_options);

    $alignments = array_intersect($attributes['class'], $alignment_classes);
    if ($alignments) {
      sscanf(end($alignments), 'media-wysiwyg-align-%s', $alignment);
      $attributes['class'] = array_diff($attributes['class'], $alignment_classes);
    }
    if ($class_was_exploded) {
      $attributes['class'] = implode(' ', $attributes['class']);
    }
  }

  // The actual setting, if it exists, should triumph all of the above.
  if (!empty($tag_info['alignment'])) {
    $alignment = $tag_info['alignment'];
  }

  // The one place to rule them all:
  $tag_info['alignment'] = in_array($alignment, $alignment_options) ? $alignment : '';
  return $tag_info['alignment'];
}

/**
 * Upgrade a single media token from 3.0 to 4.0.
 *
 * @param array $settings
 *   Media instance settings, aka tag_info.
 */
function media_wysiwyg_token_upgrade_40(array &$settings) {
  // The old 'fields' subarray is now flattened and main structure of our token.
  // Just move every values to settings root, and rename 'format' to
  // 'view_mode'.
  if (is_array($settings['fields'])) {
    // First, remove properties that leaked over to the fields sub-structure.
    // @see issue #2946265.
    foreach (media_wysiwyg_allowed_attributes() as $attribute) {
      unset($settings['fields'][$attribute]);
    }
    foreach ($settings['fields'] as $property => $value) {
      $settings[$property] = $value;
    }
    // Although 'view_mode' should be set, we double check and use the one
    // provided from (the old) media_wysiwyg_format_form().
    if (!empty($settings['fields']['format'])) {
      $settings['view_mode'] = $settings['format'];
    }
  }

  // Validate and rebuild. Exceptions are cought further up the call stack.
  media_wysiwyg_validate_instance_settings($settings);
  $settings = media_wysiwyg_rebuild_instance_settings($settings);
}

/**
 * Rebuild media token based on schema and allowed overridable fields.
 *
 * This only includes properties of media instance settings that are part of the
 * token schema. Attributes that are fed by fields (usually alt and title) are
 * removed, classes that are generated during input filtering are removed and
 * properties that are empty are removed.
 *
 * @param array $settings
 *   The media instance settings.
 *
 * @return array
 *   The cleaned and rebuilt media settings.
 */
function media_wysiwyg_rebuild_instance_settings(array $settings) {
  $rebuild = array();

  $schema = media_wysiwyg_schema_token();
  foreach ($schema as $property => $prop_settings) {
    if (isset($settings[$property])) {
      if (!empty($settings[$property])) {
        $rebuild[$property] = $settings[$property];
      }
    }
  }
  // Inspect and remove old and generated classes.
  if (!empty($rebuild['attributes']['class'])) {
    $classes = explode(' ', $rebuild['attributes']['class']);
    $classes = array_filter($classes, function ($class) {
      return
        !($class == 'media-element' ||
          preg_match('/^file-\S+/', $class) ||
          preg_match('/^media-wysiwyg-align-\S+/', $class));
    });
    // Old versions of media_wysiwyg added the view mode as its own class name,
    // without prefix. Remove this as well.
    $view_mode_class = strtr($rebuild['view_mode'], '_', '-');
    if (($index = array_search($view_mode_class, $classes)) !== FALSE) {
      unset($classes[$index]);
    }
    if ($classes) {
      $rebuild['attributes']['class'] = implode(' ', $classes);
    }
    else {
      unset($rebuild['attributes']['class']);
    }
  }
  if (!empty($rebuild['attributes'])) {
    // Allowed attributes in older versions of media.
    unset($rebuild['attributes']['data-delta']);
    unset($rebuild['attributes']['data-fid']);
    unset($rebuild['attributes']['data-media-element']);

    // Remove attributes that are fed through file entity fields or media token
    // field overrides.
    foreach (media_wysiwyg_get_attribute_fields() as $attribute => $field_name) {
      unset($rebuild['attributes'][$attribute]);
    }
  }
  if (empty($rebuild['attributes'])) {
    unset($rebuild['attributes']);
  }

  // Move over overridable fields from old $settings to new $rebuild.
  $file_type = db_select('file_managed', 'f')->fields('f', array('type'))->condition('fid', $rebuild['fid'])->execute()->fetchField();
  if ($file_type) {
    $overridable_types = media_wysiwyg_overridable_fields();
    if (($overridables = $overridable_types[$file_type])) {
      foreach ($overridables as $field_name => $status) {
        foreach ($settings as $property => $value) {
          if (strpos($property, $field_name . '[') === 0) {
            $rebuild[$property] = $settings[$property];
          }
        }
      }
    }
  }

  return $rebuild;
}
