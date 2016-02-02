(function ($) {
    Drupal.behaviors.mjh_views_alter = {
        attach: function () {

            $("button.location-gmap-find-address-button").click(function (e) {
                e.preventDefault();
                var address_parts = new Array();
                $("fieldset#edit-" + $(this).val() + " .form-item input[type=text]," +
                    "fieldset#edit-" + $(this).val() + " .form-item select > option:selected").each(function () {
                        if (!$(this).hasClass('gmap-control') && $(this).val() != '') {
                            if ($(this).is('option')) {
                                address_parts.push($(this).text());
                            } else {
                                address_parts.push($(this).val());
                            }
                        }
                    });

                var address_string = address_parts.join(', ');
                var gmap_id = $("fieldset#edit-" + $(this).val() + " .gmap-map").attr('id');
                if (google.maps.version !== 'undefined') { // assume Google Maps API v3 as API v2 did not have this variable
                    var geocoder = new google.maps.Geocoder();
                    geocoder.geocode({'address': address_string}, function (results, status) {
                        if (status == google.maps.GeocoderStatus.OK) {
                            var m = Drupal.gmap.getMap(gmap_id);
                            m.locpick_coord = results[0].geometry.location;
                            m.change('locpickchange');
                            m.map.setCenter(results[0].geometry.location);
                            m.map.setZoom(14);
                        }
                        else {
                            alert(Drupal.t("Your address was not found."));
                        }
                    });
                }
                else {
                    var geocoder = new GClientGeocoder();
                    geocoder.reset(); // Clear the client-side cache
                    geocoder.getLatLng(address_string, function (point) {
                        if (!point) {
                            alert(Drupal.t("Your address was not found."));
                        }
                        else {
                            var m = Drupal.gmap.getMap(gmap_id);
                            m.locpick_coord = point;
                            m.change('locpickchange');
                            m.map.setCenter(point, 14);

                        }
                    });
                }

            });

        }

    }
})(jQuery);

