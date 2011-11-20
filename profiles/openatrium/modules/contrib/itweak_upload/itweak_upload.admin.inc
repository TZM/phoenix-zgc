<?php
// $Id: itweak_upload.admin.inc,v 1.1.2.12 2010/03/26 18:24:58 iva2k Exp $

/**
 * @file
 * Administration settings for iTweak Upload module
 */

/*
 * Collect list of available presets for image options.
 */
function _itweak_upload_setting_image_options($default_text = NULL) {
  $ret = array();
  if ($default_text) $ret['_default'] = $default_text;
  $ret['_none'] = t('No preview');
  $ret['_original'] = t('Full image');
  if (module_exists('imagecache')) {
    foreach (imagecache_presets() as $preset_id => $preset) {
      $ret[$preset_id] = $preset['presetname'];
    }
  }
  else {
    drupal_set_message(t('Imagecache module is not installed / enabled. Previously configured attachement preview images will be disabled. Use of "Full image" will impact website performance.'));
  }
  return $ret;
}

/*
 * Collect list of available handlers for link options.
 */
function _itweak_upload_setting_link_options($default_text = NULL) {
  $ret = array();
  if ($default_text) $ret['_default'] = $default_text;
  $ret['none'] = t('Open image');
  if (module_exists('lightbox2')) {
    $ret['lightbox2'] = t('Lightbox');
    $ret['lightbox2grouped'] = t('Lightbox Grouped');
    $ret['lightbox2slideshow'] = t('Lightbox Slideshow');
  }
  if (module_exists('thickbox')) {
    $ret['thickbox'] = t('Thickbox');
  }
  if (module_exists('fancybox')) {
    $ret['fancybox'] = t('Fancybox');
  }
  if (module_exists('shadowbox')) {
    $ret['shadowbox'] = t('Shadowbox');
    $ret['shadowboxgrouped'] = t('Shadowbox Grouped');
  }
  if (module_exists('highslide')) {
    $ret['highslide'] = t('Highslide');
    $ret['highslidegrouped'] = t('Highslide Grouped');
  }
  return $ret;
}

/**
 * Pre-render callback to adjust weights of fields.
 */
function _itweak_upload_node_type_alter_weights($elements) {
  if (isset($elements['workflow']) && isset($elements['workflow']['#weight'])
    && isset($elements['itweak_upload_display'])) {
    $elements['itweak_upload_display']['#weight'] = $elements['workflow']['#weight'] + 0.001;
	unset($elements['#sorted']);
  }
  if (isset($elements['workflow']) && isset($elements['workflow']['upload']) && isset($elements['workflow']['upload']['#weight'])
    && isset($elements['workflow']['itweak_upload_collapse'])) {
    $elements['workflow']['itweak_upload_collapse']['#weight'] = $elements['workflow']['upload']['#weight'] + 0.001;
	unset($elements['#sorted']);
    unset($elements['workflow']['#sorted']);
  }
  // Hide "Image attachments on comments" setting - it is ignored anyway.
  unset($elements['comment']['comment_upload_images']);
  return $elements;
}

/**
 * Pre-render callback to adjust weights of fields.
 */
function _itweak_upload_admin_settings_alter_weights($elements) {
  if (isset($elements['settings_general']) && isset($elements['settings_general']['#weight']) 
    && isset($elements['itweak_upload'])) {
	$elements['itweak_upload']['#weight'] = $elements['settings_general']['#weight'] + 0.001;
	unset($elements['#sorted']);
  }
  return $elements;
}

/**
 * Administration settings form.
 *
 * @return
 *   The completed form definition.
 */
function itweak_upload_admin_settings() {
  $form = array();
  _itweak_upload_admin_settings($form);
  return system_settings_form($form);
}

/**
 * Administration settings form worker code.
 */
