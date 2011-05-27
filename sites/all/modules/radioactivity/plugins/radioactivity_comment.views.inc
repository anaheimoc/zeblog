<?php
// $Id: radioactivity_comment.views.inc,v 1.1.2.2 2010/01/24 21:32:51 skiminki Exp $
/**
 * @file
 * Comment radioactivity views support
 */

function radioactivity_comment_views_data() {
  require_once drupal_get_path('module', 'radioactivity') .'/radioactivity.inc';

  $data=array();

  foreach (radioactivity_get_decay_profiles() as $dpid => $decay_profile) {
    // s = sort, f = field
    foreach (array('s', 'f') as $suffix) {
      $table_alias='radioactivity_comment_'. $dpid .'_'. $suffix;
      $data[$table_alias]['table']['group']=t('Radioactivity (@e)', array('@e' => $decay_profile['label']));
      $data[$table_alias]['table']['title']=t('Comment energy');
      $data[$table_alias]['table']['help']=t('Comment radioactive energy in decay profile @e.', array('@e' => $decay_profile['label']));

      // join to comment
      $data[$table_alias]['table']['join']['comments'] =
        array('left_field' => 'cid',
              'field' => 'id',
              'table' => 'radioactivity',
              'extra' => array(array('field' => 'class',
                                     'value' => 'comment'),
                               array('field' => 'decay_profile',
                                     'value' => $dpid,
                                     'numeric' => TRUE)));

      // energy field
      $data[$table_alias]['energy']=
        array('title' => t('Comment energy'),
              'help' => t('The radioactive energy of a comment.'));
      
      switch ($suffix) {
        case 'f':
          $data[$table_alias]['energy']['field'] =
            array('handler' => 'views_handler_field');
          break;
        case 's':
          $data[$table_alias]['energy']['sort'] =
            array('handler' => 'radioactivity_comment_views_handler_sort_left_or_inner');
          break;
      }
  }
  }
  return $data;
}