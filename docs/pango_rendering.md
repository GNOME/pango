---
Title: The Rendering Pipeline
---

# The Rendering Pipeline

The Pango rendering pipeline takes a string of Unicode characters, converts them
it into glyphs, and renders them on some output medium. This section describes the
various stages of this pipeline and the APIs that implement them.

<picture>
  <source srcset="pipeline-dark.png" media="(prefers-color-scheme: dark)">
  <img alt="Pango Rendering Pipeline" src="pipeline-light.png">
</picture>

Itemization
: breaks a piece of text into segments with consistent direction and shaping
  properties. Among other things, this determines which font to use for each
  character. Use [func@Pango2.itemize] to itemize text.

Shaping
: converts characters into glyphs. Use [func@Pango2.shape] or
  [func@Pango2.shape_item] to shape text.

Line Breaking
: determines where line breaks should be inserted into a sequence of glyphs.
  The functions [func@Pango2.default_break], [func@Pango2.tailor_break] and
  [func@Pango2.attr_break] determine possible line breaks. The actual line
  breaking is done by [class@Pango2.LineBreaker].

Justification
: adjusts inter-word spacing to form lines of even length. This is done by
  [struct@Pango2.Line].

Rendering
: takes a string of positioned glyphs, and renders them onto a surface.
  This is accomplished by a [class@Pango2.Renderer] object. The functions
  [func@Pango2.cairo_show_glyph_string] and [func@Pango2.cairo_show_layout]
  use a [class@Pango2.Renderer] to draw text onto a cairo surface.
