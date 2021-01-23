/**
 *  @file
 *  File with utilities to handle media in html editing.
 */
(function ($) {

  Drupal.media = Drupal.media || {};

  /**
   * Utility to deal with media tokens / placeholders.
   */
  Drupal.media.filter = {

    /**
     * @var {object} Map of media instances.
     */
    instanceMap: {},

    /**
     * Replaces media tokens with the placeholders for html editing.
     *
     * @param {string} content
     *   Text string (usually from textareas) to replace tokens in.
     *
     * @return {string}
     *   The given content as given with media tokens replaced with wysiwyg
     *   placeholder html equivalents.
     */
    replaceTokenWithPlaceholder: function(content) {
      var match, instance, markup;
      var matches = content.match(/\[\[.*?\]\]/g);

      if (!matches) {
        return content;
      }
      for (var i = 0; i < matches.length; i++) {
        match = matches[i];
        try {
          if ((instance = this.getMediaInstanceFromToken(match))) {
            instance.reloadPlaceholder();
          }
          else {
            instance = new Drupal.media.WysiwygInstance(match);
          }
          this.addMediaInstance(instance);
          markup = instance.getPlaceholderHtml();
        }
        catch (err) {
          // @todo: error logging.
          // Malformed or otherwise unusable token. Proceed to next.
          continue;
        }
        // Use split and join to replace all instances of macro with markup.
        content = content.split(match).join(markup);
      }
      return content;
    },

    /**
     * Replaces media placeholder elements with tokens.
     *
     * @param content (string)
     *   The markup within the wysiwyg instance.
     *
     * @return {string}
     *   The given content with wysiwyg placeholder code replaced with media
     *   tokens ready for input filtering.
     */
    replacePlaceholderWithToken: function(content) {
      // Locate and process all the media placeholders in the WYSIWYG content.
      // @todo: once baseline jQuery is 1.8+, switch to using
      // $.parseHTML(content)
      var $contentElements = $('<div/>');
      var self = this;
      $contentElements.get(0).innerHTML = content;
      $contentElements.find('.media-element').each(function () {
        var $placeholder = $(this);
        var mediaInstance = self.getMediaInstanceFromElement($placeholder);
        if (!mediaInstance) {
          return;
        }
        // Feed instance with current placeholder and make sure we still are
        // able to track it before replacing it with the token.
        mediaInstance.setPlaceholderFromWysiwyg($placeholder);
        self.addMediaInstance(mediaInstance);
        $(this).replaceWith(mediaInstance.getToken());
      });
      content = $contentElements.html();
      return content;
    },

    /**
     * Add and keep track of a media instance.
     *
     * @param {Drupal.media.WysiwygInstance} instance
     *   The instance to keep track of.
     *
     * @return {string}
     *   The index in this.instanceMap the instance was added to.
     */
    addMediaInstance: function(instance) {
      var key = instance.getKey();
      this.instanceMap[key] = instance;
      return key;
    },

    /**
     * Get media instance related to token.
     *
     * @param {string} token
     *   Find media instance based on this media token.
     *
     * @return {Drupal.media.WysiwygInstance}
     */
    getMediaInstanceFromToken: function(token) {
      var instanceKey = Drupal.media.WysiwygInstance.createKey(token);
      return instanceKey ? this.instanceMap[instanceKey] : null;
    },

    /**
     * Get media instance related to placeholder element.
     *
     * @param {jQuery} $element
     *   Placeholder element.
     *
     * @return {Drupal.media.WysiwygInstance}
     */
    getMediaInstanceFromElement: function($element) {
      var instanceKey = $element.attr('data-media-key');
      var instance = null;
      if (instanceKey) {
        instance = this.instanceMap[instanceKey];
      }
      return instance;
    }

  };

})(jQuery);
