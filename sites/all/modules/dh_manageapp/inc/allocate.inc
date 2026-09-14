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
