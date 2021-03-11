Title: Xft Fonts and Rendering

The Xft library is a library for displaying fonts on the X window
system; internally it uses the fontconfig library to locate font
files, and the FreeType library to load and render fonts. The
Xft backend is the recommended Pango font backend for screen
display with X. (The Cairo back end is another possibility.)

Using the Xft backend is generally straightforward;
[func@PangoXft.get_context] creates a context for a specified display
and screen. You can then create a [class@Pango.Layout] with that context
and render it with [func@PangoXft.render_layout]. At a more advanced
level, the low-level fontconfig options used for rendering fonts
can be affected using [func@PangoXft.set_default_substitute], and
[func@PangoXft.substitute_changed].

A range of functions for drawing pieces of a layout, such as individual
layout lines and glyphs strings are provided.  You can also directly
create a [class@PangoXft.Renderer]. Finally, in some advanced cases,
it is useful to derive from [class@PangoXft.Renderer]. Deriving from
[class@PangoXft.Renderer] is useful for two reasons. One reason is be
to support custom attributes by overriding `PangoRendererClass` virtual
functions like 'prepare_run' or 'draw_shape'. The other reason is to
customize exactly how the final bits are drawn to the destination by
overriding the `PangoXftRendererClass` virtual functions
'composite_glyphs' and 'composite_trapezoids'.
