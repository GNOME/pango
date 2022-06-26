---
Title: Migrating from Pango 1.x to Pango 2
---

Pango 2 is a major new version of Pango that breaks both API and ABI
compared to Pango 1.x. Therefore, some porting is required when switching
an application from Pango 1 to Pango 2. Thankfully, most of the changes
are not hard to adapt to.

## Preparation in Pango 1.x

The steps outlined in the following sections assume that your application
is working with a recent version of Pango 1.x, such as Pango 1.50. If you
are using an older version of Pango, you should first get your application
to build and work with Pango 1.50.

### Do not use deprecated symbols

Over the years, a number of functions and other APIs have been deprecated.
These deprecations are clearly spelled out in the API reference, with hints
about the recommended replacements.

To verify that your program does not use any deprecated symbols,
you can use the compiler flag

    -Wdeprecated-declarations

which will make the compiler warn for every use of a deprecated function.

### Stop using non-cairo drawing APIs

The platform-specific drawing APIs have been removed in favor of
cairo. If you are using pango_win32_render_layout(), pango_ft2_render_layout()
or similar APIs, you should port your application to use cairo for
text rendering.

## Changes that need to be done at the time of the switch

### Switch to the new prefix

Pango 2 has changed its prefix from `pango` to `pango2`, and all types
have been renamed from `Pango` to `Pango2` as well. Headers use the new
prefix as well:

    #include <pango2/pango.h>

The big advantage of this change is that the two libraries can be used
side-by-side in the same process without symbol or type clashes. The downside
is, of course, that you have to adapt your code to change all uses of
Pango API. Thankfully, this is largely a mechanical change.

### Build setup changes

Pango 2 ships as a single shared library. If you've used libpangocairo,
libpangoft2 or any of the other Pango 1.x libraries, you may be able to
simplify your build setup by only linking against libpango2. If you are
using pkgconfig (as you should), just use pango2.pc, going forward.

The cairo support is still optional, you can enable it with the `-Dcairo`
meson option. But if it is enabled, it no longer requires linking against
a separate shared library or using a separate pkgconfig file. Just include
the `pangocairo.h` header, and use the APIs that are declared in it.

There is still a `pango2cairo.pc` file, if you want to be explicit in your
build configuration about requiring cairo support. If you want to handle
the possible absence of cairo support at runtime, you can check the
`PANGO2_HAS_CAIRO` macro before including `pangocairo.h`.

The native fontmap implementations provide their own headers:
`pangofc-fontmap.h`, `pangocoretext-fontmap.h` and `pangodwrite-fontmap.h`.
If you want to check for the available fontmaps implementations
at runtime (even though they are entirely determined by the platform you
are running on, currently), you can check the `PANGO2_HAS_FC_FONTMAP`,
`PANGO2_HAS_CORE_TEXT_FONTMAP` and `PANGO2_HAS_DIRECT_WRITE_FONTMAP` macros
before including the corresponding header file.

### PangoFontMap changes

The [class@Pango2.FontMap] class has seen some significant changes. It is now possible
to instantiate a `PangoFontMap`, and populate it manually with `PangoFontFamily`
and `PangoFontFace` objects. If you simply want to obtain the default fontmap
that is populated with the native font enumeration APIs, use
[func@Pango2.FontMap.get_default]. It is no longer necessary to use cairo-specific
APIs to obtain a suitable fontmap, or set the resolution.

| Old API | New API |
|---------|---------|
|pango_cairo_font_map_get_default() | [func@Pango2.FontMap.get_default] |
|pango_cairo_font_map_set_resolution() | [method@Pango2.FontMap.set_resolution] |
|pango_font_map_create_context() | [ctor@Pango2.Context.new_with_font_map] |

### Font-related API changes

Some APIs have been moved between [class@Pango2.Font], [class@Pango2.FontFace]
and [class@Pango2.FontFamily] or renamed.

| Old API | New API |
|---------|---------|
| pango_font_face_get_face_name() | [method@Pango2.FontFace.get_name] |
| pango_font_family_is_monospace() | [method@Pango2.FontFace.is_monospace] |
| pango_font_family_is_variable() | [method@Pango2.FontFace.is_variable] |
| pango_font_get_languages() | [method@Pango2.FontFace.get_languages] |
| pango_font_has_char() | [method@Pango2.FontFace.has_char] |

The `PangoCoverage` object has been dropped, in favor of
[method@Pango2.FontFace.has_char].

The [struct@Pango2.FontMetrics] struct has been made opaque and given
copy/free semantics instead of ref/unref.

### PangoContext changes

[ctor@Pango2.Context.new] creates a `PangoContext` that uses the default
font map. To create a context with a custom font map, use
[ctor@Pango2.Context.new_with_font_map].

