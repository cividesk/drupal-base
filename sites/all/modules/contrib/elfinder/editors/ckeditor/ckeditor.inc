<?php
// $Id$

/**
 * @file
 * CKeditor integration plugin
 */

/**
  * Pseudo-hook for elfinder hook_wysiwyg_plugin implementation
  */
function elfinder_ckeditor_elfinder_editor_plugin($options) {
  return array(
    'elfinder' => array(
      'extensions' => array('elfinder' => t('elFinder')),
      'url' => $options['homepage_url'],
      'options' => array(
        'filebrowserBrowseUrl' => $options['elfinder_url'],
      ),
      'load' => FALSE,
    ),
  );
}
