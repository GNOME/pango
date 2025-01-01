---
Title: Fonts and Glyphs
---

# Fonts

Pango supports a flexible architecture where platform-specific code can supply
an implementation of font enumeration. The `Pango2Font` structure represents a
rendering-system-independent font. Pango provides routines to list available
fonts, and to load a font matching a given description.

Conceptually, Pango groups fonts into faces and families which are identified
by a name. A *font face* provides the different sizes of a single font style.
A *font family* provides the available styles of a font.

As an example, "Helvetica" is a family, "Helvetica Bold" is a face of this
family, and "Helvetica Bold 12pt" is a concrete font of this face.

# Font Enumeration

The central object for dealing with the available fonts on a system and caching
loaded fonts is a [class@Pango2.FontMap]. An application typically uses a single
font map.

Since the font map depends on the platform in use, you'll need to use the function
[func@Pango2.FontMap.get_default] to obtain the default fontmap. Depending on the
platform, it will return a `Pango2FcFontMap`, a `Pango2DirectWriteFontMap`
or a `Pango2CoreTextFontMap`.

Once you have a fontmap, you can enumerate the available font families using
the [iface@Gio.ListModel] implementation of the fontmap. To enumerate the faces
of a font family, use the [iface@Gio.ListModel] implementation of the font family.

# Custom Fonts

The default font map used by Pango will contain the fonts that are available
via the font enumeration APIs of the system (for Linux, that is fontconfig).
For special situations (such as writing Pango tests), it can appropriate
to create an empty font map with [ctor@Pango2.FontMap.new] and populate it
only with the fonts you need, using [method@Pango2.FontMap.add_file].

It is also possible to add custom fonts to the default font map if you
just want to make some custom font available in addition to the normal
system fonts. While loading a font from a .ttf or .otf file with
[method@Pango2.FontMap.add_file] is often the most convenient way to add
a custom font, it is also possible to load a font from memory by combining
[ctor@Pango2.HbFace.new_from_hb_face] and `hb_face_create()`.

Another approach to custom fonts is to draw the glyphs yourself. This
is possible with [class@Pango2.UserFace]. Such font faces can also be
added to font maps and used like regular font faces.

# Font Descriptions

Since loading fonts uses system resources, Pango provides a way to describe
a font without loading it. A [struct@Pango2.FontDescription] is a struct that
contains enough information to load a concrete font with
[method@Pango2.FontMap.load_font] or [method@Pango2.Context.load_font]. You can
obtain a font description from a font face using [method@Pango2.FontFace.describe],
or by parsing a string such as

    Helvetica Bold 12pt

with [func@Pango2.FontDescription.from_string].

# Glyphs

A font provides information about glyphs and how to position and render them.
The Pango rendering pipeline uses this information to create a
[struct@Pango2.GlyphString], which contains the glyphs corresponding to the
characters in the text and related information such as glyph positions and sizes,
and clustering information (i.e. which glyphs correspond to which characters).

![A glyph string](rects3.png)

A glyph is identified by a [alias@Pango2.Glyph], which is a numeric ID. Note that
glyph IDs are font-specific: the same character can be represented by different
glyph IDs in different fonts.

The mapping between characters and glyphs is in general neither 1-1 nor a map:
a single glyph may represent multiple characters (as is the case with ligatures),
a single character may be represented by multiple glyphs (for example, when
combining accents and base character), and in complex scripts, multiple characters
may form clusters that get rearranged and represented by multiple glyphs.
