<?php
/**
 * Unified resource-allocation engine (cells + dining). See
 * docs/superpowers/specs/2026-09-13-unified-allocation-engine-design.md.
 */

/** Resource descriptor: 'cell' or 'dining'. */
function dh_alloc_descriptor($resource) {
  if ($resource === 'cell') {
    return array('resource'=>'cell','name'=>'Cell','has'=>'cs_has_cells','config'=>'cs_cells_config',
      'main_col'=>'aa_cell','group_col'=>'aa_group_cell','fixed_col'=>'aa_cell_fixed',
      'batch_col'=>'aa_cell_group','shareable'=>true);
  }
  return array('resource'=>'dining','name'=>'Dining','has'=>'cs_has_dining','config'=>'cs_dining_config',
    'main_col'=>'aa_dining','group_col'=>'aa_group_dining','fixed_col'=>'aa_dining_fixed',
    'batch_col'=>null,'shareable'=>false);
}

/** Expand "1-40, 55, A1-A5" into an array of labels (letter-prefix aware). */
function dh_alloc_expand($str) {
  $out = array();
  if ($str === '' || $str === null) return $out;
  foreach (explode(',', $str) as $v) {
    $v = trim($v);
    if ($v === '') continue;
    $p = explode('-', $v);
    if (count($p) > 1) {
      $prefix = preg_replace('/[0-9]/', '', $p[0]);
      $a = (int) preg_replace('/[A-Za-z-]/', '', $p[0]);
      $b = (int) preg_replace('/[A-Za-z-]/', '', $p[1]);
      for ($i = $a; $i <= $b; $i++) $out[] = $prefix . $i;
    } else {
      $out[] = $p[0];
    }
  }
  return $out;
}

/** Build a pool descriptor for one INI section. */
function dh_alloc_pool($ini, $key) {
  $sec = (isset($ini[$key]) && is_array($ini[$key])) ? $ini[$key] : array();
  $reserved = dh_alloc_expand(isset($sec['Reserved']) ? $sec['Reserved'] : '');
  $sep = (isset($sec['Old']) && $sec['Old'] !== '') || (isset($sec['New']) && $sec['New'] !== '');
  if ($sep) {
    return array('sep'=>true, 'cells'=>array(),
      'old'=>dh_alloc_expand(isset($sec['Old']) ? $sec['Old'] : ''),
      'new'=>dh_alloc_expand(isset($sec['New']) ? $sec['New'] : ''),
      'reserved'=>$reserved);
  }
  return array('sep'=>false, 'old'=>array(), 'new'=>array(),
    'cells'=>dh_alloc_expand(isset($sec['Cells']) ? $sec['Cells'] : ''),
    'reserved'=>$reserved);
}

/** True if the config defines any server section. */
function dh_alloc_has_server_config($ini) {
  if (!is_array($ini)) return false;
  foreach ($ini as $k => $v) {
    if (strpos($k, '_SERVER') !== false) return true;
  }
  return false;
}

/** Resolve the INI section key for one attendee, or null (server w/o section). */
function dh_alloc_effective_key($ini, $gender, $group, $class, $mode) {
  $base = (strtoupper($gender) === 'M') ? 'MALE' : 'FEMALE';
  if ($class === 'server') $base .= '_SERVER';
  if ($mode === 'group') {
    $g = (int) $group;
    if ($g > 0 && isset($ini[$base . '_' . $g])) return $base . '_' . $g;
  }
  if (isset($ini[$base])) return $base;
  // Students always resolve to their base (pool may be empty -> blank);
  // servers only when a server section exists.
  return ($class === 'server') ? null : $base;
}
