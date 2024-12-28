.. _pango-segmentation(1):

==================
pango-segmentation
==================

-----------------------
Pango text segmentation
-----------------------

:Version: Pango
:Manual section: 1
:Manual group: Pango commands

SYNOPSIS
--------

|    **pango-segmentation** [OPTIONS...] [FILE]

DESCRIPTION
-----------

``pango-segmentation`` shows text boundaries determined by Pango.

These can be graphemes (cursor positions), words boundaries, word
starts and ends, line break possibilities, sentence boundaries, or
sentence starts and ends.

HELP OPTIONS
------------

``-h, --help``

  Show help options.

``--version``

  Print the command's version and exit.

APPLICATION OPTIONS
-------------------

``--text=STRING``

  Text to display (instead of ``FILE``).

``--kind=KIND``

  The boundary to show. ``KIND`` can be one of grapheme, word, words, line, sentence,
  sentences. (default: grapheme).
