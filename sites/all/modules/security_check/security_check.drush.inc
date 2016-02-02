<?php

/**
 * @file
 * Security check module drush commands.
 */

// Include the module file for when we are being called from outside of a site.
require_once __DIR__ . '/security_check.module';

/**
 * Implements hook_drush_command().
 */
function security_check_drush_command() {
  $items = array();

  $items['security-check'] = array(
    'description' => 'Performs a security check of basic site configuration.',
    'aliases' => array('secchk'),
  );

  return $items;
}

/**
 * Drush security-check command callback.
 */
function drush_security_check() {
  $issues = security_check_review_site();
  if (count($issues)) {
    drush_set_error(dt('Potential security issues discovered.'));
    foreach ($issues as $issue) {
      drush_set_error($issue);
    }
    return drush_set_error(dt('Site requires further investigation.'));
  }

  drush_print(dt('Site passed basic security check.'));
  return TRUE;
}
