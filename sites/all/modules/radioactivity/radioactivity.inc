<?php
// $Id: radioactivity.inc,v 1.2.4.8 2010/01/24 21:32:51 skiminki Exp $
/*
 * @file
 * Some radioactivity functions that may be called from other modules, even when the main radioactivity
 * is not loaded. For instance, radioactivity_node utilizes these functions in radioactivity_node_exit.
 */

DEFINE('RADIOACTIVITY_PERM_ADMIN', 'administer radioactivity');

// defines for memcache availability
DEFINE('RADIOACTIVITY_MEMCACHE_OK', 1);
DEFINE('RADIOACTIVITY_MEMCACHE_NO_BIN', -1);
DEFINE('RADIOACTIVITY_MEMCACHE_NO_MODULE', -2);

/**
 * Returns target object classes. Objects of target classes may receive energy.
 */
function radioactivity_get_radioactivity_info($reset_cache=FALSE) {
  static $info;

  if ($reset_cache || !isset($info)) {
    $info = array('targets' => array(), 'sources' => array());
    foreach (module_implements('radioactivity_info') as $name) {
      $function = $name .'_radioactivity_info';
      $result = $function();
      if (isset($result) && is_array($result)) {
        $info = array_merge_recursive($info, $result);
      }
    }
  }

  return $info;
}

/**
 * Retrieves decay profiles in array. The array key is decay profile id (positive int) and the value is the decay
 * profile. The profile is array as follows:
 *   "label"            -> The profile label
 *   "description       -> The profile description
 *   "half_life"        -> The radioactivity half-life (in seconds)
 */
function radioactivity_get_decay_profiles() {
  return variable_get("radioactivity_profiles", array());
}

function _radioactivity_possibly_remap_id($oid, $oclass) {
  static $map=array();
  if (!isset($map[$oclass][$oid])) {
    $info=radioactivity_get_radioactivity_info();
    $function='';
    if (isset($info['targets'][$oclass]['id_mapper'])) {
      $function=$info['targets'][$oclass]['id_mapper'];
    }
    if ($function) {
      // ok, remapper function defined for this object class. Use it to
      // map the id into database id. This is used to map string ids, such as urls,
      // into integers by some plugins (most notably arbitrary target)
      $new_oid=$function($oid, $oclass);
    }
    else {
      $new_oid=$oid;
    }
    $map[$oclass][$oid]=$new_oid;
  }
  return $map[$oclass][$oid];
}

/**
 * Resolve full class name for a specific object. This utilizes subclass resolver if defined.
 */
function _radioactivity_resolve_classname($oid, $oclass) {
  static $map=array();

  if (!isset($map[$oclass][$oid])) {
    $info=radioactivity_get_radioactivity_info();
    $function=$info['targets'][$oclass]['subclass_resolver'];
    if ($function) {
      $subclass=$function($oid, $oclass);
      if ($subclass) {
        $classname=$oclass .':'. $subclass;
      }
      else {
        $classname=$oclass;
      }
    }
    else {
      $classname=$oclass;
    }
    $map[$oclass][$oid]=$classname;
  }
  return $map[$oclass][$oid];
}

/**
 * Return energies for source action.
 * @param $oid
 * @param $oclass Base class, e.g. 'node'. Subclass will be resolved by subclass_resolver hook if necessary
 * @param $source Energy source, e.g. 'view'
 * @return array of dpid to energy, e.g, array(1 => 3.4, 2 => 5.6);
 */
function radioactivity_get_energies_for_source($oid, $oclass, $source) {
  $ret=array();
  foreach (radioactivity_get_decay_profiles() as $dpid => $decay_profile) {
    $classname=$oclass; // the default class name

    // check if we need to resolve full classname
    if (is_array($decay_profile['energy'][$oclass]['subclasses'])) {
      foreach ($decay_profile['energy'][$oclass]['subclasses'] as $subsources) {
        if (isset($subsources[$source])) {
          // there is subclass specific energy value for the source, so
          // resolve subclass
          $classname=_radioactivity_resolve_classname($oid, $oclass);
          break;
        }
      }
    }

    // resolve energy amount
    $energy=$decay_profile['energy'];
    $value=0;
    $classparts=explode(':', $classname);
 
    // Default energy is 0. Try first the base class and then possible subclass.
    // e.g. if full class name is node:story, try first $decay_profile['energy']['node'][<action>]
    // and then $decay_profile['energy']['node']['subclasses']['story'][<action>]
    // The last match is the defining match
    foreach ($classparts as $part) {
      if (!is_array($energy[$part])) break;
      $energy=$energy[$part];

      if (isset($energy[$source]))
        $value_cand=$energy[$source];
      else
        $value_cand=FALSE;

      if (strlen($value_cand)>0) $value=(double)$value_cand;

      if (isset($energy['subclasses']))
        $energy=$energy['subclasses'];
      else
        $energy=FALSE;
    }

    $ret[$dpid]=$value;
  }
  return $ret;
}

