/**
 * @file
 * Utilities useful for manipulating media in wysiwyg editors.
 */

(function ($) {

"use strict";

Drupal.media = Drupal.media || {};

Drupal.media.utils = {

  /**
   * Generate integer hash of string.
   *
   * Rip-off from:
   * http://www.erlycoder.com/49/javascript-hash-functions-to-convert-string-into-integer-hash-
   *
   * This method has to match its PHP sibling implementation. The server-side
   * hash has to be the same as this.
   *
   * @see media_wysiwyg.filter.inc:media_wysiwyg_hash_code().
   *
   * @param {string} str String to generate hash from.
   *
   * @return {number}
   *   The generated hash code as integer.
   */
  hashCode: function(str) {
    var hash = 0;
    var char;
    var i;

    if (str.length == 0) {
      return hash;
    }
    for (i = 0; i < str.length; i++) {
      char = str.charCodeAt(i);
      hash = ((hash << 5) - hash) + char;
      // Convert to 31bit integer, omitting signed/unsigned compatibility issues
      // between php and js.
      hash &= 0x7fffffff;
    }
    return hash;
  },

  /**
   * Gets the HTML content of an element.
   *
   * Drupal 7 ships with JQuery 1.4.4, which allows $(this).attr('outerHTML') to
   * retrieve the element's HTML, but many sites use JQuery update and version
   * 1.6+, which insists on $(this).prop('outerHTML'). Until the minimum jQuery
   * is >= 1.6, we need to do this the old-school way. See
   * http://stackoverflow.com/questions/2419749/get-selected-elements-outer-html
   *
   * @param {jQuery} $element
   */
  outerHTML: function ($element) {
    return $element[0].outerHTML || $('<div>').append($element.eq(0).clone()).html();
  },

  /**
   * Assert that copied target is copied as new, not by reference.
   *
   * @param {mixed} src
   *   The value that should be copied.
   */
  copyAsNew: function(src) {
    return src instanceof Object ? $.extend(true, {}, src) : src;
  },

  /**
   * Find the first property in haystack that starts with needle.
   *
   * @param {string} needle
   *   The property starts with this string.
   * @param {object} haystack
   *   Object to search for matching property in.
   */
  propertyStartsWith(needle, haystack) {
    var property;
    for (property in haystack) {
      if (haystack.hasOwnProperty(property)) {
        if (property.startsWith(needle)) {
          return property;
        }
      }
    }
    return null;
  }

};

})(jQuery);
