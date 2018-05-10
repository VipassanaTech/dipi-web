Responsive Tables
=================

Overview
--------

Allows tables to act in a responsive fashion using a [technique designed by the Filament Group](http://filamentgroup.com/lab/responsive_design_approach_for_complex_multicolumn_data_tables/)

Installation
------------

Extract into your modules directory (ex: sites/all/modules/contrib) and enable the module.

Usage
-----

The module can be used as a library by calling drupal_add_library('responsive_tables', 'responsive_tables') along with your own table output.

It can also be used in conjunction with the [Better Views Tables](http://drupal.org/sandbox/minorOffense/1793630) module to allow configuration from the Views UI.

Edit your view and change the style plugin to "Better Views Tables". Then in the settings for the style plugin, mark the "Responsive" checkbox near the bottom and configure your column priorities to "Persist", "Essential", "Optional" or "None".

Persist
:   Sets a column to always show no matter the screen resolution

Essential
:   High priority column which remains visible at all times. But unlike "persist" columns, these can be toggled in the display drop down.

Optional
:   Optional columns are automatically hidden as the table is resized. They can be re-enabled using the display drop down.

None
:   These are the first columns to disappear as they have the lowest priority. They can also be toggled in the display.