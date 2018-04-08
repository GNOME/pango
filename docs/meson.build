subdir('xml')

version_xml_conf = configuration_data()
version_xml_conf.set('PANGO_VERSION', meson.project_version())
configure_file(input: 'version.xml.in',
               output: 'version.xml',
               configuration: version_xml_conf)

private_headers = [
  'pangoatsui-private.h',
  'pangocairo-private.h',
  'pangocairo-fc.h',
  'pangocairo-win32.h',
  'pangocairo-atsui.h',
  'pango-color-table.h',
  'pango-engine-private.h',
  'pango-impl-utils.h',
  'pango-glyph-item-private.h',
  'pango-layout-private.h',
  'pango-script-table.h',
  'pango-script-private.h',
  'pangofc-private.h',
  'pangoft2-private.h',
  'pangowin32-private.h',
  'pangoatsui-private.h',
  'pangox-private.h',
  'pangoxft-private.h',
  'pango-ot-private.h',
  'pango-script-lang-table.h',
  'pango-language-sample-table.h',
  'pangocoretext-private.h',
  'pangoatsui-private.h',
]

if host_system != 'darwin'
  private_headers += [
    'pangocairo-coretextfont.h',
    'pangocairo-coretext.h',
    'pangocoretext.h',
  ]
endif

if host_system != 'windows'
  private_headers += [
    'pangocairo-win32.h',
    'pangowin32.h',
  ]
else
  private_headers += [
    'pangoxft.h',
    'pangoxft-render.h',
  ]
endif

content_files = [
  'pango_markup.sgml',
  'version.xml',
]

html_images = [
  'layout.gif',
  'rotated-text.png',
]

docdeps = [ libpango_dep ]

if build_pangoft2
  docdeps += libpangoft2_dep
endif

if xft_dep.found()
  docdeps += libpangoxft_dep
endif

if cairo_dep.found()
  docdeps += libpangocairo_dep
endif

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
cairo_prefix = dependency('cairo').get_pkgconfig_variable('prefix')
cairo_docpath = join_paths(cairo_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(pango_datadir, 'gtk-doc', 'html')

gnome.gtkdoc('pango',
             main_xml: 'pango-docs.sgml',
             src_dir: [
               join_paths(meson.source_root(), 'pango'),
               join_paths(meson.build_root(), 'pango'),
             ],
             dependencies: docdeps,
             gobject_typesfile: 'pango.types',
             scan_args: [
               '--rebuild-types',
               '--deprecated-guards=PANGO_DISABLE_DEPRECATED',
               '--ignore-decorators=_PANGO_EXTERN',
               '--ignore-headers=' + ' '.join(private_headers),
             ],
             mkdb_args: [
               '--sgml-mode',
               '--output-format=xml',
             ],
             fixxref_args: [
               '--html-dir=@0@'.format(docpath),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
               '--extra-dir=@0@'.format(join_paths(cairo_docpath, 'cairo')),
             ],
             html_assets: html_images,
             install: true)
