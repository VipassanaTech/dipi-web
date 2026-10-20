/**
 * Teacher profile mobiles (address-book.inc): the international telephone widget, as on the schedule
 * site's forms. Choose a country (India first), type your own number; "+91" is shown beside the flag.
 * The chosen country goes into a hidden field, so the server reads a number typed without +code for
 * that country; a valid number is also sent in full (+9198…). The server checks every number again
 * (dh_atportal_phone_parse) and keeps the full international form.
 */
(function ($, Drupal) {
  'use strict';

  Drupal.behaviors.dhIntlTel = {
    attach: function (context) {
      if (!window.intlTelInput) {
        return;
      }
      $(context).find('input.dh-intl-tel').once('dh-intl-tel').each(function () {
        var input = this;
        var region = document.getElementById(input.getAttribute('data-region-field'));
        var iti = window.intlTelInput(input, {
          initialCountry: input.getAttribute('data-region') || 'in',
          preferredCountries: ['in'],
          separateDialCode: true,
          nationalMode: true,
          autoPlaceholder: 'aggressive',
          utilsScript: Drupal.settings.dhIntlTel.utils
        });
        function sync() {
          var c = iti.getSelectedCountryData();
          if (region && c && c.iso2) {
            region.value = c.iso2.toUpperCase();
          }
        }
        input.addEventListener('countrychange', sync);
        sync();
        $(input.form).on('submit', function () {
          if (!$.trim(input.value)) {
            return;
          }
          sync();
          try {
            if (iti.isValidNumber()) {
              var full = iti.getNumber();
              if (full) {
                input.value = full;
              }
            }
          }
          catch (e) {}
        });
      });
    }
  };
})(jQuery, Drupal);
