<?php

$file = fopen("dh_donation.csv","r");
print_r(fgetcsv($file));
fclose($file);