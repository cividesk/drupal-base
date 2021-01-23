function elfinder_ffs_callback(url) {
          var fieldName = Drupal.settings.elfinder.field_name;
          var fieldId = Drupal.settings.elfinder.filepath_id;
          
          var filePath = url;

          window.opener.jQuery('input#'+fieldId).val(filePath).change();
          window.opener.focus();
          window.close();
}
