<?php
// $Id: radioactivity_node_views_handler_sort_left_or_inner.inc,v 1.1.2.4 2010/01/24 21:32:51 skiminki Exp $
/**
 * @file
 * Node radioactivity views handler to allow choosing between LEFT or INNER joining.
 */

class radioactivity_node_views_handler_sort_left_or_inner extends views_handler_sort {
  function option_definition() {
    $options = parent::option_definition();
    $options['join_type'] = array('default' => 'left');
    return $options;
  }
 
  function admin_summary() {
    $ret=parent::admin_summary();

    $type=t($this->options['join_type'] .' join');
    

    return $ret .', '. $type;
  }

  function radioactivity_object_type_in_plural() {
    return 'nodes';
  }

  function ensure_my_table() {
    if (!isset($this->table_alias)) {
      $join=$this->get_join();
      $join->type=strtoupper($this->options['join_type']);
      $this->table_alias = $this->query->ensure_table($this->table, $this->relationship, $join);
    }
    return $this->table_alias;
  }

  function query() {
    if ($this->options['join_type'] == 'inner') {
      // inner join: no coalesce workaround required
      parent::query();
      return;
    }

    // NOTE:
    // In Radioactivity, NULL energy (i.e. no energy row per node) is equivalent
    // to 0 energy. So, add the sort key accordingly.

    $this->ensure_my_table();
    $this->query->add_orderby(NULL,
                              'COALESCE('. $this->table_alias . '.' . $this->field .',0)', $this->options['order'],
                              $this->table_alias .'_'. $this->field .'_sortkey');
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    
    $form['join_type'] = array(
       '#type' => 'radios',
       '#title' => t('Join type'),
       '#options' => array(
         'left' => t('LEFT JOIN: Include all '. $this->radioactivity_object_type_in_plural()),
         'inner' => t('INNER JOIN: Include only '. $this->radioactivity_object_type_in_plural() .' that have radioactivity data (faster)'),
       ),
       '#description' => t('Select the join type for sorting.'),
       '#default_value' => $this->options['join_type'],
    );
  }

}
