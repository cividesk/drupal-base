This module provide a new Image plugin, which is a mofified version of
CKEditor's image plugin. It helps you center image using toolbar button or
in Image dialog.

--------------------------------------------------------------------------------
Implementation:
--------------------------------------------------------------------------------

All the credit go to CKSource team, what I did is just a small modification in
the source code. I also take the idea from this one http://bit.ly/14GgCxp

--------------------------------------------------------------------------------
Integration:
--------------------------------------------------------------------------------

This module supports both CKEditor and WYSIWYG module.

--------------------------------------------------------------------------------
Dependencies:
--------------------------------------------------------------------------------

- CKEditor or WYSIWYG.

--------------------------------------------------------------------------------
Installation:
--------------------------------------------------------------------------------

Download the module and simply copy it into your contributed modules folder:
[for example, your_drupal_path/sites/all/modules] and enable it from the
modules administration/management page.
More information at: Installing contributed modules (Drupal 7).

--------------------------------------------------------------------------------
Configuration
--------------------------------------------------------------------------------

After successful installation, you don't have to do anything. Because this
module hijacks the Image button on toolbar, replaces it with the one in this
module and automatically enables the plugin itself.

But don't worry, you can disable it in profile configuration page of CKEditor
(admin/config/content/ckeditor) or WYSIWYG
(admin/config/content/wysiwyg/profile/filtered_html/edit).

And when you disable this module, it also automatically returns the original one

--------------------------------------------------------------------------------
Additional information
--------------------------------------------------------------------------------

This module supports CKEditor 3.1+
Tested on Chrome and Firefox with CKEditor 3.6.6.1 and 4.1.2 Standard/Full