function _itweak_upload_admin_settings(&$form) {
  $link_options = _itweak_upload_setting_link_options();
  $image_options = _itweak_upload_setting_image_options();
  $image_option_default = _itweak_upload_encode_derivative();

  $form['itweak_upload'] = array(
    '#type' => 'fieldset',
    '#title' => t('Attachments display'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('These global settings apply to the display of attachments in all site content by default. Each content type can override these settings.'),
  );
  // Register our own form after_build function to order our fieldset
//  if (!isset($form['#pre_render'])) $form['#pre_render'] = array();
  $form['#pre_render'][] = '_itweak_upload_admin_settings_alter_weights';

  $form['itweak_upload']['itweak_upload_progress_indicator'] = array(
    '#type' => 'radios',
    '#title' => t('Progress indicator'),
    '#options' => array(
      'bar' => t('Bar with progress meter'),
      'throbber' => t('Throbber'),
    ),
    '#default_value' => variable_get('itweak_upload_progress_indicator', 'bar'),
    '#description' => t('Your server supports upload progress capabilities. The "throbber" display does not indicate progress but takes up less room on the form, you may want to use it if you\'ll only be uploading small files or if experiencing problems with the progress bar.'),
    '#weight' => 5,
    '#access' => itweak_upload_progress_implementation(),
  );

  $form['itweak_upload']['itweak_upload_thumbnail_preset_default'] = array(
    '#type' => 'select',
    '#title' => t('Default preset for image thumbnail'),
    '#default_value' => variable_get('itweak_upload_thumbnail_preset_default', $image_option_default),
    '#options' => $image_options,
    '#description' => t('This preset will be used by default for image thumbnails. This setting can be overridden on each content type configuration page.')
      . (user_access('administer imagecache')
        ? ' ' . t('Presets can be edited on <a href="!link">ImageCache</a> page.',
            array('!link' => '/admin/build/imagecache'))
        : ''),
  );
  $form['itweak_upload']['itweak_upload_thumbnail_link_default'] = array(
    '#type' => 'select',
    '#title' => t('Default thumbnail link open mode'),
    '#default_value' => variable_get('itweak_upload_thumbnail_link_default', _itweak_upload_setting_link_default()),
    '#options' => $link_options,
    '#description' => t('This mode will be used by default for opening image thumbnails. This setting can be overridden on each content type configuration page.'),
  );
  $form['itweak_upload']['itweak_upload_thumbnail_title_default'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show file name on the opened thumbnail link'),
    '#default_value' => variable_get('itweak_upload_thumbnail_title_default', 1),
    '#description' => t('Check to show by default the file name on the image opened from thumbnails (only if selected link open mode supports it).')
  );
}

/**
 * Node type settings form.
 */
function _itweak_upload_node_type_form(&$form) {
  $node_type = (isset($form['old_type']) && isset($form['old_type']['#value'])) ? $form['old_type']['#value'] : NULL;

  $display_options = array(
//    '_default' => t('(Site default)'),
    0 => t('Disabled'),
    1 => t('Show files list (images included as files)'),
    2 => t('Show files list with image thumbnails'),
    3 => t('Show files list and image gallery'),
    4 => t('Show image gallery only')
  );

  $image_options = _itweak_upload_setting_image_options( t('(Site default)') );
  $image_option_default = '_default';

  $link_options = _itweak_upload_setting_link_options(t('(Site default)'));
    
  if (isset($form['workflow'])) {
    $form['workflow']['itweak_upload_collapse'] = array(
      '#type' => 'radios',
      '#title' => t('Collapse state of the Attachments box in <em>!type</em>', array('!type' => $node_type)),
      '#default_value' => variable_get('itweak_upload_collapse_' . $node_type, 0),
      '#options' => array(
        t('Not collapsible'),
        t('Collapsible'),
        t('Collapsed'),
      ),
      '#description' => t('Select collapsible mode for "Attach files" box of this type.')
    );
  }

  $form['itweak_upload_display'] = array(
    '#type' => 'fieldset',
    '#title' => t('Attachments display'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('These settings apply only to the display of all attachments in <em>!type</em> content. They do not change the settings that enable or allow attachments and comments.', array('!type' => $node_type)),
  );
  // Register our own form after_build function to order our fieldset
//  if (!isset($form['#pre_render'])) $form['#pre_render'] = array();
  $form['#pre_render'][] = '_itweak_upload_node_type_alter_weights';

  $form['itweak_upload_display']['itweak_upload_thumbnail_preset_default'] = array(
    '#type' => 'select',
    '#title' => t('Default preset for image thumbnail'),
    '#default_value' => variable_get('itweak_upload_thumbnail_preset_default_' . $node_type, $image_option_default),
    '#options' => $image_options,
    '#description' => t('This preset will be used by default for image thumbnails. Individual settings can be done below.')
      . (user_access('administer imagecache')
        ? ' ' . t('Presets can be edited on <a href="!link">ImageCache</a> page.',
            array('!link' => '/admin/build/imagecache'))
        : ''),
  );
  $form['itweak_upload_display']['itweak_upload_thumbnail_link_default'] = array(
    '#type' => 'select',
    '#title' => t('Default thumbnail link open mode'),
    '#default_value' => variable_get('itweak_upload_thumbnail_link_default_' . $node_type, '_default'),
    '#options' => $link_options,
    '#description' => t('This mode will be used by default for opening image thumbnails. Individual settings can be done below.'),
  );

  // Change default wordings for the rest of the form
  $image_options['_default'] = t('(Default)');
  $link_options['_default'] = t('(Default)');

  $form['itweak_upload_display']['upload'] = array(
    '#type' => 'fieldset',
    '#title' => t('Upload form'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('These settings apply to the upload attachments form in <em>!type</em> content. They do not change the settings that enable or allow attachments and comments.', array('!type' => $node_type)),
  );
  $form['itweak_upload_display']['upload']['itweak_upload_upload_preview'] = array(
    '#type' => 'checkbox',
    '#title' => t('Preview uploaded attachments while editing  <em>!type</em>', array('!type' => $node_type)),
    '#default_value' => variable_get('itweak_upload_upload_preview_' . $node_type, 1),
    '#description' => t('Check to enable attachments preview while editing nodes of this type.')
  );
  $form['itweak_upload_display']['upload']['itweak_upload_thumbnail_preset_upload'] = array(
    '#type' => 'select',
    '#title' => t('Preset for image thumbnail'),
    '#default_value' => variable_get('itweak_upload_thumbnail_preset_upload_' . $node_type, $image_option_default),
    '#options' => $image_options,
    '#description' => t('This preset will be used for image thumbnails.')
      . (user_access('administer imagecache')
        ? ' ' . t('Presets can be edited on <a href="!link">ImageCache</a> page.',
            array('!link' => '/admin/build/imagecache'))
        : ''),
  );
  $form['itweak_upload_display']['upload']['itweak_upload_thumbnail_link_upload'] = array(
    '#type' => 'select',
    '#title' => t('Thumbnail link open mode'),
    '#default_value' => variable_get('itweak_upload_thumbnail_link_upload_' . $node_type, '_default'),
    '#options' => $link_options,
    '#description' => t('This mode will be used for opening image thumbnails.'),
  );

  $form['itweak_upload_display']['node'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node view'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('These settings apply to the attachments display in <em>!type</em> content.', array('!type' => $node_type)),
  );
  $form['itweak_upload_display']['node']['itweak_upload_node_display'] = array(
    '#type' => 'radios',
    '#title' => t('Attachments display in <em>!type</em>', array('!type' => $node_type)),
    '#default_value' => variable_get('itweak_upload_node_display_' . $node_type, 2),
    '#options' => $display_options,
    '#description' => t('Select display mode for attached files and images for nodes of this type.')
  );
  $form['itweak_upload_display']['node']['itweak_upload_thumbnail_preset_node'] = array(
    '#type' => 'select',
    '#title' => t('Preset for image thumbnail'),
    '#default_value' => variable_get('itweak_upload_thumbnail_preset_node_' . $node_type, $image_option_default),
    '#options' => $image_options,
    '#description' => t('This preset will be used for image thumbnails.')
      . (user_access('administer imagecache')
        ? ' ' . t('Presets can be edited on <a href="!link">ImageCache</a> page.',
            array('!link' => '/admin/build/imagecache'))
        : ''),
  );
  $form['itweak_upload_display']['node']['itweak_upload_thumbnail_link_node'] = array(
    '#type' => 'select',
    '#title' => t('Thumbnail link open mode'),
    '#default_value' => variable_get('itweak_upload_thumbnail_link_node_' . $node_type, '_default'),
    '#options' => $link_options,
    '#description' => t('This mode will be used for opening image thumbnails.'),
  );

  $form['itweak_upload_display']['teaser'] = array(
    '#type' => 'fieldset',
    '#title' => t('Teaser view'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('These settings apply to the attachments display in <em>!type</em> teaser.', array('!type' => $node_type)),
  );
  $form['itweak_upload_display']['teaser']['itweak_upload_teaser_display'] = array(
    '#type' => 'radios',
    '#title' => t('Attachments display in <em>!type</em> teaser', array('!type' => $node_type)),
    '#default_value' => variable_get('itweak_upload_teaser_display_' . $node_type, 0),
    '#options' => $display_options,
    '#description' => t('Select display mode for attached files and images in teaser for nodes of this type.')
  );
  $form['itweak_upload_display']['teaser']['itweak_upload_teaser_images'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of images shown in <em>!type</em> teaser gallery', array('!type' => $node_type)),
    '#default_value' => variable_get('itweak_upload_teaser_images_' . $node_type, 0),
    '#size' => 5,
    '#maxlength' => 2,
    '#description' => t('The maximum number of images to show in the node teaser. Only applies when <em>Gallery</em> image display is selected. 0 will not show images. Leave blank to show all images.'),
  );
  $form['itweak_upload_display']['teaser']['itweak_upload_thumbnail_preset_teaser'] = array(
    '#type' => 'select',
    '#title' => t('Preset for image thumbnail'),
    '#default_value' => variable_get('itweak_upload_thumbnail_preset_teaser_' . $node_type, $image_option_default),
    '#options' => $image_options,
    '#description' => t('This preset will be used for image thumbnails.')
      . (user_access('administer imagecache')
        ? ' ' . t('Presets can be edited on <a href="!link">ImageCache</a> page.',
            array('!link' => '/admin/build/imagecache'))
        : ''),
  );
  $form['itweak_upload_display']['teaser']['itweak_upload_thumbnail_link_teaser'] = array(
    '#type' => 'select',
    '#title' => t('Thumbnail link open mode'),
    '#default_value' => variable_get('itweak_upload_thumbnail_link_teaser_' . $node_type, '_default'),
    '#options' => $link_options,
    '#description' => t('This mode will be used for opening image thumbnails.'),
  );

  if (module_exists('comment_upload')) {
    $form['itweak_upload_display']['comment'] = array(
      '#type' => 'fieldset',
      '#title' => t('Comments'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#description' => t('These settings apply to the attachments display in <em>!type</em> comments.', array('!type' => $node_type)),
    );
    if (module_exists('nodecomment')) {
      $form['itweak_upload_display']['comment']['#description'] .= '<br>'
	   . t('(Applicable only to normal Drupal comments - provided by core comment module. If Nodecomment module is selected for comments, attachments should be configured in the corresponding content type.)');
    }
    $form['itweak_upload_display']['comment']['itweak_upload_comment_display'] = array(
      '#type' => 'radios',
      '#title' => t('Attachments display in <em>!type</em> comments', array('!type' => $node_type)),
      '#default_value' => variable_get('itweak_upload_comment_display_' . $node_type, 2),
      '#options' => $display_options,
      '#description' => t('Select display mode for attached files and images in comments for nodes of this type.')
    );
    $form['itweak_upload_display']['comment']['itweak_upload_thumbnail_preset_comment'] = array(
      '#type' => 'select',
      '#title' => t('Preset for image thumbnail'),
      '#default_value' => variable_get('itweak_upload_thumbnail_preset_comment_' . $node_type, $image_option_default),
      '#options' => $image_options,
      '#description' => t('This preset will be used for image thumbnails.')
        . (user_access('administer imagecache')
          ? ' ' . t('Presets can be edited on <a href="!link">ImageCache</a> page.',
              array('!link' => '/admin/build/imagecache'))
          : ''),
    );
    $form['itweak_upload_display']['comment']['itweak_upload_thumbnail_link_comment'] = array(
      '#type' => 'select',
      '#title' => t('Thumbnail link open mode'),
      '#default_value' => variable_get('itweak_upload_thumbnail_link_comment_' . $node_type, '_default'),
      '#options' => $link_options,
      '#description' => t('This mode will be used for opening image thumbnails.'),
    );
  }
}

