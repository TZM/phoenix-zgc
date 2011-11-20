<?php 
// $Id$
/**
 * @file
 *   Admin settings and update page
 */
/**
 * Project has a new release available.
 */
define('L10N_UPDATE_NOT_CURRENT', 4);

/**
 * Project is up to date.
 */
define('L10N_UPDATE_CURRENT', 5);

/**
 * Project's status cannot be checked.
 */
define('L10N_UPDATE_NOT_CHECKED', -1);

/**
 * No available update data was found for project.
 */
define('L10N_UPDATE_UNKNOWN', -2);

/**
 * There was a failure fetching available update data for this project.
 */
define('L10N_UPDATE_NOT_FETCHED', -3);

/**
 * Update form
 * 
 * @todo selectable packages
 * @todo check language support in server
 * @todo check file update dates
 */
function l10n_update_admin_import_form() {
  module_load_include('inc', 'l10n_update');
  // For now we get package information provided by modules
  $projects = l10n_update_get_projects();
  $languages = l10n_update_language_list('name');  

  if ($projects && $languages) {
    $history = l10n_update_refresh_history();
    $available = l10n_update_available_releases();
    $updates = _l10n_update_compare($projects, $history, $available);
    
    $form['updates'] = array('#type' => 'value', '#value' => $updates);
    
    $form['last'] = array(
      '#type' => 'item',
      '#title' => t('Last checked'),
      '#value' => format_date(variable_get('l10n_update_last_check', 0)),
    );

    $form['projects'] = array('#value' => theme('l10n_update_project_status', $projects, $languages, $history, $available, $updates));
    $form['languages'] = array(
      '#title' => t('Languages'),
      '#type' => 'checkboxes',
      '#options' => $languages,
      '#default_value' => array(language_default('language')),
      '#description' => t('Select one or more languages to download and update.'),
    );
    $form['mode'] = array('#type' => 'radios',
      '#title' => t('Mode'),
      '#default_value' => variable_get('l10n_update_import_mode', LOCALE_IMPORT_KEEP),
      '#options' => _l10n_update_admin_import_options(),
    );
    $form['buttons']['download'] = array('#type' => 'submit', '#value' => t('Update translations'));
    $form['buttons']['refresh'] = array('#type' => 'submit', '#value' => t('Refresh information'));
  } else {
    $form['warning'] = array('#value' => t('No projects or languages to update.'));
  }
  return $form;
}

/**
 * Update form
 */
function l10n_update_admin_import_form_submit($form, $form_state) {
  module_load_include('inc', 'l10n_update');
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  $projects = l10n_update_get_projects();
  
  if ($op == t('Update translations')) {
    $languages = array_filter($form_state['values']['languages']);
    $updates = $form_state['values']['updates'];
    $mode = $form_state['values']['mode'];
    
    if ($languages && $projects && $updates) {
      $batch = l10n_update_batch_multiple($projects, $languages, $updates, $mode);
      batch_set($batch);
    }
    else {
      drupal_set_message(t('No packages or languages selected.'), 'error');
    }
  }
  elseif ($op == t('Refresh information')) {
    if ($available = l10n_update_available_releases(TRUE)) {
      drupal_set_message(t('Fetched information about available updates from the server'));
    }
    else {
      drupal_set_message(t('Failed to fetch information about available updates from the server', 'error'));
    }
  }
}

/**
 * Settings form
 */
function l10n_update_admin_settings_form() {
  $form['l10n_update_import_mode'] = array('#type' => 'radios',
    '#title' => t('Import mode'),
    '#default_value' => variable_get('l10n_update_import_mode', LOCALE_IMPORT_KEEP),
    '#options' => _l10n_update_admin_import_options(),
  );
  $form['l10n_update_check_frequency'] = array(
    '#type' => 'radios',
    '#title' => t('Check for updates'),
    '#default_value' => variable_get('l10n_update_check_frequency', 0),
    '#options' => array(
      0 => t('Never (manually)'),
      '1' => t('Daily'),
      '7' => t('Weekly'),
    ),
    '#description' => t('Select how frequently you want to automatically check for updated translations for installed modules and themes.'),
  );
  return system_settings_form($form);
}

