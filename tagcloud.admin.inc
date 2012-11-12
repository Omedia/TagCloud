<?php

/*
 * Implements hook_menu()
 */
function tagcloud_block_block_menu() {
  $items['admin/structure/block/add-tagcloud-block'] = array(
    'title'            => 'Add Tagcloud block',
    'description'      => 'Adds a new tagcloud block.',
    'page callback'    => 'drupal_get_form',
    'page arguments'   => array('tagcloud_add_block_form'),
    'access arguments' => array('administer blocks'),
    'type'             => MENU_LOCAL_ACTION,
  );
  
  $default_theme = variable_get('theme_default', 'bartik');
  foreach (list_themes() as $key => $theme) {
    if ($key != $default_theme) {
      $items['admin/structure/block/list/' . $key . '/add-tagcloud-block'] = array(
        'title'            => 'Add Tagcloud block',
        'description'      => 'Adds a new tagcloud block.',
        'page callback'    => 'drupal_get_form',
        'page arguments'   => array('tagcloud_add_block_form'),
        'access arguments' => array('administer blocks'),
        'type'             => MENU_LOCAL_ACTION,
      );
    }
  }
  
  $items['admin/structure/block/delete-tagcloud-block'] = array(
    'title'            => 'Delete Tagcloud block',
    'page callback'    => 'drupal_get_form',
    'page arguments'   => array('tagcloud_block_delete'),
    'access arguments' => array('administer blocks'),
    'type'             => MENU_CALLBACK,
  );
  
  return $items;
}

/**
 * Menu callback for add tagcloud block form
 */
function tagcloud_block_add_block_form() {
  module_load_include('inc', 'block', 'block.admin');
  $form = array();
  $form = block_admin_configure($form, $form_state, 'tagcloud_block', NULL);
  $form_state['build_info']['base_form_id'] = 'block_add_block_form';
  $form['#validate'] = array();
  $form['#submit'] = array('tagcloud_block_add_block_form_submit');
  return $form;
}

/**
 * Implements hook_block_configure().
 */
function tagcloud_block_block_configure_block($form, &$form_state) {
  /* get delta */
  if (!empty($form_state['values']['delta'])) {
    $block_data = _tagcloud_get_block_data($form_state['values']['delta']);
  }
  /* get all vocabularies */
  $vocabularies = taxonomy_get_vocabularies();
  $vocabularies = array_map('_tagcloud_vocabularies_callback', $vocabularies);
  $form['vocabulary'] = array (
    '#type'           =>  'select',
    '#title'          =>  t('Choose vocabulary to generate tagcloud from'),
    '#options'        =>  $vocabularies,
    '#default_value'  =>  isset($block_data['data']['vocabulary']) ? $block_data['data']['vocabulary'] : 1,
  );
  
  $form['tags_limit'] = array (
    '#type'           =>  'textfield',
    '#title'          =>  t('Specify the limit for tags or 0 for no limit'),
    '#default_value'  =>  isset($block_data['data']['tags_limit']) ? $block_data['data']['tags_limit'] : 0,
  );
  
  $form['min_font_size'] = array (
    '#type'           =>  'textfield',
    '#title'          =>  t('Minimum font size'),
    '#default_value'  =>  isset($block_data['data']['min_font_size']) ? $block_data['data']['min_font_size'] : 10,
  );
  
  $form['max_font_size'] = array (
    '#type'           =>  'textfield',
    '#title'          =>  t('Maximum font size'),
    '#default_value'  =>  isset($block_data['data']['max_font_size']) ? $block_data['data']['max_font_size'] : 35,
  );
  
  return $form;
}
  
  /**
   * callback function for mapping tags array
   */
  function _tagcloud_vocabularies_callback($item) {
    return $item->name;
  }
  

/**
 * Generate basic config array
 */
function tagcloud_block_block_get_config($delta = '') {
  return array('delta' => $delta);
}


/**
 * hook_block_configure().
 */
function tagcloud_block_block_configure($delta = '') {
  $form_state = array('values' => tagcloud_block_get_config($delta));
  return tagcloud_block_configure_block(array(), $form_state);
}

/*
 * add block form submit callback
 */