// this is the MySQL optimized version
function _radioactivity_add_energy_mysql($oid, $oclass, $dpid, $amount, $timestamp) {
  db_query("INSERT INTO {radioactivity} (id, class, decay_profile, energy, last_emission_timestamp) ".
           "VALUES (%d, '%s', %d, %f, %d) ON DUPLICATE KEY UPDATE energy=energy+%f",
           $oid, $oclass, $dpid, $amount, $timestamp, $amount);
}

// standards compliant version
function _radioactivity_add_energy_std($oid, $oclass, $dpid, $amount, $timestamp) {
  db_query("UPDATE {radioactivity} SET energy=energy+%f ".
           "WHERE id=%d AND class='%s' AND decay_profile=%d", $amount, $oid, $oclass, $dpid);

  if (db_affected_rows()==0) {
    // No new rows, try update again and insert one if necessary.
    // Note that we do the second update inside lock table to be certain that there wasn't insert
    // after the last update.

    db_lock_table("radioactivity");
    $result=db_query("UPDATE {radioactivity} SET energy=energy+%f ".
                     "WHERE id=%d AND class='%s' AND decay_profile=%d", $amount, $oid, $oclass, $dpid);

    if (db_affected_rows()==0) {
      db_query("INSERT INTO {radioactivity} (id, class, decay_profile, energy, last_emission_timestamp) ".
               "VALUES (%d, '%s', %d, %f, %d)",
               $oid, $oclass, $dpid, $amount, $timestamp);
    }
    db_unlock_tables();
  }
}

/**
 * Add energy to nodes.
 * @param $oid Object id
 * @param $oclass Object base class, e.g. 'node'. Subclass will be resolved if necessary
 * @param $source Energy source, e.g. 'view'
 * @param $allow_delayed_processing TRUE if delayed processing is allowed (memcache acceleration)
 * @return TRUE if successful
 */
function radioactivity_add_energy($oid, $oclass, $source, $allow_delayed_processing=TRUE) {
  if ($allow_delayed_processing && radioactivity_get_memcached_enable()) {
    // postpone event to memcache
    if (radioactivity_write_memcache_entry(array('type' => 'add-energy',
                                                 'oid' => $oid,
                                                 'oclass' => $oclass,
                                                 'source' => $source))) {
      return TRUE;
    }
  }

  return _radioactivity_add_energy_internal($oid, $oclass, $source, 1);
}

function _radioactivity_add_energy_internal($oid, $oclass, $source, $multiplier) {
  $db_type=$GLOBALS['db_type'];
  $timestamp=time();

  $energies=radioactivity_get_energies_for_source($oid, $oclass, $source);

  foreach ($energies as $dpid => $amount) {
    if (!$amount) continue;

    $amount*=$multiplier;

    // remap id if necessary
    $oid=_radioactivity_possibly_remap_id($oid, $oclass);

    switch ($db_type) {
      case 'mysql':
      case 'mysqli':
        _radioactivity_add_energy_mysql($oid, $oclass, $dpid, $amount, $timestamp);
        break;

      default:
        _radioactivity_add_energy_std($oid, $oclass, $dpid, $amount, $timestamp);
        break;
    }
  }

  return TRUE;
}

function radioactivity_get_radioactivity_array($oid, $oclass) {

  $ret=array();
  $energies=radioactivity_get_energy($oid, $oclass);

  foreach ($energies as $dpid => $energy) {
    $ret['energy'][$dpid]=(double)$energies[$dpid];
  }

  return $ret;
}

function radioactivity_get_memcached_enable() {
  if (radioactivity_determine_memcached_availability()!=RADIOACTIVITY_MEMCACHE_OK) {
    return 0;
  }
  return (int)variable_get('radioactivity_memcached_enable', 0);
}

function radioactivity_get_memcached_expiration() {
  return (int)variable_get('radioactivity_memcached_expiration', 600);
}


// returns one of RADIOACTIVITY_MEMCACHE_*
// Note that positive return value stands for available, negative values stand
// for not available
function radioactivity_determine_memcached_availability() {
  static $availability;

  if (!isset($availability)) {
    if (!function_exists('dmemcache_object'))
      $availability=RADIOACTIVITY_MEMCACHE_NO_MODULE;
    elseif (!dmemcache_object('radioactivity'))
      $availability=RADIOACTIVITY_MEMCACHE_NO_BIN;
    else
      $availability=RADIOACTIVITY_MEMCACHE_OK;
  }

  return $availability;
}

function radioactivity_write_memcache_entry($entry) {
  $mc=dmemcache_object('radioactivity');

  // get id sequence for new entry
  $id_key=dmemcache_key('entry_id_seq', 'radioactivity');

  $id=$mc->increment($id_key);

  if ($id===FALSE) {
    // initialize sequence
    $id=0;
    $mc->add($id_key, $id);
    dmemcache_set('entry_id_processed', 0, 0, 'radioactivity');
  }

  // put the entry into memcache
  return dmemcache_set('entry-'. $id, $entry,
                       radioactivity_get_memcached_expiration(),
                       'radioactivity');
}