/**
 * Get array of import options 
 */
function _l10n_update_admin_import_options() {
  return array(
    LOCALE_IMPORT_OVERWRITE => t('Strings in the uploaded file replace existing ones, new ones are added'),
    LOCALE_IMPORT_KEEP => t('Edited strings are kept, only default ones (previously imported) are overwritten and new strings are added')
  );
}
/**
 * Format project update status
 */
function theme_l10n_update_project_status($projects, $languages, $history, $available, $updates) {
  // We use the core update module CSS
  drupal_add_css(drupal_get_path('module', 'update') .'/update.css');   
  $output = '';  
  //$header = array(t('Project'), t('Current version'), t('Available update'), '');
  $header = $rows = array();

  foreach ($projects as $name => $project) {
    $row = '<div class="version-status">';
    if (empty($available[$name])) {
      // Remote information not checked
      $class = 'unknown';
      $status = 'unknown';
    }
    elseif (empty($updates[$name])) {
      // No updates available
      $class = 'ok';
      $status = 'ok';
    }
    else {
      // Update available
      $class = 'warning';
      $status = 'update';
    }    
    
    $row = theme('l10n_update_version_status', $status);
    
    $row .= "<div class=\"project $class\">";
    $title = isset($project['title']) ? $project['title'] : $project['name'];
    $row .= check_plain($title);
    $row .= ' '. check_plain($project['info']['version']);
    $row .= "</div>\n";

    $row .= "<div class=\"versions\">\n";
    $versions = array();
    foreach ($languages as $lang => $language) {
      $current = isset($history[$name]->updates[$lang]) ? theme('l10n_update_tag_date', $history[$name]->updates[$lang]) : '';
      $update = isset($updates[$name][$lang]) ? theme('l10n_update_tag_date', $updates[$name][$lang]) : '';      
      $download = isset($updates[$name][$lang]) ? l(t('Download'), $updates[$name][$lang]['download_link']) : '';
      if ($update) {
        $status = 'update';
        $class = 'warning';
      }
      elseif ($current) {
        $status = $class = 'ok';
      }
      else {
        $status = $class ='unknown';
      }
      $version = array(
        array('data' => $language, 'class' => 'version-title'),
        $current, 
        $update,          
        array('data' => $download, 'class' => 'version-links'),
        theme('l10n_update_version_status', $status),     
      );
      $versions[] = array('data' => $version, 'class' => $class); 
    }
    $row .= theme('table', array(), $versions);
    $row .= "</div>\n";
    $rows[] = array($row);
  }
  $output .= theme('table', $header, $rows, array('class' => 'update')); 
  return $output; 
}

/**
 * Format tag and release date
 * @param $tag
 * @param $date
 * @return unknown_type
 */
function theme_l10n_update_tag_date($release) {
  if (is_object($release)) {
    // This is a release history object
    $tag = $release->download_version;
    $date = $release->download_date;
  }
  else {
    // This is fetched information
    $tag = $release['tag'];
    $date = $release['date'];
  }
  return '<strong>' . check_plain($tag) . '</strong> <span class="version-date">' . format_date($date, 'short') . '</span>';
}

/**
 * Format version status with icon
 * 
 * @param $status
 * 
 * @return HTML
 */
function theme_l10n_update_version_status($status) {
  $output = '<div class="version-status">';
  switch ($status) {
    case 'ok':
      $icon = theme('image', 'misc/watchdog-ok.png', t('ok'), t('ok'));
      $msg = '<span class="current">'. t('Up to date') .'</span>';
      break;
    case 'update':
      $icon = theme('image', 'misc/watchdog-warning.png', t('warning'), t('warning'));
      $msg = '<span class="not-current">'. t('Update available') .'</span>';
      break;
    case 'unknown':
      $icon = theme('image', 'misc/watchdog-warning.png', t('warning'), t('warning'));
      $msg = '<span class="not-supported">'. t('No information') .'</span>';
      break;
  }
  $output .= $msg;
  $output .= '<span class="icon">'. $icon .'</span>';
  $output .= "</div>";
  return $output;
}