function tagcloud_block_block_add_block_form_submit($form, &$form_state) {
  $block_ids = variable_get('tagcloud_block_ids', array());
  $delta = empty($block_ids) ? 1 : max($block_ids) + 1;
  $form_state['values']['delta'] = 'tagcloud_'.$delta;
  $block_ids[] = $delta;
  variable_set('tagcloud_block_ids', $block_ids);
  
  tagcloud_block_save('tagcloud_'.$delta, $form_state['values']);

  // Run the normal new block submission (borrowed from block_add_block_form_submit).
  $query = db_insert('block')->fields(array('visibility', 'pages', 'custom', 'title', 'module', 'theme', 'region', 'status', 'weight', 'delta', 'cache'));
  foreach (list_themes() as $key => $theme) {
    if ($theme->status) {
      $region = !empty($form_state['values']['regions'][$theme->name]) ? $form_state['values']['regions'][$theme->name] : BLOCK_REGION_NONE;
      $query->values(array(
        'visibility' => (int) $form_state['values']['visibility'],
        'pages' => trim($form_state['values']['pages']),
        'custom' => (int) $form_state['values']['custom'],
        'title' => $form_state['values']['title'],
        'module' => $form_state['values']['module'],
        'theme' => $theme->name,
        'region' => ($region == BLOCK_REGION_NONE ? '' : $region),
        'status' => 0,
        'status' => (int) ($region != BLOCK_REGION_NONE),
        'weight' => 0,
        'delta' => 'tagcloud_'.$delta,
        'cache' => DRUPAL_NO_CACHE,
      ));
    }
  }
  $query->execute();

  $query = db_insert('block_role')->fields(array('rid', 'module', 'delta'));
  foreach (array_filter($form_state['values']['roles']) as $rid) {
    $query->values(array(
      'rid' => $rid,
      'module' => $form_state['values']['module'],
      'delta' => $delta,
    ));
  }
  $query->execute();

  drupal_set_message(t('The block has been created.'));
  cache_clear_all();
  $form_state['redirect'] = 'admin/structure/block';
}


/**
 * hook_block_save().
 */
function tagcloud_block_block_save($delta = '', $edit = array()) {
  _tagcloud_block_save($delta, $edit);
}


/**
 * Implements hook_block_save().
 */
function _tagcloud_block_save($delta = '', $values = array()) {
  if (!empty($delta)) {
    variable_set($delta, array('data' => $values));
  }
}

/**
 * Block delete menu callback
 */
function tagcloud_block_block_delete($form, &$form_state, $delta = 0) {
  $block_data = _tagcloud_get_block_data('tagcloud_'.$delta);
  $title = block_load('tagcloud_block', 'tagcloud_'.$delta);
  $title = $title->title;
  $form['block_title'] = array('#type' => 'hidden', '#value' => $title);
  $form['delta'] = array('#type' => 'hidden', '#value' => $delta);

  return confirm_form($form, t('Are you sure you want to delete the "%name" block?', array('%name' => $title)), 'admin/structure/block', NULL, t('Delete'), t('Cancel'));
}

/**
 * Deletion of tagcloud blocks.
 */
function tagcloud_block_block_delete_submit($form, &$form_state) {
  /* delete our variables */
  $delta = $form_state['values']['delta'];
  $block_ids = variable_get('tagcloud_block_ids', array());
  unset($block_ids[array_search($delta, $block_ids)]);
  sort($block_ids);
  variable_set('tagcloud_block_ids', $block_ids);
  variable_del('tagcloud_'.$delta);

  db_delete('block')
    ->condition('module', 'tagcloud_block')
    ->condition('delta', $delta)
    ->execute();
  db_delete('block_role')
    ->condition('module', 'tagcloud_block')
    ->condition('delta', $delta)
    ->execute();
  drupal_set_message(t('The block "%name" has been removed.', array('%name' => $form_state['values']['block_title'])));
  cache_clear_all();
  $form_state['redirect'] = 'admin/structure/block';
  return;
}


/**
 * Alters the block admin form to add delete links next to menu blocks.
 */
function _tagcloud_form_block_admin_display_form_alter(&$form, $form_state) {
  $block_ids = variable_get('tagcloud_block_ids', array());
  foreach ($block_ids as $key => $delta) {
    if (!empty($form['blocks']['tagcloud_tagcloud_'.$delta])) {
      $form['blocks']['tagcloud_tagcloud_'.$delta]['delete'] = array('#type' => 'link', '#title' => t('delete'), '#href' => 'admin/structure/block/delete-tagcloud-block/' . $delta);
    }
  }
}

/**
 * blocks listing form alter function
 */
function tagcloud_block_form_block_admin_display_form_alter(&$form, $form_state) {
  return _tagcloud_form_block_admin_display_form_alter($form, $form_state);
}
