<?php
// ============================================================================
// Deploy the "Cells and Dining Seats" wiki page to PROD (Drupal book, bid 10).
// Body snapshotted from LOCAL on generation; regenerate if the local page is
// edited again before deploy (scratchpad gen_wiki_prod.php).
//
// RUN ON PROD WITH php7.3 (php7.4 CLI lacks DOMDocument -> node_save on a
// full_html body fatals). From the Drupal root:
//     php7.3 deploy/2026-09-17-wiki-cell-dining.php
// Idempotent: updates the page if it already exists (matched by title in book 10),
// sets the alias, and adds the root-TOC link once.
// ============================================================================
define('DRUPAL_ROOT', getcwd());
require_once DRUPAL_ROOT.'/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$title = 'Cells and Dining Seats';
$alias = 'wiki-cell-dining';
$body  = base64_decode('PHA+RElQSSBjYW4gZ2l2ZSBlYWNoIHN0dWRlbnQgYSA8Yj5jZWxsPC9iPiAoYSBtZWRpdGF0aW9uIGNlbGwpIGFuZCBhIDxiPmRpbmluZyBzZWF0PC9iPi4gQ2VsbHMgYW5kIGRpbmluZyBzZWF0cyB3b3JrIGluIGV4YWN0bHkgdGhlIHNhbWUgd2F5LCBzbyB0aGlzIHBhZ2UgZXhwbGFpbnMgYm90aCB0b2dldGhlci4gVXNlIHRoZSA8Yj5DZWxsPC9iPiBzY3JlZW5zIGZvciBjZWxscyBhbmQgdGhlIDxiPkRpbmluZzwvYj4gc2NyZWVucyBmb3IgZGluaW5nIHNlYXRzICZtZGFzaDsgZXZlcnkgc3RlcCBiZWxvdyBpcyB0aGUgc2FtZS48L3A+DQoNCjxwPjxiPjEuIFR1cm4gaXQgb24gZm9yIHlvdXIgY2VudHJlPC9iPjwvcD4NCjxwPk9wZW4gPGI+Q2VudHJlIFNldHRpbmdzPC9iPi4gT3BlbiB0aGUgPGI+Q2VsbCBTZXR0aW5nczwvYj4gc2VjdGlvbiAob3IgPGI+RGluaW5nIFNldHRpbmdzPC9iPikgYW5kIHRpY2sgPGI+VGhpcyBjZW50cmUgaGFzIG1lZGl0YXRpb24gY2VsbHM8L2I+IChvciA8Yj5UaGlzIGNlbnRyZSBhc3NpZ25zIGRpbmluZyBzZWF0czwvYj4pLiBJZiB5b3VyIGNlbnRyZSBoYXMgbm8gY2VsbHMsIGxlYXZlIGl0IHVudGlja2VkLjwvcD4NCg0KPHA+PGI+Mi4gU2V0IHRoZSBjZWxsIC8gc2VhdCBudW1iZXJzPC9iPjwvcD4NCjxwPkluIHRoZSBzYW1lIHNlY3Rpb24gdGhlcmUgaXMgYSBzbWFsbCB0YWJsZS4gRm9yIDxiPk1hbGU8L2I+IGFuZCA8Yj5GZW1hbGU8L2I+LCB0eXBlIHdoaWNoIG51bWJlcnMgYXJlIGF2YWlsYWJsZSwgZm9yIGV4YW1wbGUgPGNvZGU+MS00MDwvY29kZT4gb3IgPGNvZGU+MS00MCwgNTUsIEExLUE1PC9jb2RlPi48L3A+DQo8dWw+DQo8bGk+WW91IGNhbiBnaXZlIDxiPm9sZDwvYj4gc3R1ZGVudHMgYW5kIDxiPm5ldzwvYj4gc3R1ZGVudHMgZGlmZmVyZW50IG51bWJlcnMgaWYgeW91IHdpc2guPC9saT4NCjxsaT5Zb3UgY2FuIGtlZXAgc29tZSBudW1iZXJzIGFzaWRlIHNvIHRoZXkgYXJlIG5ldmVyIGdpdmVuIG91dCBhdXRvbWF0aWNhbGx5LjwvbGk+DQo8bGk+SWYgeW91ciBzZXJ2ZXJzIChTZXZha3MpIGFsc28gZ2V0IGNlbGxzIG9yIHNlYXRzLCBhZGQgYSA8Yj5TZXJ2ZXI8L2I+IHJvdy48L2xpPg0KPC91bD4NCjxwPkNsaWNrIDxiPlVwZGF0ZTwvYj4gdG8gc2F2ZSB0aGUgc2V0dGluZ3MuIFlvdSB1c3VhbGx5IGRvIHN0ZXBzIDEgYW5kIDIgb25seSBvbmNlLjwvcD4NCg0KPHA+PGI+My4gT24gRGF5IDAgJm1kYXNoOyBtYXJrIGF0dGVuZGVkIGFuZCBhc3NpZ24gYXMgdGhleSBhcnJpdmU8L2I+PC9wPg0KPHA+T24gdGhlIDxiPlplcm8gRGF5PC9iPiBwYWdlLCBtYXJrIGVhY2ggc3R1ZGVudCA8Yj5hdHRlbmRlZDwvYj4gYXMgdGhleSByZWFjaCB0aGUgY2VudHJlLiBOZXh0IHRvIGEgcGVyc29uLCBjbGljayB0aGUgPGI+VXBkYXRlPC9iPiBidXR0b247IGluIHRoZSBwb3B1cCB5b3UgY2FuIHNldCB0aGVpciA8Yj5DZWxsPC9iPiBhbmQgPGI+RGluaW5nIHNlYXQ8L2I+IChhbmQgdGhlIGdyb3VwIG9uZXMgaWYgeW91IHVzZSBncm91cHMpLjwvcD4NCjx1bD4NCjxsaT5GaWxsIHRoZSA8Yj5NYWluPC9iPiBib3hlcyAoQ2VsbCwgRGluaW5nKSBmb3IgdGhlIG5vcm1hbCBwbGFuLiBJZiB0aGlzIGNvdXJzZSBzZWF0cyBwZW9wbGUgPGI+YnkgZ3JvdXA8L2I+IChHcm91cCAxLCBHcm91cCAyICZoZWxsaXA7KSwgZmlsbCB0aGUgPGI+R3JvdXA8L2I+IGJveGVzIChHcm91cCBDZWxsLCBHcm91cCBEaW5pbmcpIGluc3RlYWQuIEZpbGwgb25seSB0aGUgb25lcyB5b3UgbmVlZCAmbWRhc2g7IE1haW4gPGI+b3I8L2I+IEdyb3VwLCB3aGljaGV2ZXIgbWF0Y2hlcyBob3cgeW91IHNlYXQgdGhpcyBjb3Vyc2UuPC9saT48bGk+RWFjaCBib3ggc2hvd3Mgb25seSB0aGUgbnVtYmVycyB0aGF0IGFyZSBzdGlsbCBmcmVlLCBidXQgeW91IG1heSBhbHNvIHR5cGUgYW55IG51bWJlci48L2xpPg0KPGxpPlRpY2sgPGI+Rml4ZWQ8L2I+IGJlc2lkZSBhIGJveCB0byBsb2NrIHRoYXQgbnVtYmVyLCBzbyBpdCBpcyBub3QgY2hhbmdlZCB3aGVuIHlvdSBmaWxsIG9yIHJlZ2VuZXJhdGUgbGF0ZXIuPC9saT4NCjwvdWw+DQo8cD5Bc3NpZ25pbmcgaGVyZSBhcyBwZW9wbGUgYXJyaXZlIG1lYW5zIG1vc3Qgc3R1ZGVudHMgYWxyZWFkeSBoYXZlIHRoZWlyIGNlbGwgLyBzZWF0IGJlZm9yZSB5b3Ugb3BlbiB0aGUgd29ya2JlbmNoLjwvcD4NCg0KPHA+PGI+NC4gRmluYWxpemUgaW4gdGhlIFdvcmtiZW5jaDwvYj48L3A+DQo8cD5XaGVuIG1vc3QgcGVvcGxlIGFyZSBpbiwgb3BlbiB0aGUgY291cnNlIHBhZ2UgYW5kIGNsaWNrIDxiPkNlbGwgV29ya2JlbmNoPC9iPiAob3IgPGI+RGluaW5nIFdvcmtiZW5jaDwvYj4pLiBUaGlzIHNob3dzIGV2ZXJ5b25lIGluIG9uZSBsaXN0IHNvIHlvdSBjYW4gZmluaXNoIGFuZCBjaGVjayB0aGUgcGxhbi48L3A+DQo8dWw+DQo8bGk+PGI+QXV0by1maWxsPC9iPiAmbWRhc2g7IGZpbGxzIGFueW9uZSB3aG8gc3RpbGwgaGFzIG5vIG51bWJlciwgYXV0b21hdGljYWxseSwgaW4gc2VuaW9yaXR5IG9yZGVyLiBQZW9wbGUgeW91IG1hcmtlZCA8Yj5GaXhlZDwvYj4gYXJlIG5vdCBjaGFuZ2VkLjwvbGk+DQo8bGk+VG8gY2hhbmdlIG9uZSBwZXJzb24sIGNsaWNrIHRoZWlyIGJveCBhbmQgcGljayBhbm90aGVyIG51bWJlciwgb3IgdHlwZSBhIG5ldyBvbmUuPC9saT4NCjxsaT5BIGJveCB0dXJucyA8Yj5yZWQ8L2I+IGlmIHR3byBwZW9wbGUgaW4gdGhlIHNhbWUgZ3JvdXAgaGF2ZSB0aGUgc2FtZSBudW1iZXIuIFBsZWFzZSBmaXggaXQgYmVmb3JlIHlvdSBwcmludC48L2xpPg0KPGxpPjxiPkJhdGNoPC9iPiAmbWRhc2g7IGlmIGEgZ3JvdXAgaGFzIG1vcmUgcGVvcGxlIHRoYW4gbnVtYmVycywgRElQSSBzcGxpdHMgdGhlbSBpbnRvIGJhdGNoZXMgKGJhdGNoIDEgdXNlcyB0aGUgbnVtYmVycyBmaXJzdCwgdGhlbiBiYXRjaCAyIHVzZXMgdGhlIHNhbWUgbnVtYmVycyBhZ2FpbikuIFlvdSBjYW4gbW92ZSBhIHBlcnNvbiB0byBhbm90aGVyIGJhdGNoIGluIHRoZSA8Yj5CYXRjaDwvYj4gYm94LjwvbGk+DQo8bGk+VGhlIGNsZWFyIGJ1dHRvbnMgbGV0IHlvdSBzdGFydCBhZ2FpbjogPGI+Q2xlYXIgbm9uLWZpeGVkPC9iPiwgPGI+Q2xlYXIgYWxsPC9iPiwgYW5kIDxiPkNsZWFyIGFsbCBmaXhlZCBjaGVja2JveGVzPC9iPi48L2xpPg0KPGxpPjxiPlNhdmUgYWxsPC9iPiAmbWRhc2g7IHNhdmVzIHlvdXIgY2hhbmdlcy4gSXQgYXNrcyB5b3UgdG8gY29uZmlybSBmaXJzdCwgYmVjYXVzZSBpdCByZXBsYWNlcyB3aGF0IHdhcyBzYXZlZCBiZWZvcmUuPC9saT4NCjwvdWw+DQo8cD48Yj5NYWluIGFuZCBHcm91cC13aXNlOjwvYj4gYXQgdGhlIHRvcCB5b3UgY2FuIHN3aXRjaCBiZXR3ZWVuIDxiPk1haW48L2I+ICh0aGUgbm9ybWFsIHBsYW4pIGFuZCA8Yj5Hcm91cC13aXNlPC9iPiAodXNlIHRoaXMgb25seSB3aGVuIHlvdXIgZ3JvdXBzICZtZGFzaDsgR3JvdXAgMSwgR3JvdXAgMiAmaGVsbGlwOyAmbWRhc2g7IHNpdCBpbiBkaWZmZXJlbnQgcGxhY2VzIGFuZCBuZWVkIHRoZWlyIG93biBudW1iZXJzKS4gVGhlIHR3byBwbGFucyBhcmUgc2F2ZWQgc2VwYXJhdGVseS48L3A+DQoNCjxwPjxiPjUuIFByaW50IHRoZSBsaXN0PC9iPjwvcD4NCjxwPkZyb20gdGhlIHdvcmtiZW5jaCBjbGljayA8Yj5QcmludGFibGUgbGlzdDwvYj4sIG9yIG9wZW4gPGI+Q2VsbCBMaXN0PC9iPiAvIDxiPkRpbmluZyBMaXN0PC9iPiBmcm9tIHRoZSBjb3Vyc2UgcGFnZS4gT24gdGhlIGxpc3QgeW91IGNhbjo8L3A+DQo8dWw+DQo8bGk+U29ydCBieSA8Yj5OYW1lPC9iPiBvciBieSA8Yj5DZWxsPC9iPiAvIDxiPkRpbmluZyBTZWF0PC9iPi48L2xpPg0KPGxpPkNob29zZSA8Yj4xPC9iPiBvciA8Yj4yPC9iPiBjb2x1bW5zLjwvbGk+DQo8bGk+Q2xpY2sgPGI+UHJpbnQ8L2I+LjwvbGk+DQo8L3VsPg0KPHA+U2VydmVycyBzaG93IGFzIGEgc2VwYXJhdGUgZ3JvdXAgKGZvciBleGFtcGxlICZsZHF1bztNYWxlIFNlcnZlciZyZHF1bzspLiBBIHNlY3Rpb24gaXMgbWFya2VkICZsZHF1bztCYXRjaCAxJnJkcXVvOywgJmxkcXVvO0JhdGNoIDImcmRxdW87IG9ubHkgd2hlbiBpdCBoYXMgbW9yZSB0aGFuIG9uZSBiYXRjaC4gVXNlIDxiPkJhY2sgdG8gV29ya2JlbmNoPC9iPiBvbiB0aGUgbGlzdCBpZiB5b3Ugd2FudCB0byBtYWtlIG1vcmUgY2hhbmdlcy48L3A+');

