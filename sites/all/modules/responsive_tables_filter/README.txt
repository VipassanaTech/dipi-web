RESPONSIVE TABLES FILTER
------------------------
Enable the "Make tables responsive" text format filter to display field tables responsively using the [Tablesaw Library](https://www.filamentgroup.com/lab/tablesaw.html).

 * For a full description of the module, visit the project page:
   https://drupal.org/project/responsive_tables_filter

REQUIREMENTS / DEPENDENCIES
---------------------------
Filter module (core)
jQuery version 1.7 or higher. Consider using [jQuery Update module](https://www.drupal.org/project/jquery_update)
[Tablesaw v 1.x or 2.x](https://www.filamentgroup.com/lab/tablesaw.html)

INSTALLATION
------------
1. Extract this module into your modules directory (ex: sites/all/modules/contrib)
and enable it.

USAGE
-----
0. If you want to make Views tables responsive, enable this at /admin/config/content/responsive_tables_filter.
1. Go to admin/config/content/formats.
2. On any text formats for which you want to make tables responsive (e.g., Filtered HTML),
Enable the filter "Make tables responsive" .
3. Verify the text format(s) allow HTML table tags (admin/config/content/formats
> “Limit HTML tags”). All of the following should be allowed:
<table> <th> <tr> <td> <thead> <tbody> <tfoot>

Any fields that use the text format(s) which have tables in them will now be
responsive.

FAQ
---
Q: Does this make Drupal Views tables responsive?

A: Yes. Enable this at /admin/config/content/responsive_tables_filter

Q: Can I override the tablesaw CSS?

A: Yes. On the configuration page (/admin/config/content/responsive_tables_filter),
you can set a path to a custom CSS file that will take the place of what this
module supplies at tablesaw/css/tablesaw.stackonly.css. It is best to copy that
file, then make your modifications. The maintainer takes no responsibility if
your modifications break functionality.

Q: What if I can't use jQuery update due to other requirements?

A: Comment out the dependency on jQuery Update in this module’s .info file, and try using an older version of the Tablesaw library.

Q: Can I target specific tables within nodes?

A: Yes. The Drupal variable 'responsive_tables_filter_table_xpath' accepts an xpath query. See _tablesaw_filter().

Q: Can I do anything to have tablesaw not be applied to a table (WYSIWYG Only).

A: Yes! Add the class 'no-tablesaw' to the table.

MAINTAINERS
-----------
Current maintainers:
 * Mark Fullmer (mark_fullmer) - https://www.drupal.org/u/mark_fullmer
