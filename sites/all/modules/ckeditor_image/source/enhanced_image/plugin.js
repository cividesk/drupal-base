/**
 * @file
 * Enhanced Image plugin
 */

(function() {
  "use strict";

  function getSelectedImage(editor, element) {
    if (!element) {
      var sel = editor.getSelection();
      element = sel.getSelectedElement();
    }

    if (element && element.is('img') && !element.data('cke-realelement') && !element.isReadOnly()) {
      return element;
    }
  }

  function getImageAlignment(element) {
    var align = element.getStyle('float');
    var ds, ml, mr;

    if (align == 'inherit' || align == 'none') {
      align = 0;
    }

    if (!align) {
      align = element.getAttribute('align');
    }

    if(!align) {
      ds = element.getStyle('display');
      ml = element.getStyle('margin-left');
      mr = element.getStyle('margin-right');

      if(ds == 'block' && ml == 'auto' && mr == 'auto') {
        align = 'center';
      }
    }

    return align;
  }

  var ckeditorVersion = CKEDITOR.version.split('.')[0] | 0;
  var pluginName      = 'enhanced_image';
  var pluginRequires  = ckeditorVersion < 4 ? ['dialog'] : 'dialog';

  var pluginInit      = function(editor) {
    var imagePlugin = CKEDITOR.plugins.get('image') ? true : false;
    var btnOptions  = {
      label    : editor.lang.common.image,
      command  : pluginName,
      toolbar  : 'insert,10'
    };

    // Register the dialog.
    CKEDITOR.dialog.add(pluginName, this.path + 'dialogs/enhanced_image_' + ckeditorVersion + '.js');

    if(ckeditorVersion > 3) {
      var allowed     = 'img[alt,!src]{border-style,border-width,float,display,height,margin,margin-bottom,margin-left,margin-right,margin-top,width}';
      var required    = 'img[alt,src]';
      btnOptions.icon = 'image';

      if (CKEDITOR.dialog.isTabEnabled(editor, pluginName, 'advanced')) {
        allowed = 'img[alt,dir,id,lang,longdesc,!src,title]{*}(*)';
      }

      // Register the command.
      editor.addCommand(pluginName, new CKEDITOR.dialogCommand(pluginName, {
        allowedContent        : allowed,
        requiredContent       : required,
        contentTransformations: [
          ['img{width}: sizeToStyle', 'img[width]: sizeToAttribute'],
          ['img{float}: alignmentToStyle', 'img[align]: alignmentToAttribute']
        ]
      }));

    }
    else {
      btnOptions.className = 'cke_button_image';
      editor.addCommand(pluginName, new CKEDITOR.dialogCommand(pluginName));
    }

    // Register the toolbar button.
    editor.ui.addButton && editor.ui.addButton('EnhancedImage', btnOptions);

    editor.on('doubleclick', function(evt) {
      var element = evt.data.element;

      if (element.is('img') && !element.data('cke-realelement') && !element.isReadOnly()) {
        evt.data.dialog = pluginName;
      }
    });

    // If the "menu" plugin is loaded, register the menu items.
    if (editor.addMenuItems) {
      if (imagePlugin) {
        delete editor._.menuItems.image;
      }

      editor.addMenuItems({
        image: {
          label  : editor.lang.image.menu,
          command: pluginName,
          group  : 'image'
        }
      });
    }

    // If the "contextmenu" plugin is loaded, register the listeners.
    if (editor.contextMenu) {
      // Unplug image plugin from the context menu.
      if (imagePlugin) {
        var listeners = editor.contextMenu._.listeners || [];
        var position  = false;

        for (var i = 0; i < listeners.length; i++) {
          var listener = listeners[i].toString();

          if(/return[\r\n\s]*\{([\r\n\s]*)image\s*:\s*((CKEDITOR\.TRISTATE_OFF)|2)[\r\n\s]*\}/.test(listener)) {
            position = i;
            break;
          }
        }

        if(position !== false) {
          listeners.splice(position, 1);
        }
      }

      // Register our listener.
      editor.contextMenu.addListener(function(element, selection) {
        if (getSelectedImage(editor, element)) {
          return {
            image: CKEDITOR.TRISTATE_OFF
          };
        }
      });
    }
  };

  var pluginAfterInit = function(editor) {
    function setupAlignCommand(value) {
      var command = editor.getCommand('justify' + value);

      if (command) {
        if (value == 'left' || value == 'right' || value == 'center') {
          command.on('exec', function(evt) {
            var img = getSelectedImage(editor);

            if (img) {
             var align = getImageAlignment(img);

              if (value == align) {
                  img.removeStyle('float');
                  img.removeStyle('display');
                  img.removeStyle('margin-right');
                  img.removeStyle('margin-left');

                  // Remove "align" attribute when necessary.
                  if (value == getImageAlignment(img)) {
                    img.removeAttribute('align');
                  }
              }
              else {
                switch (value) {
                  case 'left':
                  case 'right':
                  default:
                      img.setStyle('float', a);
                      img.removeStyle('display');
                      img.removeStyle('margin-left','auto');
                      img.removeStyle('margin-right','auto');
                      break;

                  case 'center':
                      img.removeStyle('float');
                      img.setStyle('display','block');
                      img.setStyle('margin-left','auto');
                      img.setStyle('margin-right','auto');
                      break;
                }
              }

              evt.cancel();
            }
          });
        }

        // Run this callback before Image plugin
        command.on('refresh', function(evt) {
          var img = getSelectedImage(editor);
          var align, state;

          if (img) {
            align = getImageAlignment(img);
            state = (align == value) ? CKEDITOR.TRISTATE_ON : ((value == 'right' || value == 'left' || value == 'center') ? CKEDITOR.TRISTATE_OFF : CKEDITOR.TRISTATE_DISABLED);

            this.setState(state);
            evt.cancel();
          }
        }, this, {}, -1);
      }
    }

    // Customize the behavior of the alignment commands. (#7430)
    setupAlignCommand('left');
    setupAlignCommand('right');
    setupAlignCommand('center');
    setupAlignCommand('block');
  };

  CKEDITOR.plugins.add(pluginName, {
    requires : pluginRequires,
    // lang     : 'af,ar,bg,bn,bs,ca,cs,cy,da,de,el,en,en-au,en-ca,en-gb,eo,es,et,eu,fa,fi,fo,fr,fr-ca,gl,gu,he,hi,hr,hu,is,it,ja,ka,km,ko,ku,lt,lv,mk,mn,ms,nb,nl,no,pl,pt,pt-br,ro,ru,si,sk,sl,sq,sr,sr-latn,sv,th,tr,ug,uk,vi,zh,zh-cn',
    icons    : 'image',
    init     : pluginInit,
    afterInit: pluginAfterInit
  });
}) ();

/**
 * Whether to remove links when emptying the link URL field in the image dialog.
 *
 * config.image_removeLinkByEmptyURL = false;
 */
CKEDITOR.config.image_removeLinkByEmptyURL = true;

/**
 * Padding text to set off the image in preview area.
 *
 * config.image_previewText = CKEDITOR.tools.repeat('___ ', 100);
 */
