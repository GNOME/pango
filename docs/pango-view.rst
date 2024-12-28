.. _pango-view(1):

==========
pango-view
==========

-----------------
Pango text viewer
-----------------

:Version: Pango
:Manual section: 1
:Manual group: Pango commands

SYNOPSIS
--------

|    **pango-view** [OPTIONS...] FILE

DESCRIPTION
-----------

``pango-view`` formats a text using Pango, with options to exercise
most of Pango's layout features.

HELP OPTIONS
------------

``-h, --help``

  Show help options.

``--help-all``

  Show all help options.

``--help-cairo``

   Show help for options understood by the cairo backend.

``--version``

  Print the command's version and exit.

CAIRO BACKEND OPTIONS
---------------------

``--annotate=FLAGS``

  Annotate the output. Comma-separated list of gravity, progression, baselines,
  layout, line, run, cluster, char, glyph, caret, slope

APPLICATION OPTIONS
-------------------

``--no-auto-dir``

  No layout direction according to contents.

``--backend=cairo/xft/ft2``

  Pango backend to use for rendering (default: cairo).

``--background=red/#rrggbb/#rrggbbaa/transparent``

  Set the background color.

``-q, --no-display``

  Do not display (just write to file or whatever).

``--dpi=number``

  Set the resolution.

``--align=left/center/right``

  Set text alignment.

``--ellipsize=start/middle/end``

  Set ellipsization mode.

``--font=description``

  Set the font description.

``--foreground=red/#rrggbb/#rrggbbaa``

  Set the text color.

``--gravity=south/east/north/west/auto``

  Set base gravity: glyph rotation.

``--gravity-hint=natural/strong/line``

  Set gravity hint.

``--header``

  Display the options in the output.

``--height=+points/-numlines``

  Set height in points (positive) or number of lines (negative) for ellipsizing.

``--hinting=none/auto/slight/medium/full``

  Set hinting style.

``--antialias=none/gray/subpixel``

  Set antialiasing.

``--hint-metrics=on/off``

  Set hint metrics.

``--subpixel-positions``

  Set subpixel positioning.

``--subpixel-order=rgb/bgr/vrgb/vbgr``

  Set subpixel order.

``--indent=points``

  Set paragraph indentation (in points).

``--spacing=points``

  Set line spacing (in points).

``--line-spacing=factor``

  Set spread factor for line height.

``--justify``

  Stretch paragraph lines to be justified.

``--justify-last-line``

  Justify the last line of the paragraph.

``--language=en_US/etc``

  Set language to use for font selection.

``--margin=CSS-style numbers in pixels``

  Set the margin on the output in pixels.

``--markup``

  Interpret text as Pango markup.

``-o, --output=file``

  Save rendered image to output file.

``--pixels``

  Use pixel units instead of points (sets dpi to 72).

``--pango-units``

  Use Pango units instead of points.

``--rtl``

  Set base direction to right-to-left.

``--rotate=degrees``

  Set angle at which to rotate results,

``-n, --runs=integer``

  Run Pango layout engine this many times.

``--single-par``

  Enable single-paragraph mode.

``-t, --text=string``

  Text to display (instead of a file).

``--waterfall``

  Create a waterfall display.

``-w, --width=points``

  Width in points to which to wrap lines or ellipsize.

``--wrap=word/char/word-char``

  Text wrapping mode (needs a width to be set).

``--serialized=FILE``

  Create layout from a serialized file,

``--serialize-to=FILE``

  Serialize result to a file.