### Itemization-related API changes

[struct@Pango2.Item] and [struct@Pango2.Analysis] are now opaque structs
and have getters. APIs for creating items have been dropped.

### PangoLayout changes

Most APIs that provide information about the formatted output have been
moved from [class@Pango2.Layout] to the new objects [class@Pango2.Lines] and
[struct@Pango2.LineIter]. To obtain these from a `PangoLayout`, use
[method@Pango2.Layout.get_lines] and [method@Pango2.Layout.get_iter].

The `PangoLayoutLine` struct has been replaced by [struct@Pango2.Line],
and the role of `PangoLayoutIter` has been taken over by [struct@Pango2.LineIter].
The `PangoGlyphItem/PangoLayoutRun` struct has been replaced by a new
[struct@Pango2.Run].

| Old API | New API |
|---------|---------|
| pango_layout_get_extents() | [method@Pango2.Lines.get_extents] |
| pango_layout_get_size() | [method@Pango2.Lines.get_size] |
| pango_layout_get_pixel_extents() | use [func@Pango2.extents_to_pixels] |
| pango_layout_get_pixel_size() | use [func@Pango2.extents_to_pixels] |
| pango_layout_get_baseline() | [method@Pango2.Lines.get_baseline] |
| pango_layout_get_line_count() | [method@Pango2.Lines.get_line_count] |
| pango_layout_get_log_attrs_readonly() | [method@Pango2.Layout.get_log_attrs] |
| pango_layout_set_single_paragraph_mode() | [method@Pango2.Layout.set_single_paragraph] |
| pango_layout_is_wrapped() | [method@Pango2.Lines.is_wrapped] |
| pango_layout_is_ellipsized() | [method@Pango2.Lines.is_ellipsized] |
| pango_layout_xy_to_index() | [method@Pango2.Lines.pos_to_index] |
| pango_layout_get_cursor_pos() | [method@Pango2.Lines.get_cursor_pos] |
| pango_layout_get_caret_pos() | [method@Pango2.Lines.get_caret_pos] |
| pango_layout_move_cursor_visually() | [method@Pango2.Lines.move_cursor] |
| pango_layout_line_get_extents() | [method@Pango2.Line.get_extents] |
| pango_layout_line_get_x_ranges() | [method@Pango2.Lines.get_x_ranges] |
| pango_layout_line_x_to_index() | [method@Pango2.Line.x_to_index] |
| pango_layout_iter_get_layout_extents() | [method@Pango2.LineIter.get_layout_extents] |
| pango_layout_iter_get_line_yrange() | [method@Pango2.LineIter.get_line_extents] |

### PangoAttribute changes

[struct@Pango2.Attribute] is no longer defined with different structs,
but is an opaque type with getters for the different types of value.
To set the range of an attribute use [method@Pango2.Attribute.set_range].
To define your own attribute types, use [func@Pango2.AttrType.register].

Some of the existing attribute types have seen changes as well. All the
color-related attributes now take a [struct@Pango2.Color] argument in their
constructor. Alpha attributes have been removed, since [struct@Pango2.Color]
now contains an alpha field.

The line-related attributes have been reorganized. There is now a
[enum@Pango2.LineStyle] enumeration that can be applied to underlines,
overlines and strikethroughs, and a separate [enum@Pango2.UnderlinePosition]
enumeration.

### Markup changes

The attributes 'alpha', 'fgalpha', 'background_alpha' and 'bgalpha' have
been removed. Alpha values can now be specified as part of color attributes.

The 'underline', 'overline' and 'strikethrough' attributes now all take values
from the [enum@Pango2.LineStyle] enumeration. The `PANGO_UNDERLINE_LOW` value
has been replaced by the separate 'underline-position' attribute.

## New functionality to explore

### Complex line breaking with PangoLineBreaker

[class@Pango2.LineBreaker] is the core of pango's line-breaking algorithm,
broken out from `PangoLayout`. It gives applications more direct access to
influence the operation of the line-breaking algorithm, and facilitates
complex line breaking tasks such as shaped paragraphs or multi-column layout.

To create a `PangoLineBreaker`, use [ctor@Pango2.LineBreaker.new]. To add text
to it, use [method@Pango2.LineBreaker.add_text], and to obtain formatted lines,
use [method@Pango2.LineBreaker.next_line].

### Application-specific fonts

[class@Pango2.HbFace] makes it easily possible to create fonts from font files
that are shipped with an application or even from font data in memory. These
custom faces can then be added to any fontmap with [method@Pango2.FontMap.add_face],
to make them available like any other font.

### Custom fonts

[class@Pango2.UserFace] is a callback-based implementation of fonts that allows
for entirely appication-defined font handling, including glyph drawing.
