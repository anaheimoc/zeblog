<?php
// $Id: radioactivity-admin-ui.inc,v 1.1.2.7 2010/01/24 21:32:51 skiminki Exp $
/**
 * @file
 * Radioactivity core admin UI.
 */

function radioactivity_admin_general_form() {

  $form=array();

  $form['radioactivity_decay_granularity']=
    array('#type' => 'textfield',
          '#title' => t('Decay granularity (in seconds)'), 
          '#description' => t('This setting determines how often at most the radioactivity is decreased by the decay formula. '.
                              'The shorter the time, the more accurate the modeling will be, but the more database '.
                              'activity is required. The default (10 minutes) should be good starting point.'),
          '#size' => 10,
          '#required' => TRUE,
          '#default_value' => _radioactivity_get_decay_granularity());


  $form['memcached']=array('#type' => 'fieldset',
                           '#tree' => FALSE,
                           '#title' => t('Memcached acceleration'));


  $memcached_ok=FALSE;
  $mc_status=radioactivity_determine_memcached_availability();
  $memcached_ok=$mc_status>0;
  $memcached_availability_text=_radioactivity_get_memcached_availability_string($mc_status);


  $form['memcached']['availability']=
    array('#type' => 'item',
          '#title' => t('Memcached status'),
          '#value' => $memcached_availability_text);


  if ($memcached_ok) {
    $form['memcached']['radioactivity_memcached_enable']=
      array('#type' => 'checkbox',
            '#title' => t('Memcached acceleration'),
            '#description' => t('If this option is enabled, energy additions are deferred by saving an '.
                                'entry with minimal information to memcached bin <em>radioactivity</em>. '.
                                'These entries are batch processed in cron runs.'),
            '#default_value' => radioactivity_get_memcached_enable());

    $form['memcached']['radioactivity_memcached_expiration']=
      array('#type' => 'textfield',
            '#title' => t('Memcached entry expiration time (in seconds)'),
            '#description' => t('Expiration time for memcached entries used by radioactivity. This should be at least twice as long as your maximum '.
                                'cron interval.'),
            '#size' => 10,
            '#required' => TRUE,
            '#default_value' => radioactivity_get_memcached_expiration());
  }

  return system_settings_form($form);
}

function radioactivity_admin_profile_list() {
  $form=array();

  $decay_profiles=radioactivity_get_decay_profiles();
  
  $profile_rows=array();
  if (count($decay_profiles)) {
    foreach ($decay_profiles as $dpid => $decay_profile) {
      $profile_rows[]=array('data' => array($dpid, check_plain($decay_profile["label"]),
                                            '<a href="'. url('admin/settings/radioactivity/edit_profile/'. $dpid)
                                            .'">'. t("Edit") .'</a>'));
    }
  }
  else {
    $profile_rows[]=array('data' => array(array('data' => t('No profiles defined'), 'colspan' => 3)));
  }

  $profiles_table=theme('table',
                        array(t('Id'), t('Label'), t('Actions')),
                        $profile_rows);

  $form['profiles_table']=array('#value' => $profiles_table);

  return $form;
}

function _radioactivity_oclassdef_to_form($oclass, $name, $def, $sources, $energy, $level=0) {
  $form=array('#type' => 'fieldset',
              '#tree' => TRUE,
              '#collapsible' => TRUE,
              '#title' => t('Energy settings for @oclass', array('@oclass' => $name)));
  $collapsed=TRUE;

  if (count($sources)==0) {
    $form['no_sources']=
      array('#type' => 'item',
            '#value' => t('You must enable at least one plug-in that provides an energy source for this target class.'));
  }

  if ($level==0 && is_array($def['subclasses'])) {
    $form[]=array('#type' => 'item',
                  '#value' => t('The default settings for #type.',
                                array('#type' => $name)));
  }
  elseif ($level>0) {
    $form[]=array('#type' => 'item',
                  '#value' => t('Specific settings for #type. Empty field uses setting from parent.',
                                array('#type' => $name)));
  }

  foreach ($sources as $source => $sdef) {
    $energy_value=$energy[$source];
    $form[$source]=
      array('#type' => 'textfield',
            '#title' => t('Incident energy from %s', array('%s' => $sdef['title_placeholder'])),
            '#default_value' => $energy_value);
    if (isset($sdef['description'])) $form[$source]['#description']=t($sdef['description']);
    if (strlen($energy_value)>0) $collapsed=FALSE;
  }

  if (is_array($def['subclasses'])) {
    foreach ($def['subclasses'] as $subclass => $subclassdef) {
      $form['subclasses'][$subclass]=_radioactivity_oclassdef_to_form($subclass, $name .' / '. $subclass, $subclassdef, $sources,
                                                                      $energy['subclasses'][$subclass], $level+1);
      if (!$form['subclasses'][$subclass]['#collapsed']) $collapsed=FALSE;
    }
  }

  $form['#collapsed']=$collapsed;

  return $form;

}

