Feeds extensible parsers
========================

A set of extensible parsers for Feeds.
http://drupal.org/project/feeds_ex

Provided parsers
================
- XPath XML & HTML
- QueryPath XML & HTML (requires the QueryPath module)
- JSONPath JSON & JSON lines parser (requires JSONPath)
- JMESPath JSON & JSON linesparser (requires JMESPath and PHP 5.4)

Requirements
============

- Feeds
  http://drupal.org/project/feeds

Installation
============

- Download and enable just like a normal module.

QueryPath
=========
To use the QueryPath parsers, download and enable the QueryPath module.
http://drupal.org/project/querypath

JSONPath
========
To use the JSONPath parser, you'll need PHP >= 5.4.

There are two ways to install it:

1. Without composer:

  1. Download and enable the modules "libraries" and "xautoload".
  2. Download and unpack the following file
     https://github.com/FlowCommunications/JSONPath/archive/0.5.0.tar.gz
     and place the contents of JSONPath-0.5.0 in sites/all/libraries/jsonpath

2. With composer, in sites/all/libraries:

   $ git clone https://github.com/FlowCommunications/JSONPath.git jsonpath
   $ cd jsonpath
   $ composer install --no-dev
   $ drush cc all # Or just clear the cache however you normally would.

JMESPath
========
To use the JMESPath parsers, you'll need Composer, and PHP >= 5.4.
In sites/all/libraries:

$ git clone https://github.com/mtdowling/jmespath.php.git
$ cd jmespath.php
$ composer install --no-dev
$ drush cc all # Or just clear the cache however you normally would.

Development
===========
To run tests locally, the TUnit module is required.
http://drupal.org/project/tunit
One of the goals of this module is to allow other developers to easily create
certain types of parsers. For example, it should be trivial to create a parser
for a specific XML format.
Documention to come.

History
=======
This is the new home for:
- Feeds XPath Parser
- Feeds JSONPath Parser
- Feeds QueryPath Parser
The above modules are all in various states of maintenance. This project is
meant to combine them so that they can benefit from each other's development and
simplify maintenance. There are also some major architectural changes.

There is no upgrade path for the old modules. It remains on the table, but those
modules are still maintained and should continue to work.
