<?php
/**
 * @file
 * Event connector module administrator forms.
 */

/**
 * Module field mapping settings form.
 */
function event_connector_mapping() {
  $form = array();

  $our_event_type = variable_get('event_connector_type', '0');

  $types = node_type_get_types();
  $content_types[0] = t('Choose');
  foreach ($types as $type) {
    $content_types[$type->type] = $type->name;
  }
  $form['event_connector_type'] = array(
    '#type' => 'select',
    '#title' => t('Event content type'),
    '#options' => $content_types,
    '#default_value' => $our_event_type,
    '#description' => t('Select your event content type to map.'),
    '#disabled' => $our_event_type == '0' ? FALSE : TRUE,
  );

  if ($our_event_type != '0') {
    $fields = field_info_instances('node', $our_event_type);

    $items[0] = t('No mapping');
    $items['title'] = $types[$our_event_type]->title_label;
    foreach ($fields as $key => $value) {
      // dpm($key);
      if ($value['widget']['type'] == 'date_popup') {
        $items[$key . '-value-date'] = $value['label'] . ':value';
        $items[$key . '-value2-date'] = $value['label'] . ':value2';
      }
      elseif ($value['widget']['module'] == 'taxonomy') {
        $items[$key . '-tid-term'] = $value['label'];
      }
      elseif ($value['widget']['module'] == 'image') {
        $items[$key . '-fid-image'] = $value['label'];
        $items[$key . '-alt'] = $value['label'] . ':alt';
      }
      else {
        $items[$key] = $value['label'];
      }
    }
    $items['published'] = t('Published');

    $defs = variable_get('event_connector_mappings', array());
    event_connector_field_mappings($form, $items, $defs);
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Implements hook_submit().
 */
function event_connector_mapping_submit($form, &$form_state) {
  $values = $form_state['values'];
  if ($values['op'] == $values['submit']) {
    $res = array();
    foreach ($values as $level => $arr) {
      if ($level == 'event_connector_type') {
        variable_set('event_connector_type', $arr);
        continue;
      }
      if (strpos($level, 'level_') === FALSE) {
        continue;
      }
      $res_inner = array();
      foreach ($arr as $key => $value) {
        if (strpos($key, 'event_connector') === 0 && !empty($value)) {
          $res_inner[$key] = $value;
        }
      }
      $res[$level] = $res_inner;
    }
    variable_set('event_connector_mappings', $res);
  }
}

/**
 * Module settings form.
 */
function event_connector_settings() {
  $form = array();

  $form['event_connector_apiurl'] = array(
    '#type' => 'textfield',
    '#title' => t('URL to API'),
    '#default_value' => variable_get('event_connector_apiurl', ''),
    '#description' => t('URL to Event Calendar API.'),
    '#required' => TRUE,
  );

  $form['event_connector_apikey'] = array(
    '#type' => 'textfield',
    '#title' => t('Apikey'),
    '#default_value' => variable_get('event_connector_apikey', ''),
    '#description' => t('Apikey to Event Calendar API.'),
    '#required' => TRUE,
  );

  $form['event_connector_test'] = array(
    '#type' => 'checkbox',
    '#title' => t('Test content.'),
    '#default_value' => variable_get('event_connector_test', FALSE),
  );

  $form['event_local_test'] = array(
    '#type' => 'checkbox',
    '#title' => t('Test content locally.'),
    '#description' => t('When checked no content is sended. Query is printed to screen.'),
    '#default_value' => variable_get('event_local_test', TRUE),
  );

  $form['event_conn_log'] = array(
    '#type' => 'checkbox',
    '#title' => t('Log messages.'),
    '#description' => t('Send message log for debug reasons.'),
    '#default_value' => variable_get('event_conn_log', FALSE),
  );

  return system_settings_form($form);
}

/**
 * Page to show list get from api.
 */
function event_connector_list_page() {
  $collapsed = FALSE;
  // Area section.
  $form['ec_area'] = array(
    '#type' => 'fieldset',
    '#title' => t('Event areas'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );
  $area_rows = _event_connector_fetch_list('areas/list');
  $area_header = array(t('Id'), t('Areas'));
  $form['ec_area']['areas'] = array(
    '#type' => 'item',
    '#title' => t('Areas'),
    '#markup' => theme('table', array('header' => $area_header, 'rows' => $area_rows)),
  );
  $form['ec_area']['area_update'] = array(
    '#type' => 'submit',
    '#value' => t('Update Areas'),
    '#submit' => array('event_connector_area_list_submit'),
  );
  // Category section.
  $form['ec_category'] = array(
    '#type' => 'fieldset',
    '#title' => t('Event categories'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );
  $category_rows = _event_connector_fetch_list('categories/list');
  $category_header = array(t('Id'), t('Categories'));
  $form['ec_category']['categories'] = array(
    '#type' => 'item',
    '#title' => t('Categories'),
    '#markup' => theme('table', array('header' => $category_header, 'rows' => $category_rows)),
  );
  $form['ec_category']['categories_update'] = array(
    '#type' => 'submit',
    '#value' => t('Update Categories'),
    '#submit' => array('event_connector_category_list_submit'),
  );
  // Target section.
  $form['ec_target'] = array(
    '#type' => 'fieldset',
    '#title' => t('Event target groups'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );
  $target_rows = _event_connector_fetch_list('targetgroups/list');
  $target_header = array(t('Id'), t('Target groups'));
  $form['ec_target']['targetgroups'] = array(
    '#type' => 'item',
    '#title' => t('Target groups'),
    '#markup' => theme('table', array('header' => $target_header, 'rows' => $target_rows)),
  );
  $form['ec_target']['target_update'] = array(
    '#type' => 'submit',
    '#value' => t('Update Target groups'),
    '#submit' => array('event_connector_target_list_submit'),
  );

  return $form;
}

/**
 * Submit function updating area list from event calendar api.
 */
function event_connector_area_list_submit($form, &$form_state) {
  event_connector_update_taxonomy(EVENT_CONNECTOR_AREA_VOC, 'areas/list');
  drupal_set_message('Areas updated.');
}

/**
 * Submit function updating category list from event calendar api.
 */
function event_connector_category_list_submit($form, &$form_state) {
  event_connector_update_taxonomy(EVENT_CONNECTOR_CATEGORY_VOC, 'categories/list');
  drupal_set_message('Categories updated.');
}

/**
 * Submit function updating target group list from event calendar api.
 */
function event_connector_target_list_submit($form, &$form_state) {
  event_connector_update_taxonomy(EVENT_CONNECTOR_TARGET_GROUP_VOC, 'targetgroups/list');
  drupal_set_message('Target groups updated.');
}
