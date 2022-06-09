Pango
=====

Pango is a library for layout and rendering of text, with an emphasis
on internationalization. Pango can be used anywhere that text layout
is needed; however, most of the work on Pango so far has been done using
the GTK widget toolkit as a test platform. Pango forms the core of text
and font handling for GTK.

Pango is using native mechanism to enumerate fonts on each platform
that it supports. There are three basic backends

- Using FontConfig on Linux
- Using DirectWrite on Microsoft Windows
- Using the CoreText framework on MacOS

The integration of Pango with [Cairo](https://cairographics.org)
provides a complete solution with high quality text handling and
graphics rendering.

As well as the low level layout rendering routines, Pango includes
PangoLayout, a high level driver for laying out entire blocks of text,
and routines to assist in editing internationalized text.

For more information about Pango, see:

 https://www.pango.org/

Dependencies
------------
Pango depends on the GLib library; more information about GLib can
be found at https://www.gtk.org/.

To use the Free Software stack backend, Pango depends on the following
libraries:

- [FontConfig](https://www.fontconfig.org) for font discovery
- [HarfBuzz](http://www.harfbuzz.org) for complex text shaping
- [fribidi](http://fribidi.org) for bidirectional text handling

Optionally, Pango can use [libthai](https://linux.thai.net/projects/libthai)
for handling special needs of Thai script.

Rendering support depends on the [Cairo](https://cairographics.org) library.
The Cairo renderer is the preferred renderer to use Pango with and is
subject of most of the development in the future.  It has the
advantage that the same code can be used for display and printing.

It is also possible to use HarfBuzz's hb-draw APIs to receive the
data for glyph contours, and render them with any graphics API that
supports Beziér curves.

For details about installation of Pango on Win32 see README.win32.

License
-------
Most of the code of Pango is licensed under the terms of the
GNU Lesser Public License (LGPL) - see the file COPYING for details.
