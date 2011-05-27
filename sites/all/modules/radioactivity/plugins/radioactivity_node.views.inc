<?php
// $Id: radioactivity_node.views.inc,v 1.1.2.4 2010/01/24 21:32:51 skiminki Exp $
/**
 * @file
 * Node radioactivity views support
 */

function radioactivity_node_views_data() {
  require_once drupal_get_path('module', 'radioactivity') .'/radioactivity.inc';

  $data=array();

  foreach (radioactivity_get_decay_profiles() as $dpid => $decay_profile) {
    // s = sort, f = field
    foreach (array('s', 'f') as $suffix) {
      $table_alias='radioactivity_node_'. $dpid .'_'. $suffix;
      $data[$table_alias]['table']['group']=t('Radioactivity (@e)', array('@e' => $decay_profile['label']));
      $data[$table_alias]['table']['title']=t('Node energy');
      $data[$table_alias]['table']['help']=t('Node radioactive energy in decay profile @e.', array('@e' => $decay_profile['label']));

      // join to node
      $data[$table_alias]['table']['join']['node'] =
        array('left_field' => 'nid',
              'field' => 'id',
              'table' => 'radioactivity',
              'extra' => array(array('field' => 'class',
                                     'value' => 'node'),
                               array('field' => 'decay_profile',
                                     'value' => $dpid,
                                     'numeric' => TRUE)));

      // energy field
      $data[$table_alias]['energy']=
        array('title' => t('Node energy'),
              'help' => t('The radioactive energy of the node.'));
      
      switch ($suffix) {
        case 'f':
          $data[$table_alias]['energy']['field'] =
            array('handler' => 'views_handler_field');
          break;
        case 's':
          $data[$table_alias]['energy']['sort'] =
            array('handler' => 'radioactivity_node_views_handler_sort_left_or_inner');
          break;
      }
    }
  }
  return $data;
}
