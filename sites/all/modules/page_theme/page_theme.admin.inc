<?php

/**
 * @file
 * Admin page callbacks for the page_theme module.
 */


/**
 * Menu callback; lists all defined themes.
 */
function page_theme_admin_overview($form, &$form_state) {

  $themes = array();

  $result = db_query('SELECT theme, pages, status, weight FROM {page_theme} ORDER BY weight, theme');
  foreach ($result as $page_theme) {

    $row['name'] = array(
      '#markup' => page_theme_get_theme_name($page_theme->theme),
    );
    $row['pages'] = array(
      '#markup' => str_replace("\n", '<br />', check_plain($page_theme->pages)),
    );
    $row['status'] = array(
      '#type' => 'checkbox',
      '#default_value' => $page_theme->status,
    );
    $row['weight'] = array(
      '#type' => 'weight',
      '#default_value' => $page_theme->weight,
      '#delta' => 50,
      '#attributes' => array(
        'class' => array('page-theme-order-weight'),
      ),
    );
    $row['configure'] = array(
      '#markup' => l(t('configure'), 'admin/structure/page-theme/manage/'. $page_theme->theme .'/configure'),
    );
    $row['delete'] = array(
      '#markup' => l(t('delete'), 'admin/structure/page-theme/manage/'. $page_theme->theme .'/delete'),
    );

    $themes[$page_theme->theme] = $row;
  }

  $form['themes'] = array(
    '#tree' => TRUE,
  );
  $form['themes'] += $themes;
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/structure/page-theme'),
  );

  return $form;
}

function theme_page_theme_admin_overview($variables) {

  $rows = array();

  $form = $variables['form'];
  foreach (element_children($form['themes']) as $theme) {

    $row = array();
    $row[] = drupal_render($form['themes'][$theme]['name']);
    $row[] = drupal_render($form['themes'][$theme]['pages']);
    $row[] = drupal_render($form['themes'][$theme]['status']);
    $row[] = drupal_render($form['themes'][$theme]['weight']);
    $row[] = drupal_render($form['themes'][$theme]['configure']);
    $row[] = drupal_render($form['themes'][$theme]['delete']);

    $rows[] = array('data' => $row, 'class' => array('draggable'));
  }

  if (count($rows) > 0) {

    drupal_add_tabledrag('page-theme-order', 'order', 'sibling', 'page-theme-order-weight');

    $header = array();
    $header[] = t('Theme');
    $header[] = t('Pages');
    $header[] = t('Enabled');
    $header[] = t('Weight');
    $header[] = array('data' => t('Operations'), 'colspan' => '2');

    $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'page-theme-order')));
    $output .= drupal_render_children($form);
  }
  else {

    $output = '<p>' . drupal_placeholder(t('No themes defined.')) . '</p>';
  }

  return $output;
}

function page_theme_admin_overview_submit($form, &$form_state) {

  foreach ($form_state['values']['themes'] as $theme => $value) {

    $field = array();
    $field['status'] = $value['status'];
    $field['weight'] = $value['weight'];
    db_update('page_theme')
      ->fields($field)
      ->condition('theme', $theme)
      ->execute();
  }

  drupal_set_message(t('The configuration has been saved.'));
}


/**
 * Menu callback; adds a theme.
 */
function page_theme_admin_add($form, &$form_state) {

  $form['theme'] = array(
    '#type' => 'select',
    '#title' => t('Theme'),
    '#description' => t('Choose which theme the page should display in.'),
    '#default_value' => '0',
    '#options' => page_theme_get_theme_options(),
    '#required' => TRUE,
  );
  $form['pages'] = array(
    '#type' => 'textarea',
    '#title' => t('Pages'),
    '#description' => t("Enter one page per line as Drupal paths. The '*' character is a wildcard. Example paths are %blog for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>')),
    '#default_value' => '',
    '#required' => TRUE,
  );
  $form['status'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Status'),
    '#default_value' => array('enabled'),
    '#options' => array(
      'enabled' => t('Enabled'),
    ),
  );
  $form['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#default_value' => 0,
    '#delta' => 50,
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add theme'),
  );
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/structure/page-theme'),
  );

  return $form;
}

function page_theme_admin_add_validate($form, &$form_state) {

  if (empty($form_state['values']['theme'])) {
    form_set_error('theme', t('Please select a theme.'));
  }
}

function page_theme_admin_add_submit($form, &$form_state) {

  $field = array();
  $field['theme'] = $form_state['values']['theme'];
  $field['pages'] = trim($form_state['values']['pages']);
  $field['status'] = intval(is_string($form_state['values']['status']['enabled']));
  $field['weight'] = $form_state['values']['weight'];
  db_insert('page_theme')
    ->fields($field)
    ->execute();

  drupal_set_message(t('The theme has been added.'));
  $form_state['redirect'] = 'admin/structure/page-theme';
}


/**
 * Menu callback; edits a theme.
 */
function page_theme_admin_edit($form, &$form_state, $page_theme) {

  $form['theme'] = array(
    '#type' => 'value',
    '#value' => $page_theme->theme,
  );
  $form['theme_name'] = array(
    '#type' => 'item',
    '#title' => t('Theme'),
    '#markup' => page_theme_get_theme_name($page_theme->theme),
  );
  $form['pages'] = array(
    '#type' => 'textarea',
    '#title' => t('Pages'),
    '#description' => t("Enter one page per line as Drupal paths. The '*' character is a wildcard. Example paths are %blog for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>')),
    '#default_value' => $page_theme->pages,
    '#required' => TRUE,
  );
  $form['status'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Status'),
    '#default_value' => $page_theme->status ? array('enabled') : array(),
    '#options' => array(
      'enabled' => t('Enabled'),
    ),
  );
  $form['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#default_value' => $page_theme->weight,
    '#delta' => 50,
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update theme'),
  );
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/structure/page-theme'),
  );

  return $form;
}

function page_theme_admin_edit_submit($form, &$form_state) {

  $field = array();
  $field['pages'] = trim($form_state['values']['pages']);
  $field['status'] = intval(is_string($form_state['values']['status']['enabled']));
  $field['weight'] = $form_state['values']['weight'];
  db_update('page_theme')
    ->fields($field)
    ->condition('theme', $form_state['values']['theme'])
    ->execute();

  drupal_set_message(t('The theme has been updated.'));
  $form_state['redirect'] = 'admin/structure/page-theme';
}


/**
 * Menu callback; deletes a theme.
 */
function page_theme_admin_delete($form, &$form_state, $page_theme) {

  $form['theme'] = array(
    '#type' => 'value',
    '#value' => $page_theme->theme,
  );
  $question = t('Are you sure you want to delete the theme %theme?', array('%theme' => page_theme_get_themes($page_theme->theme)));
  $path = isset($_GET['destination']) ? $_GET['destination'] : 'admin/structure/page-theme';

  return confirm_form($form, $question, $path);
}

function page_theme_admin_delete_submit($form, &$form_state) {

  if ($form_state['values']['confirm']) {

    $theme = $form_state['values']['theme'];
    db_delete('page_theme')
      ->condition('theme', $theme)
      ->execute();

    drupal_set_message(t('The theme has been deleted.'));
    $form_state['redirect'] = 'admin/structure/page-theme';
    return;
  }
}