function radioactivity_admin_profile_form($form_id, $dpid) {
  $form=array();

  $form[]=
    array('#type' => 'item',
          '#title' => t('Profile id'),
          '#value' => $dpid>0? $dpid : t('Unassigned'));

  $form['decay_profile_id']=
    array('#type' => 'hidden',
          '#value' => $dpid );

  // sanity check for $dpid
  $bad_dpid=FALSE;
  if (!isset($dpid) || !is_numeric($dpid) || ($dpid!=-1 && $dpid<1)) {
    $bad_dpid=TRUE;
  }

  if (!$bad_dpid) {
    if ($dpid>0) {
      $decay_profiles=radioactivity_get_decay_profiles();
      if (isset($decay_profiles[$dpid])) {
        $decay_profile=$decay_profiles[$dpid];
      }
      else {
        $bad_dpid=TRUE;
      }
      unset($decay_profiles);
    }
    else {
      // defaults for new
      $decay_profile=array('half_life' => 6*3600,
                           'cut_off_energy' => 0.5,
                           'energy' => array('node' => array('view' => 1)));
    }
  }

  if ($bad_dpid) {
    drupal_goto('admin/settings/radioactivity/list_profiles');
  }

  $form['label']=
    array('#type' => 'textfield',
          '#title' => t('Profile label'),
          '#required' => TRUE,
          '#description' => t('The profile label. Used in views, links, etc'),
          '#default_value' => $decay_profile['label']);

  $form['description']=
    array('#type' => 'textarea',
          '#title' => t('Description'),
          '#description' => t('The description of the profile.'),
          '#default_value' => $decay_profile['description']);

  $form['half_life']=
    array('#type' => 'textfield',
          '#title' => t('Half-life of the radioactivity in seconds'),
          '#required' => TRUE,
          '#description' => t('Determines the decay rate of the radioactivity. For exaple, if the decay rate is '.
                              '3600 (one hour), the radioactivity halves once an hour. If it is now 1000, it will '.
                              'be 500 after an hour, 250 after two hours, and so on. The default is 6 hours.'),
          '#default_value' => $decay_profile['half_life']);

  $form['cut_off_energy']=
    array('#type' => 'textfield',
          '#title' => t('Cut-off energy'),
          '#required' => TRUE,
          '#description' => t('The cut-off energy. Below this energy level, the node is considered non-radioactive and '.
                              'the radioactivity information will be deleted from the database. Leave 0 disable cut-off.'),
          '#default_value' => $decay_profile['cut_off_energy']);

  $radioactivity_info=radioactivity_get_radioactivity_info();

  //  $form['debug']=
  //    array('#value' => print_r($radioactivity_info, TRUE));

  $form['energy']=
    array('#type' => 'fieldset',
          '#tree' => TRUE,
          '#title' => t('Energy settings'));

  if (count($radioactivity_info["targets"])==0) {
    // no energy target classes
    $form['energy']['no_targets']=
      array('#type' => 'item',
            '#value' => t('You must enable at least one plug-in that provides an energy target class. '.
                          'Try <em>radioactivity_node</em>.'));
  }
  else {
    foreach ($radioactivity_info['targets'] as $oclass => $def) {
      $form['energy'][$oclass]=
        _radioactivity_oclassdef_to_form($oclass, $oclass, $def, $radioactivity_info['sources'][$oclass],
                                         $decay_profile['energy'][$oclass]);
      $form['energy'][$oclass]['#collapsed']=FALSE;
    }
  }

  $form['buttons']['submit'] = array('#type' => 'submit', '#value' => t('Save decay profile') );
  if ($dpid>0) {
    $form['buttons']['delete'] = array('#value' => l(t('Delete profile'), 'admin/settings/radioactivity/delete_profile/'. $dpid));
  }
  if (!empty($_POST) && form_get_errors()) {
    drupal_set_message(t('The settings have not been saved because of the errors.'), 'error');
  }

  return $form;
}

