<?php
// AT Portal Area Clusters (hole 9, 25 Nov 2026): the Edit page accepts only a real cluster number in the address
// (area_cluster_id_load; anything else = Page not found) and reads the cluster with a placeholder; the cluster list
// shows names as plain text.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-area-cluster.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
	$now = date('Y-m-d H:i:s');
	$id = (int) db_insert('dh_teacher_area_cluster')->fields(array('tac_name' => 'Zzq Cluster <b>bold</b>', 'tac_created' => $now, 'tac_created_by' => 1, 'tac_updated' => $now, 'tac_updated_by' => 1))->execute();

	echo "Edit address check\n";
	t_eq(area_cluster_id_load((string) $id), (string) $id, 'a real cluster number: accepted');
	t_ok(area_cluster_id_load("$id or 1=1") === FALSE, 'database text: refused');
	t_ok(area_cluster_id_load('abc') === FALSE, 'text: refused');
	t_ok(area_cluster_id_load('1e3') === FALSE, 'not a plain number: refused');
	t_ok(area_cluster_id_load('999999999') === FALSE, 'unknown number: refused (no empty "add" form)');
	t_ok(area_cluster_id_load(array($id)) === FALSE, 'not text: refused');
	$items = dh_atportal_menu();
	t_ok(isset($items['at-portal/area-cluster/edit/%area_cluster_id']) && !isset($items['at-portal/area-cluster/edit/%']), 'the Edit route uses the check');

	echo "Edit page\n";
	$_GET['q'] = "at-portal/area-cluster/edit/$id";
	drupal_static_reset('arg');
	$form = dh_atportal_manage_area_cluster();
	t_eq($form['tac_name']['#default_value'], 'Zzq Cluster <b>bold</b>', 'the cluster is loaded into the form');

	echo "Cluster list\n";
	drupal_static_reset('drupal_add_js');
	dh_atportal_area_cluster_list();
	$js = '';
	foreach (drupal_add_js() as $item)
		if (isset($item['type']) && $item['type'] == 'inline' && strpos($item['data'], 'tac-list') !== FALSE)
			$js .= $item['data'];
	t_ok(strpos($js, 'Zzq Cluster &lt;b&gt;bold&lt;\/b&gt;') !== FALSE, 'cluster name shown as plain text');
	t_ok(strpos($js, 'Zzq Cluster <b>') === FALSE, 'cluster name never as page code');
});

$src = file_get_contents(__DIR__ . '/../../dh_atportal/address-book.inc');
t_ok(strpos($src, 'tac_id=$tac_id') === FALSE, 'the cluster number is no longer pasted into the query');
t_done();
