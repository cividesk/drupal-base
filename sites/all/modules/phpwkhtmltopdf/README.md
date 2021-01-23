After lots of frustration and not-close-enough results with other HTML to PDF modules & libraries, I decided to make a Drupal implementation of something I know works extremely well.

IMHO: WkHtmlToPdf is just fantastic and works much better than things like mPDF.

The Print module was way more than I needed and still would have required custom work for my requirements.

**This is intended for module developers, nothing is usable from the front-end.**

**2.x now has two additional dependencies, review the installation steps below for details.** You can also view your site's "/admin/reports/status" page
for more details on the libraries installation status.

This module is a Drupal Library implementation for the PHPWkHtmlToPdf wrapper and as a result is extremely small because it uses WkHtmlToPdf to do the heavy work.

## Quick Start

The following example is an adaption from the official PhpWkHtmlToPdf docs located at: https://github.com/mikehaertl/phpwkhtmltopdf

```php
if (($library = libraries_load('phpwkhtmltopdf')) && !empty($library['loaded'])) {
    $pdf = new WkHtmlToPdf();

    // Add a HTML file, a HTML string or a page from a URL
    $pdf->addPage('/home/joe/page.html');
    $pdf->addPage('<html>....</html>');
    $pdf->addPage('http://google.com');

    // Add a cover (same sources as above are possible)
    $pdf->addCover('mycover.html');

    // Save the PDF
    $pdf->saveAs('/tmp/new.pdf');

    // ... or send to client for inline display
    $pdf->send();

    // ... or send to client as file download
    $pdf->send('test.pdf');
}
```

## Requirements

### WkHtmlToPdf Installed
You must have WkHtmlToPdf installed and working on the server before attempting to use this wrapper library.
See https://github.com/wkhtmltopdf/wkhtmltopdf for installation details if my unofficial instructions don't work for you.

#### TLDR
*Basically you'll want to [download the latest release package](http://wkhtmltopdf.org/downloads.html)* and install it via your package manager.
Example: `sudo dpkg -i wkhtmltox-0.12.1_linux-precise-amd64.deb`

### PhpWkHtmlToPdf Library
You will need to make sure the PhpWkHtmlToPdf library and its dependencies have been installed in the sites/all/library/phpwkhtmltopdf folder. See Step 2 of my install instructions below.

## Installation

Please note: these are my own instructions for my install on Ubuntu 12.04.2 LTS 64-bit.

### Step 1 - (My Unofficial) WkHtmlToPdf Install Instructions
```bash
cd ~
wget http://downloads.sourceforge.net/project/wkhtmltopdf/0.12.1/wkhtmltox-0.12.1_linux-precise-amd64.deb
sudo dpkg -i wkhtmltox-0.12.1_linux-precise-amd64.deb
```

*See https://github.com/wkhtmltopdf/wkhtmltopdf/blob/master/INSTALL.md for more detailed instructions on installing from source.*

### Step 2 - Install PhpWkHtmlToPdf to Library

```bash
cd [siteroot]/sites/all/libraries
wget https://github.com/mikehaertl/phpwkhtmltopdf/archive/2.0.1.tar.gz
tar -zxf 2.0.1.tar.gz
mv phpwkhtmltopdf-* phpwkhtmltopdf
```
__2.x.x Version:__ The author of the PHPWkHtmlToPdf library has added 2 dependency libraries that you'll need to also copy to your libraries folder.

```bash
wget https://github.com/mikehaertl/php-shellcommand/archive/1.0.3.tar.gz
tar -zxf 1.0.3.tar.gz
mv php-shellcommand-* php-shellcommand

wget https://github.com/mikehaertl/php-tmpfile/archive/1.0.0.tar.gz
tar -zxf 1.0.0.tar.gz
mv php-tmpfile-* php-tmpfile
```

You basically want to make sure you have the following files:
  * `[siteroot]/sites/all/libraries/phpwkhtmltopdf/src/Pdf.php`
  * `[siteroot]/sites/all/libraries/php-shellcommand/src/Command.php`
  * `[siteroot]/sites/all/libraries/php-tmpfile/src/File.php`

Note: Your site's `libraries` folder may be located somewhere else depending on your configuration. For example, some distributions
such as Commons place it in [siteroot]/profiles/commons/libraries

### Step 3 - Enable this Module
Then install/enable this module like any other Drupal module.

### Step 4 - Verify libraries are installed
Verify the libraries are installed correctly by visiting your sites Status page (admin/reports/status).
You should see a reported version for the following items:
  * PHPWkHtmlToPdf
  * php-shellcommand
  * php-tmpfile

## Credits

A very special thanks goes out to the maintainers of WkHtmlToPDF and to authors of the PHP wrapper.

See WkHtmlTpPDF Contributors:
https://github.com/wkhtmltopdf/wkhtmltopdf/graphs/contributors

See PHP Wrapper Contributors:
https://github.com/mikehaertl/phpwkhtmltopdf/graphs/contributors
