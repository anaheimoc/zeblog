<?php
// $Id: radioactivity_node-admin-ui.inc,v 1.1.2.3 2010/06/21 11:44:49 skiminki Exp $
/**
 * @file
 * Node radioactivity admin UI
 */

function radioactivity_node_admin_form() {
  $form=array();

  $form['radioactivity_node_click_duration']=
    array('#type' => 'textfield',
          '#title' => t('Click duration per user per node (in seconds), set 0 to disable'), 
          '#description' => t('This is abuse control. This setting determines the amount of time to store click information '.
                              'per user. This is used to prevent giving energy to node when user reloads page. By setting '.
                              'this to 600 (= 10 minutes), user can give energy to specific node only once per 10 minutes. Higher values '.
                              'require more space in database tables, while lower values allow single user to give energy more '.
                              'often to a specific node. The old click info is cleaned in cron runs.'),
          '#size' => 10,
          '#required' => TRUE,
          '#default_value' => _radioactivity_node_get_click_duration());

  $form['radioactivity_node_anon_hook_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Anonymous node view callback mode'),
    '#default_value' => variable_get('radioactivity_node_anon_hook_mode', 'normal'),
    '#options' => array('normal' => t('Normal'), 'ajax' => t('AJAX'), 'external' => t('External')),
    '#description' =>
    t('This setting controls how node view information is collected.').
    '<ul><li>'.
    t('<em>Normal</em> mode uses the standard Drupal page generation process (by hook_exit) and works with page cache modes <em>Disabled</em> '.
      'and <em>Normal</em>. No further configuration is needed. This should be used if aggressive caching or external cache providers are not used.').
    '</li><li>'.
    t('<em>AJAX</em> mode inserts JavaScript code and an IMG-tag in the HTML. This makes browsers generate an extra call per page view to '.
      'collect the information. '.
      'This mode should work with all page cache modes and with static page cache providers such as Boost and Varnish. You have to enable '.
      'block "Radioactivity Node: AJAX update" which contains the dynamic callback.').
    '</li><li>'.
    t('<em>External</em> mode disables information collection by Radioactivity Node altogether. Use this '.
      'if you obtain the information by, e.g., parsing accelerator logs. Only for very busy sites.').
    '</li></ul>'.
    t('<strong>Note</strong>: Every time you change this setting the page caches must be cleared.'));

  return system_settings_form($form);
}

