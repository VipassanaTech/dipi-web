<?php
// Finalize report (/finalize-marked-attended, /finalize-not-marked-attended): open without login, so the slow
// report (9-14 s on prod) is built at most once every 15 minutes and everyone in between gets the same copy.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-finalize-report-cache.php
require __DIR__ . '/lib.php';

$cid = dh_finalize_csv_cid(FALSE);
$cid2 = dh_finalize_csv_cid(TRUE);
cache_clear_all($cid, 'cache');
cache_clear_all($cid2, 'cache');

echo "Kept for 15 minutes\n";
$a = dh_finalize_csv_get(FALSE);
$c1 = cache_get($cid);
t_ok($c1 && $c1->data === $a, 'first download builds the report and keeps it');
t_ok($c1 && $c1->expire >= REQUEST_TIME + 890 && $c1->expire <= REQUEST_TIME + 910, 'kept for 15 minutes');
t_ok($a === '' || strpos($a, "Course,MarkedA,TotalApplications\n") === 0, 'same columns as before (Course, MarkedA, TotalApplications)');
t_eq($a, dh_finalize_csv_build(FALSE), 'the kept copy is the full report');
$b = dh_finalize_csv_get(FALSE);
t_eq($b, $a, 'next download: same report');
t_eq(cache_get($cid)->created, $c1->created, 'next download did not build it again');

echo "Rebuilt after 15 minutes\n";
cache_set($cid, 'OLD COPY', 'cache', REQUEST_TIME - 1);
t_eq(dh_finalize_csv_get(FALSE), $a, 'an expired copy is not given out; the report is built again');

echo "Two reports kept apart\n";
t_ok($cid !== $cid2, 'marked and not-marked reports have their own copies');
$n = dh_finalize_csv_get(TRUE);
t_eq($n, dh_finalize_csv_build(TRUE), 'not-marked report: full report');
t_eq(cache_get($cid2)->data, $n, 'not-marked report kept');

echo "While another download is building it\n";
cache_set($cid, 'PREVIOUS COPY', 'cache', REQUEST_TIME - 1);
db_insert('semaphore')->fields(array('name' => $cid, 'value' => 'another-request', 'expire' => microtime(TRUE) + 60))->execute();
t_eq(dh_finalize_csv_get(FALSE, 0), 'PREVIOUS COPY', 'the previous copy is given instead of a second slow build');
cache_clear_all($cid, 'cache');
t_eq(dh_finalize_csv_get(FALSE, 0), $a, 'no copy at all: it is built anyway (never an empty download)');
db_delete('semaphore')->condition('name', $cid)->condition('value', 'another-request')->execute();

cache_clear_all($cid, 'cache');
cache_clear_all($cid2, 'cache');
t_done();
