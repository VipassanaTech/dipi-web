# services_auth_apikeys
# README file for Services API Keys Authetication

CONTENTS OF THIS FILE
---------------------
* Introduction
* Requirements
* Installation
* Configuration
* How it works
* Troubleshooting
* Maintainers

INTRODUCTION
------------
Extend Services (https://www.drupal.org/project/services) module authentication.

This module allows authentication towards a web service, using API keys parameters
for the service requests.
API Keys authentication: check if the provided keys are valid,
they exist and belong to the right user (role and permission).
Individual API keys can be generated for each user and API keys authentication
can be controlled on a per role basis.


REQUIREMENTS
------------
This module requires the following modules:
* Services (https://www.drupal.org/project/services)


INSTALLATION
------------
* Install as you would normally install a contributed drupal module.
  See: https://drupal.org/documentation/install/modules-themes/modules-7
  for further information.


CONFIGURATION
-------------
* Enable "API keys authentication" for a Service Endpoint.
* Authentication (tab) for the Service Endpoint.
  3 API keys available : API key, token, extra key (optional).
  Foreach API key there are some settings that could be configured:
  - Parameter type: How this API key should be sent in the request,
    for example: GET, POST, HTTP Header.
  - Identifier: What identifier do you want to use in the service request for this key.
  - Generate option: User will be able to update this key - generate new key.
  - Enable Extra key (for Extra key only): to enable a third key if needed.
* Resources (tab) for the Service Endpoint settings
  Foreach operations there will some settings available to extend the API keys authentication:
  - Disable API keys authentication: there will be no API keys authentication for this service request.
  - Owner login: Atfer the authentication the owner of the API keys becomes current user.
  - Access - Deny Access: available foreach user role with the "Use API keys authentication" permission.
    It will deny access for the selected user roles.
* User Services API keys (user tab): User API keys management page.
  Foreach service enpoint with "API keys authentication" enabled, we have:
  - Generate API Keys (button) - available only if there is no API keys were defined yet.
  - API Keys : current API keys values and settings.
  - Operate on existing API keys: Extra checkbox for user to validate that it wants
    to edit/delete/generate the API keys.
  - Generate new API key (button): available only for the API keys that has this option enabled.
  - Delete API keys: empty the API keys for the given service endpoint,
    equivalent of disabling the service from user.
  - Edit manually: checkbox to allow manual editing of the API keys.
  - Edit (manually) API Keys (subform): a sub(form) for editing manually the existing API keys.
    This is available only if the "Edit manually" checkbox is checked.
    Every API key element has validation so the values should meet be as the one generated.
    @see drupal_hash_base64().


HOW IT WORKS
------------
Authenticate with User API keys based on the existing configuration.
There could be 3 cases for a service endpoint request:
* No authentication: if it is disabled for the request.
* API Keys authentication: check if the provided keys are valid,
  they exist and belong to the right user (role and permission).
  - Owner login: there is an option to login for the API keys owner/user.
* Access denied: no access if the Deny access is set for the service request and user role.


TROUBLESHOOTING
---------------
nothing yet.


MAINTAINERS
-----------
Current maintainers:
* Tavi Toporjinschi (vasike) - https://www.drupal.org/u/vasike

This project has been developed by:
* Commerce Guys
  Commerce Guys are the creators of and experts in Drupal Commerce,
  the eCommerce solution that capitalizes on the virtues and power of Drupal,
  the premier open-source content management system.
  We focus our knowledge and expertise on providing online merchants with
  the powerful, responsive, innovative eCommerce solutions they need to thrive.
  Visit https://commerceguys.com/ for more information.
