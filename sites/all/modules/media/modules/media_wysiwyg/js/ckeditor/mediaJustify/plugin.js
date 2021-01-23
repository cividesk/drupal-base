/**
 * @file
 * CKEditor plugin: Align media using the toolbar align buttons.
 */

(function ($) {

"use strict";

CKEDITOR.plugins.add('mediaJustify', {
  afterInit: function (editor, pluginPath) {
    var justifyCommands = {
      left: aquireJustifyCommand('left'),
      center: aquireJustifyCommand('center'),
      right: aquireJustifyCommand('right')
    };

    /**
     * Assume ownership of justify* commands suitable for media alignment.
     *
     * @param {string} align
     *   The ending part of the 'justify*' commands. 'left', 'center' or
     *   'right'.
     *
     * @return {CKEDITOR.command}
     *   If command exists and properly setup, the existing modified command.
     *   null otherwise.
     */
    function aquireJustifyCommand(align) {
      var command = editor.getCommand('justify' + align);
      if (!command) {
        return null;
      }

      // Add ourselves to the command execution event. If the setting is right
      // (media element are selected), we assume ownership over the command and
      // cancels the event further. We are in direct competition with CKEditor
      // core plugins image and image2 which does the same, so we have to bump
      // the priority of this event handler (5th argument) and cancel further
      // execution of the event once we're done.
      command.on('exec', function (e) {
        var mediaElements = getSelectedMediaElements(editor);
        var $element, mediaInstance, aligned, i;
        if (! mediaElements.length) {
          // Not our context. Leave untouched.
          return;
        }
        for (i = 0; i < mediaElements.length; i++) {
          $element = $(mediaElements[i].$);
          if (!(mediaInstance = Drupal.media.filter.getMediaInstanceFromElement($element))) {
            // Element lost contact with or has no media instance object. Ripple
            // alignment event down for others to handle.
            return;
          }
          // Feed instance with current placeholder and set new alignment.
          mediaInstance.setPlaceholderFromWysiwyg($element);
          aligned = mediaInstance.setAlignment(align, true);
        }
        this.setState((mediaElements.length == 1 && aligned) ? CKEDITOR.TRISTATE_ON : CKEDITOR.TRISTATE_OFF);
        // Maybe a bug in CKEditor or not, but CKEDITOR.command.refresh() is not
        // run on other commands, so we have to manually reset the other
        // commands in this group.
        for (i in justifyCommands) {
          if (i != align && justifyCommands[i]) {
            justifyCommands[i].setState(CKEDITOR.TRISTATE_OFF);
          }
        }
        // We own the alignment command for this element/event combination. Don't
        // allow anyone else to mess with this.
        e.cancel();
      }, null, null, 5);

      // Ditto for the refresh handler.
      command.on('refresh', function (e) {
        var mediaElements = getSelectedMediaElements(editor);
        var $element, mediaInstance, currentAlignment;
        if (mediaElements.length != 1) {
          return;
        }
        $element = $(mediaElements[0].$);
        if (!(mediaInstance = Drupal.media.filter.getMediaInstanceFromElement($element))) {
          return;
        }
        // Feed instance with current placeholder before accessing alignment.
        mediaInstance.setPlaceholderFromWysiwyg($element);
        currentAlignment = mediaInstance.getAlignment();
        this.setState(currentAlignment == align ? CKEDITOR.TRISTATE_ON : CKEDITOR.TRISTATE_OFF);
        e.cancel();
      }, null, null, 5);
      return command;
    }
  }
});

/**
 * Get the currently selected media element(s) from editor.
 *
 * @param {CKEDITOR.editor} editor
 *   The editor instance to scan selection from.
 *
 * @return {CKEDITOR.dom.element[]}
 *   An array of CKEDITOR DOM media placeholder elements.
 */
function getSelectedMediaElements(editor) {
  var selection = editor.getSelection();
  var mediaElements = [];
  var ranges;
  var element;
  var i;

  // If the selection type is an element, only one element is selected.
  // Otherwise it's a set of ranges of type SELECTION_TEXT.
  if (selection.getType() == CKEDITOR.SELECTION_ELEMENT) {
    if ((element = getParentMediaElement(selection.getSelectedElement()))) {
      mediaElements.push(element);
    }
  }
  else {
    ranges = selection.getRanges();
    for (i = 0; i < ranges.length; i++) {
      if ((element = getParentMediaElement(ranges[i].getCommonAncestor(true, true)))) {
        mediaElements.push(element);
      }
    }
  }
  return mediaElements;
}

/**
 * Search element parents for nearest media-element.
 *
 * @param {CKEDITOR.dom.element} element
 *   The element to start from;
 *
 * @return {CKEDITOR.dom.element}
 *   The media element container, if found, null otherwise.
 */
function getParentMediaElement(element) {
  var parent;
  while (1) {
    if (element.hasClass('media-element')) {
      return element;
    }
    parent = element.getParent();
    if (!parent || parent == element) {
      break;
    }
    element = parent;
  }
  return null;
}

})(jQuery);
