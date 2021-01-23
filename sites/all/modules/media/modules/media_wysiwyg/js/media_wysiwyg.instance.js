/**
 *  @file
 *  Utilities related to one inserted media instance in text areas.
 */

(function ($) {
"use strict";

Drupal.media = Drupal.media || {};

/**
 * Wysiwyg media instance handling.
 *
 * This class unites everything related to one media instance in wysiwyg
 * environments. Its a map and logic around the moving pieces of the game:
 * - The media token: as JSON-encoded string encapsulated in double brackets.
 * - The media instance settings: an object with all settings and attributes in
     play.
 * - The wysiwyg placeholder element: The element present in wysiwyg editors as
     a jQuery object.
 *
 * @param {string|object} instanceInfo
 *   This should either be the token as it exists in textareas, including
 *   opening '[[' and closing ']]' brackets, or the instance settings as an
 *   object, i.e. the parsed equivalent of the token.
 * @param {string} placeholderBase
 *   Use this as media wysiwyg placeholder base/template to extend
 *   settings/attributes on. Required if this is a new media instance. Existing
 *   media instances will have this base available either from
 *   Drupal.settings.media.tagMap or from existing WysiwygInstance objects.
 */
Drupal.media.WysiwygInstance = function(instanceInfo, placeholderBase) {

  /**
   * @var {string} The full media token.
   */
  this.token = '';

  /**
   * @var {object} The settings for this media instance.
   *
   * This keeps information about view mode, alignment, attributes and
   * overridden fields. I.e. settings that are specific to this inserted
   * instance, independent of the file it belongs to. In effect, this is the
   * parsed version of this.token;
   */
  this.settings = null;

  /**
   * @var {string} Our unique index key.
   *
   * A hashed version of this object suitable for indexing inserted media
   * instance objects.
   */
  this.key = '';

  /**
   * @var {jQuery} Placeholder DOM element as jQuery object.
   */
  this.$placeholder = null;

  if (typeof instanceInfo === 'object') {
    this.settings = instanceInfo;
    this.verifySettings();
    // If an object was given, it probably comes from the server side
    // media_wysiwyg_format_form(). This adds the file type as well which is
    // required for figuring out what overridable fields are available for this
    // media instance. It's not part of the token schema, only used internally.
    if (this.settings.file_type) {
      Drupal.settings.media.fidTypeMap[this.settings.fid] = this.settings.file_type;
    }
  }
  else if (typeof instanceInfo === 'string') {
    this.token = instanceInfo;
  }
  if (placeholderBase) {
    this.setPlaceholderFromServer(placeholderBase);
  }
};

Drupal.media.WysiwygInstance.prototype = {

  /**
   * Get the current instance settings.
   *
   * @return {object}
   *   The media instance settings.
   *
   * @throws {string}
   *   If object is missing basic info.
   */
  getSettings: function() {
    if (!this.settings) {
      if (!this.token) {
        throw "Instance missing basic info";
      }
      this.settings = JSON.parse(this.token.replace('[[', '').replace(']]', ''));
      this.verifySettings();
    }
    return this.settings;
  },

  /**
   * Verify settings against media token schema.
   *
   * @throws {string}
   *   Error message if token doesn't fullfill schema spec.
   */
  verifySettings: function() {
    var property, propSettings;
    var self = this;
    if (!this.settings) {
      throw "Invalid state: Cannot verify settings without settings.";
    }
    $.each(Drupal.settings.media.tokenSchema, function(property, propSettings) {
      var settingValue;
      if (propSettings.required) {
        if (!self.settings[property]) {
          throw "Media token parse error: Missing required property '" + property + "'.";
        }
      }
      else {
        if (!self.settings[property]) {
          // For empty values, use the default value from schema to assert its type.
          self.settings[property] = Drupal.media.utils.copyAsNew(propSettings.default);
        }
      }
      if (propSettings.options !== undefined && self.settings[property]) {
        settingValue = self.settings[property];
        if (!propSettings.options[settingValue]) {
          // Add default value instead of throwing exception. At least the
          // editor will have something to work with.
          // @todo: Error logging/message about malformed token?
          self.settings[property] = propSettings.default;
        }
      }
    });
    if (this.settings.type !== 'media') {
      throw "Token not of type 'media'.";
    }
  },

  /**
   * Get the media token for this instance.
   *
   * @return {string}
   *   The media token including opening '[[' and closing ']]' brackets.
   */
  getToken: function() {
    if (this.token) {
      return this.token;
    }
    var settings = this.getSettings();
    if (typeof settings.link_text == 'string') {
      settings.link_text = this.overrideLinkTitle();
      // Make sure the link_text-html-tags are properly escaped.
      settings.link_text = settings.link_text.replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }
    this.token = '[[' + JSON.stringify(this.prepareSettingsForToken()) + ']]';
    return this.token;
  },

  /**
   * Get this instance's indexable key.
   *
   * @return {string}
   *   Instance indexable key.
   */
  getKey: function() {
    if (!this.key) {
      this.key = Drupal.media.WysiwygInstance.createKey(this.getToken());
    }
    return this.key;
  },

  /**
   * Prepare and set placeholder from server for wysiwyg.
   *
   * Placeholders given from rendered file entities comes in various form and
   * formats. Prepare it by removing link wrappers, assert it has a valid tag
   * and apply the instance settings on it, making it ready for inserting in
   * wysiwyg environments.
   *
   * @param {string} placeholder
   *   The placeholder coming from Drupal.
   */
  setPlaceholderFromServer: function(placeholder) {
    if ($('<div>').append(placeholder).text().length === placeholder.length) {
      // Element is a #text node and needs to be wrapped in a HTML element so
      // attributes can be attached.
      placeholder = '<span>' + placeholder + '</span>';
    }
    this.$placeholder = $(placeholder);
    // Parse out link wrappers.
    // @todo Fix link management for wysiwyg media.
    // @see #2918848.
    if (this.$placeholder.is('a') && this.$placeholder.find('img').length) {
      this.$placeholder = this.$placeholder.children();
    }
    this.syncSettingsToPlaceholder();
  },

  /**
   * Set the placeholder from Wysiwyg environment.
   *
   * Calling this informs this instance that the given placeholder is the
   * dominant source of media instance state. This will update the instance
   * settings and reset the token and key.
   *
   * @param {jQuery} $placeholder.
   *   The wysiwyg media placeholder.
   */
  setPlaceholderFromWysiwyg: function($placeholder) {
    this.$placeholder = $placeholder;
    this.syncPlaceholderToSettings();
  },

  /**
   * Get the wysiwyg placeholder as jQuery element.
   *
   * @return {jQuery}
   */
  getPlaceholder: function() {
    var placeholderBase;

    if (this.$placeholder) {
      return this.$placeholder;
    }
    if (!(placeholderBase = Drupal.settings.media.tagMap[this.getKey()])) {
      // The token is no longer the same as server-side, and the server-side
      // rendered html template based on this token is no longer available.
      // @todo: Feature parity with 3.x require map between fid and base html.
      throw "Unable to retrieve media html.";
    }
    this.setPlaceholderFromServer(placeholderBase);
    return this.$placeholder;
  },

  /**
   * Get a HTML representation of media token suitable for wysiwyg editors.
   *
   * @return {string}
   *   HTML representation of instance wysiwyg placeholder.
   */
  getPlaceholderHtml: function() {
    return Drupal.media.utils.outerHTML(this.getPlaceholder());
  },

  /**
   * Reload placeholder according to settings.
   *
   * This resets the placeholder to a 'clean' state and builds it up again based
   * on this instance's settings.
   */
  reloadPlaceholder() {
    if (!this.getPlaceholder()) {
      throw "Invalid state: Placeholder missing";
    }
    this.resetPlaceholderAttributes();
    this.syncSettingsToPlaceholder();
  },

  /**
   * Remove generated attributes on wysiwyg placeholder.
   *
   * Some attributes and classes are generated and added to placeholder elements
   * based on instance settings, and the mere fact that they are media elements.
   * This method resets the placeholder element so no attributes are
   * unnecessarily present.
   */
  resetPlaceholderAttributes: function() {
    if (!this.$placeholder) {
      throw "Invalid state: Instance placeholder is missing.";
    }

    // Remove any existing view mode classes, alignment classes and various data
    // attributes related to media management.
    this.removePlaceholderAlignment();
    this.$placeholder.removeClass(function (index, css) {
      return (css.match(/\bfile-\S+/g) || []).join(' ');
    }).removeClass('media-element')
      .removeAttr('data-fid')
      .removeAttr('data-media-element')
      .removeAttr('data-media-key');
  },

  /**
   * Get current alignment for instance.
   */
  getAlignment: function() {
    var settings = this.getSettings();
    return settings.alignment;
  },

  /**
   * Align instance.
   *
   * @param {string} value
   *   The alignment value. Allowed values are 'left', 'center' or 'right'.
   * @param {bool} toggle
   *   Optional. Set this to true to toggle alignment on or off based on current
   *   alignment.
   *
   * @return {bool}
   *   true if alignment was set to given value, false otherwise.
   */
  setAlignment: function(value, toggle) {
    var currentAlignment = this.getAlignment();
    if (currentAlignment == value) {
      if (toggle) {
        this.removePlaceholderAlignment();
        this.settings.alignment = '';
        return false;
      }
      return true;
    }
    else {
      if (currentAlignment) {
        this.removePlaceholderAlignment();
      }
      this.$placeholder.addClass('media-wysiwyg-align-' + value);
      this.settings.alignment = value;
      this.token = this.key = '';
      return true;
    }
  },

  /**
   * Remove any alignment methods from placeholder.
   */
  removePlaceholderAlignment: function() {
    this.$placeholder.removeClass(function (index, css) {
      return (css.match(/\bmedia-wysiwyg-align-\S+/g) || []).join(' ');
    }).removeAttr('align')
      .css('float', '');
  },

  /**
   * Sync state from settings to wysiwyg placeholder element.
   */
  syncSettingsToPlaceholder: function() {
    var attributes, classes;
    var settings = this.getSettings();
    var self = this;

    if (!this.$placeholder) {
      throw "Invalid state: Instance placeholder is missing.";
    }

    this.syncFieldsToAttributes();

    // Move attributes from instance settings to the placeholder element.
    if (settings.attributes) {
      $.each(Drupal.settings.media.wysiwygAllowedAttributes, function(i, allowed_attribute) {
        if (settings.attributes[allowed_attribute]) {
          self.$placeholder.attr(allowed_attribute, settings.attributes[allowed_attribute]);
        }
        else if (self.$placeholder.attr(allowed_attribute)) {
          // If the element has the attribute, but the value is empty, be sure
          // to clear it.
          self.$placeholder.removeAttr(allowed_attribute);
        }
      });
      delete(settings.attributes);
    }
    // Reset placeholder element and start generating various attributes and
    // classes based on instance settings and custom attributes.
    this.resetPlaceholderAttributes();
    this.$placeholder
      .attr('data-fid', settings.fid)
      .attr('data-media-key', this.getKey());
    classes = ['media-element', 'file-' + settings.view_mode.replace(/_/g, '-')];
    if (settings.alignment) {
      classes.push('media-wysiwyg-align-' + settings.alignment);
    }
    this.$placeholder.addClass(classes.join(' '));

    // Attempt to override the link_title if the user has chosen to do this.
    settings.link_text = this.overrideLinkTitle();
    // Apply link_text if present.
    if ((settings.link_text) && (!settings.external_url || settings.external_url.length === 0)) {
      $('a', this.$placeholder).html(settings.link_text);
    }
  },

  /**
   * Sync state in wysiwyg placeholder element back to settings.
   */
  syncPlaceholderToSettings: function() {
    var value;
    var $placeholder = this.getPlaceholder();
    var settings = this.getSettings();

    // Attributes. Start with flushing out attributes from settings. Insert
    // allowed attributes from placeholder to settings.attributes, and finally
    // sync attributes that are fed by fields back to their respective field ID.
    settings.attributes = {};
    $.each(Drupal.settings.media.wysiwygAllowedAttributes, function(i, allowed_attribute) {
      if ((value = $placeholder.attr(allowed_attribute))) {
        // Replace &quot; by \" to avoid error with JSON format.
        if (typeof value == 'string') {
          value = value.replace('&quot;', '\\"');
        }
        settings.attributes[allowed_attribute] = value;
      }
    });
    this.syncAttributesToFields();
    this.aggregateAlignmentFromAttributes();

    // Extract the link text, if there is any.
    settings.link_text = (Drupal.settings.media.doLinkText) ? $placeholder.find('a:not(:has(img))').html() : false;
    // The placeholder and this.settings are now the correct 'owner' of this
    // instance, and the generated token and key, if any is outdated. These have
    // to be regenerated in order to get a new data-fid-key attribute in place.
    this.token = '';
    this.key = '';
  },

  /**
   * Aggregate various alignment methods to settings.alignment.
   *
   * Wysiwyg editors are free to set media alignment either with the 'align'
   * attribute or inline css float, i.e. use other methods of alignment than the
   * text justify buttons or media instance settings for alignment. The goal
   * here is to aggregate the alignment the user actually *see* in the wysiwyg
   * editor, so the aggregation follows the following priority of alignment:
   *
   *   1. inline css float.
   *   2. 'media-wysiwyg-align-*' class
   *   3. 'align' attribute.
   *   4. settings.alignment.
   */
  aggregateAlignmentFromAttributes: function() {
    var classes, rules;
    var alignment = '';
    var settings = this.getSettings();
    var css = {};

    // 3. alignment attribute
    if (settings.attributes.align == 'left' || settings.attributes.align == 'right') {
      settings.alignment = settings.attributes.align;
      delete settings.attributes.align;
    }

    // 2. classes
    if (settings.attributes.class) {
      classes = settings.attributes.class.split (' ');
      classes = classes.filter(function (className) {
        var match = className.match(/^media-wysiwyg-align-(left|right|center)$/);
        if (match) {
          alignment = match[1];
          return false;
        }
        return true;
      });
      if (alignment) {
        settings.alignment = alignment;
        if (classes) {
          settings.attributes.class = classes.join(' ');
        }
        else {
          delete settings.attributes.class;
        }
      }
    }

    // 1. inline style and float.
    if (settings.attributes.style) {
      rules = settings.attributes.style.split(';');
      rules.forEach(function(rule) {
        var i = rule.indexOf(':');
        var property = rule.slice(0, i).trim();
        var value = rule.slice(i + 1).trim();
        if (property && value) {
          css[property] = value;
        }
      });
      if (css.float && (css.float == 'left' || css.float == 'right')) {
        settings.alignment = css.float;
        delete css.float;
        rules = [];
        $.each(css, function(property, value) {
          rules.push(property + ':' + value);
        });
        if (rules.length) {
          settings.attributes.style = rules.join(';');
        }
        else {
          delete settings.attributes.style;
        }
      }
    }
  },

  /**
   * Sync attributes to fields.
   *
   * file_entity.module uses some fields to set certain attributes (alt, title)
   * on media elements. During wysiwyg editing these attributes may be
   * overridden and needs to be synced back to fields. This operation only syncs
   * values internally on the this.settings object between field properties and
   * the settings.attributes object.
   *
   * @param {bool} reverse
   *   Optional. Sync the oposite way: Sync fields to attributes.
   */
  syncAttributesToFields: function(reverse) {
    var fieldId;
    var settings = this.getSettings();

    if (!settings.attributes) {
      settings.attributes = {};
    }
    $.each(Drupal.settings.media.attributeFields, function(attribute, fieldName) {
      fieldId = Drupal.media.utils.propertyStartsWith(fieldName + '[', settings);
      if (!reverse) {
        settings[fieldId] = settings.attributes[attribute] ? settings.attributes[attribute] : '';
      }
      else {
        if (settings[fieldId]) {
          settings.attributes[attribute] = settings[fieldId];
        }
        else {
          delete settings.attributes[attribute];
        }
      }
    });
  },

  /**
   * Sync fields with attribute data to the settings.attributes property.
   */
  syncFieldsToAttributes: function() {
    this.syncAttributesToFields(true);
  },

  /**
   * Prepare this media instance settings object for tokenization.
   *
   * During a page request the media instance settings (formely known as
   * file_info) is passed around a lot and tweaked and modified by various
   * mechanisms. This method prepares this object for final stringifycation by
   * removing temporary, duplicate and otherwise unnecessary properties.
   *
   * @return {object}
   *   A cloned and cleaned version of the given object, ready for
   *   stringifycation and final output.
   */
  prepareSettingsForToken: function() {
    var classes, property, attribute, type, field;
    var pristine = {};
    var settings = this.getSettings();

    $.each(Drupal.settings.media.tokenSchema, function(property) {
      if (settings[property]) {
        pristine[property] = Drupal.media.utils.copyAsNew(settings[property]);
      }
    });
    if (settings.instance_fields === "override") {
      // Copy all overridable fields that belong to this file type.
      $.each(Drupal.media.WysiwygInstance.getOverridableFields(settings.fid), function(field) {
        $.each(settings, function(property) {
          if (property.startsWith(field + '[')) {
            pristine[property] = settings[property];
          }
        });
      });
    }
    if (pristine.attributes) {
      // Remove attributes already present as fields.
      $.each(Drupal.settings.media.attributeFields, function(attribute) {
        delete pristine.attributes[attribute];
      });
      // Internal data attributes.
      delete pristine.attributes['data-fid'];
      delete pristine.attributes['data-media-element'];
      delete pristine.attributes["data-media-key"];
      // Remove class names that otherwise are generated during wysiwyg or
      // server side input filtering.
      if (pristine.attributes.class) {
        classes = pristine.attributes.class.split(' ');
        classes = classes.filter(function(className) {
          return !(className == 'media-element'
                  || /^file-\S+$/.test(className)
                  || /^media-wysiwyg-align-(left|right|center)$/.test(className));
        });
        if (classes.length) {
          pristine.attributes.class = classes.join(' ');
        }
        else {
          delete pristine.attributes.class;
        }
      }
      // Finally, if the overall attributes object is empty, just remove it.
      if (!Object.keys(pristine.attributes).length) {
        delete pristine.attributes;
      }
    }
    return pristine;
  },

  /**
   * Return the overridden link title based on the file_entity title field set.
   *
   * @return {string}
   *   The overridden link_title or the existing link text if no overridden.
   */
  overrideLinkTitle: function() {
    var settings = this.getSettings();
    if (!Drupal.settings.media.attributeFields.title) {
      return settings.link_text;
    }
    var file_title_field_machine_name = '';
    $.each(settings, function(field, fieldValue) {
      if (field.indexOf(Drupal.settings.media.attributeFields.title) != -1) {
        file_title_field_machine_name = field;
      }
    });

    if (typeof settings[file_title_field_machine_name] != 'undefined' && settings[file_title_field_machine_name] != '') {
      return settings[file_title_field_machine_name];
    }
    return settings.link_text;
  }

};

//
// Utility functions related to media wysiwyg instances.
//

/**
 * Get a key suitable for indexing data related to this instance.
 *
 * This key have to be possible to recreate both server- and client side, and as
 * such, only the media token can be used as hash salt. Also, without any
 * wysiwyg editors attached the token itself is the only source and owner of the
 * current inserted media.
 *
 * The server-side equivalent is media_wysiwyg_get_token_key();
 *
 * @param {string} token
 *   The full media token to create key of.
 *
 * @return {string}
 *   Client/server-side compatible key suitable for indexing.
 */
Drupal.media.WysiwygInstance.createKey = function(token) {
  return 'token-' + Drupal.media.utils.hashCode(token);
};

/**
 * Get the map of overridable fields for given file ID.
 *
 * @param {number} fid
 *   The file ID to find overridable fields for.
 *
 * @return {object}
 *   A map of overriable fields and their status.
 */
Drupal.media.WysiwygInstance.getOverridableFields = function(fid) {
  var type, fields;
  if ((type = Drupal.settings.media.fidTypeMap[fid]) && (fields = Drupal.settings.media.overridableFields[type])) {
    return fields;
  }
  return {};
};

})(jQuery);