// removes recursively empty subarrays and empty leafs
function _radioactivity_prune_array($energy) {
  $ret=array();
  if (isset($energy) && count($energy)) {
    foreach ($energy as $key => $value) {
      if (is_array($value)) {
        $value=_radioactivity_prune_array($value);
      }
      if ($value || (!is_array($value) && strlen($value))) {
        $ret[$key]=$value;
      }
    }
  }
  return $ret;
}

function radioactivity_admin_profile_form_submit($form_id, $form) {
  $form_values=$form['values'];
  $dpid=(int)$form_values['decay_profile_id'];

  if ($dpid==0) {
    // internal error
    drupal_set_message(t('Internal error: decay_profile_id=@dpid', array('@dpid' => $dpid)), 'error');
    return FALSE;
  }

  $decay_profiles=radioactivity_get_decay_profiles();

  if ($dpid<0) {
    // get new $dpid
    $dpids=array_keys($decay_profiles);
    if (count($dpids)>0) {
      $dpid=1+max($dpids);
    }
    else {
      $dpid=1;
    }
    $new_profile=TRUE;
  }
  else {
    $new_profile=FALSE;
  }

  // build profile array
  $decay_profile=array();
  $decay_profile['label']=$form_values['label'];
  $decay_profile['description']=$form_values['description'];

  $half_life=(int)$form_values['half_life'];
  if ($half_life<=0) $half_life=6*3600;
  $decay_profile['half_life']=$half_life;

  $decay_profile['cut_off_energy']=(double)$form_values['cut_off_energy'];

  $decay_profile['energy']=_radioactivity_prune_array($form_values['energy']);

  $decay_profiles[$dpid]=$decay_profile;

  _radioactivity_set_decay_profiles($decay_profiles);
  drupal_set_message(t('Profile @dpid saved.', array('@dpid' => $dpid)));

  if ($new_profile) {
    // we need a jump, as this profile's edit url just changed
    drupal_goto('admin/settings/radioactivity/edit_profile/'. $dpid);
  }
}

function radioactivity_admin_delete_profile_form($form_id, $dpid) {
  $decay_profiles=radioactivity_get_decay_profiles();

  // sanity check for dpid
  if (!isset($decay_profiles[$dpid])) {
    drupal_goto('admin/settings/radioactivity/list_profiles');
  }

  return confirm_form(array('decay_profile_id' => array('#type' => 'hidden',
                                                        '#value' => $dpid )),
                      t('Are you sure you want to delete radiation decay profile @label (@id)?',
                        array('@label' => $decay_profiles[$dpid]['label'], '@id' => $dpid)),
                      "admin/settings/radioactivity/edit_profile/". $dpid, NULL,
                      t('Delete'));
}

function radioactivity_admin_delete_profile_form_submit($form_id, $form) {
  $dpid=$form['values']['decay_profile_id'];
  drupal_set_message(t("Deleted profile @id", array('@id' => $dpid)));


  $decay_profiles=radioactivity_get_decay_profiles();
  unset($decay_profiles[$dpid]);
  _radioactivity_set_decay_profiles($decay_profiles);

  db_query("DELETE FROM {radioactivity} WHERE decay_profile=%d", $dpid);

  drupal_goto("admin/settings/radioactivity/list_profiles");
}

