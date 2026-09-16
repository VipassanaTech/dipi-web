<?php
// sites/all/modules/dh_manageapp/tests/cell_parity_test.php
// Proves the shared engine (dh_alloc_run, 'cell') assigns cells IDENTICALLY to
// the old hand-rolled allocator (_dh_generate_cell_list_legacy) across combined/
// sep/reserved/fixed/overflow cases. Cells are a PROD feature - parity is the gate.
define('DRUPAL_ROOT','/dhamma/web/dipinew'); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT.'/includes/bootstrap.inc'; drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
module_load_include('inc','dh_manageapp','inc/allocate');
module_load_include('inc','dh_manageapp','inc/zero-day');
function ok($c,$m){ echo ($c?"PASS":"FAIL")." - $m\n"; if(!$c) $GLOBALS['f']=1; }

$centre=5; $course=9900002;

// --- snapshot originals to restore at the end ---
$orig = db_query("select cs_cells_config, cs_has_cells from dh_center_setting where cs_center=$centre")->fetchObject();
$orig_rows = array();
foreach (db_query("select aa.aa_id, aa.aa_cell, aa.aa_cell_batch, aa.aa_cell_fixed from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course") as $r)
  $orig_rows[$r->aa_id] = array($r->aa_cell, $r->aa_cell_batch, $r->aa_cell_fixed);

function snap($course){
  $out=array();
  foreach (db_query("select aa.aa_id, aa.aa_cell, aa.aa_cell_batch from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_type='Student'") as $r)
    $out[$r->aa_id] = $r->aa_cell.'|'.$r->aa_cell_batch;
  return $out;
}
function clearcells($course){
  db_query("update dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant set aa.aa_cell=null, aa.aa_cell_batch=0, aa.aa_cell_fixed=0 where a.a_course=$course");
}
function setcfg($centre,$cfg){
  db_update('dh_center_setting')->fields(array('cs_cells_config'=>$cfg,'cs_has_cells'=>1))->condition('cs_center',$centre)->execute();
}
function applyfixed($fixed){ foreach($fixed as $aa=>$c){ db_update('dh_applicant_attended')->fields(array('aa_cell'=>$c,'aa_cell_fixed'=>1))->condition('aa_id',$aa)->execute(); } }

function parity($name,$centre,$course,$cfg,$fixed=array()){
  setcfg($centre,$cfg);
  clearcells($course); applyfixed($fixed);
  _dh_generate_cell_list_legacy($centre,$course);
  $A=snap($course);
  clearcells($course); applyfixed($fixed);
  generate_cell_list($centre,$course);   // new engine wrapper
  $B=snap($course);
  $diff=0; $ex='';
  foreach($A as $aa=>$v){ if(!isset($B[$aa])||$B[$aa]!==$v){ $diff++; if($ex==='') $ex=" e.g. aa=$aa legacy='$v' engine='".(isset($B[$aa])?$B[$aa]:'?')."'"; } }
  ok($diff===0, "$name: legacy==engine over ".count($A)." students ($diff diffs)$ex");
}

// a couple of male students to use as fixed examples
$two = db_query("select aa.aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=$course and a.a_gender='M' and a.a_type='Student' limit 2")->fetchCol();

parity('combined enough',  $centre,$course, "[MALE]\nCells = 1-500\n\n[FEMALE]\nCells = 1-500\n");
parity('combined overflow',$centre,$course, "[MALE]\nCells = 1-10\n\n[FEMALE]\nCells = 1-10\n");
parity('sep old/new',      $centre,$course, "[MALE]\nOld = 1-100\nNew = 101-200\n\n[FEMALE]\nOld = 1-100\nNew = 101-200\n");
parity('reserved',         $centre,$course, "[MALE]\nCells = 1-200\nReserved = 5,6,7\n\n[FEMALE]\nCells = 1-200\nReserved = 5,6\n");
parity('sep + reserved',   $centre,$course, "[MALE]\nOld = 1-50\nNew = 51-100\nReserved = 10,60\n\n[FEMALE]\nOld = 1-50\nNew = 51-100\n");
if (count($two)>=1) parity('with fixed cell', $centre,$course, "[MALE]\nCells = 1-200\n\n[FEMALE]\nCells = 1-200\n", array($two[0]=>'777'));
parity('combined tiny overflow', $centre,$course, "[MALE]\nCells = 1-3\n\n[FEMALE]\nCells = 1-3\n");

// --- restore originals ---
setcfg($centre, ''); // reset then apply orig
db_update('dh_center_setting')->fields(array('cs_cells_config'=>(string)$orig->cs_cells_config,'cs_has_cells'=>(int)$orig->cs_has_cells))->condition('cs_center',$centre)->execute();
foreach($orig_rows as $aa=>$v)
  db_update('dh_applicant_attended')->fields(array('aa_cell'=>$v[0],'aa_cell_batch'=>$v[1],'aa_cell_fixed'=>$v[2]))->condition('aa_id',$aa)->execute();
echo "-- originals restored --\n";
echo empty($GLOBALS['f'])?"ALL PASS\n":"FAILURES\n";