// Prod root book page (nid 10) mlid within book 10 (prod nids/mlids differ from local).
$root_mlid = db_query("select mlid from book where bid=10 and nid=10")->fetchField();
if (!$root_mlid) { echo "ERROR: book root (bid=10, nid=10) not found on this site\n"; exit(1); }

$nid = db_query("select b.nid from book b join node n on n.nid=b.nid where b.bid=10 and n.title=:t", array(':t'=>$title))->fetchField();
if ($nid) {
  $node = node_load($nid);
  echo "updating existing nid=$nid\n";
} else {
  $node = new stdClass();
  $node->type = 'book';
  node_object_prepare($node);
  $node->uid = 1; $node->status = 1; $node->promote = 0; $node->comment = 0;
  $node->language = LANGUAGE_NONE;
  $w = (int) db_query("select max(weight) from menu_links where plid=:p", array(':p'=>$root_mlid))->fetchField();
  $node->book = array('bid'=>10, 'plid'=>$root_mlid, 'menu_name'=>'book-toc-10', 'weight'=>$w+1, 'module'=>'book');
  echo "creating new page under root mlid=$root_mlid weight=".($w+1)."\n";
}
$node->title = $title;
$node->body[LANGUAGE_NONE][0] = array('value'=>$body, 'format'=>'full_html');
node_save($node);
echo "saved nid={$node->nid}\n";

// URL alias (idempotent).
$src = 'node/'.$node->nid;
if (!db_query("select 1 from url_alias where source=:s and alias=:a", array(':s'=>$src, ':a'=>$alias))->fetchField()) {
  $p = array('source'=>$src, 'alias'=>$alias); path_save($p);
  echo "alias set: /$alias -> $src\n";
} else { echo "alias already present\n"; }

// Root TOC (node 10 body): add the link via the alias (nid-independent), once, before </ol>.
$root = node_load(10);
$rb = $root->body[LANGUAGE_NONE][0]['value'];
$li = '<li><a href="/'.$alias.'">'.$title.'</a></li>';
if (strpos($rb, 'href="/'.$alias.'"') !== false) {
  echo "root TOC already links the page\n";
} elseif (strpos($rb, '</ol>') === false) {
  echo "WARN: no </ol> in root body; add the TOC link manually\n";
} else {
  $rb = preg_replace('#</ol>#', $li.'</ol>', $rb, 1);
  $root->body[LANGUAGE_NONE][0]['value'] = $rb;
  $root->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  node_save($root);
  echo "root TOC updated\n";
}
drupal_flush_all_caches();
echo "done. caches cleared. Verify /wiki and /$alias on prod.\n";