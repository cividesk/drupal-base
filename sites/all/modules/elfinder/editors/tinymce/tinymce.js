function elfinder_tinymce_browse_callback(field_name, url, type, win) {
 var w = window.open(tinymce.settings.file_browser_url, null, 'toolbar=yes,menubar=yes,width=900,height=600');
 window.tinymceFileField = field_name;
 window.tinymceFileWin = win;
}