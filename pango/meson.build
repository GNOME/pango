pango_sources = [
  'break.c',
  'ellipsize.c',
  'fonts.c',
  'glyphstring.c',
  'itemize.c',
  'pango-attributes.c',
  'pango-bidi-type.c',
  'pango-color.c',
  'pango-context.c',
  'pango-coverage.c',
  'pango-emoji.c',
  'pango-font-description.c',
  'pango-font-face.c',
  'pango-font-family.c',
  'pango-font-metrics.c',
  'pango-fontmap.c',
  'pango-fontset.c',
  'pango-fontset-simple.c',
  'pango-glyph-item.c',
  'pango-gravity.c',
  'pango-item.c',
  'pango-language.c',
  'pango-layout.c',
  'pango-markup.c',
  'pango-matrix.c',
  'pango-renderer.c',
  'pango-script.c',
  'pango-tabs.c',
  'pango-utils.c',
  'reorder-items.c',
  'shape.c',
  'serializer.c',
  'json/gtkjsonparser.c',
  'json/gtkjsonprinter.c',
]

pango_headers = [
  'pango.h',
  'pango-attributes.h',
  'pango-break.h',
  'pango-color.h',
  'pango-context.h',
  'pango-coverage.h',
  'pango-direction.h',
  'pango-font.h',
  'pango-font-description.h',
  'pango-font-face.h',
  'pango-font-family.h',
  'pango-font-metrics.h',
  'pango-fontmap.h',
  'pango-fontset.h',
  'pango-fontset-simple.h',
  'pango-glyph.h',
  'pango-glyph-item.h',
  'pango-gravity.h',
  'pango-item.h',
  'pango-language.h',
  'pango-layout.h',
  'pango-matrix.h',
  'pango-markup.h',
  'pango-renderer.h',
  'pango-script.h',
  'pango-tabs.h',
  'pango-types.h',
  'pango-utils.h',
]

pango_gir_includes = [
  'HarfBuzz-0.0',
  'GObject-2.0',
]

if cairo_dep.found()
  pango_headers += [
    'pangocairo.h',
  ]

  pango_sources += [
    'pangocairo-context.c',
    'pangocairo-font.c',
    'pangocairo-fontmap.c',
    'pangocairo-render.c',
  ]

  if pango_font_backends.contains('freetype')
    pango_sources += [
      'pangocairo-fcfont.c',
      'pangocairo-fcfontmap.c',
    ]
  endif

  if pango_font_backends.contains('win32')
    pango_sources += [
      'pangocairo-win32font.c',
      'pangocairo-win32fontmap.c',
    ]
  endif

  if pango_font_backends.contains('quartz')
    pango_sources += [
      'pangocoretext.c',
      'pangocoretext-fontmap.c',
      'pangocairo-coretextfont.c',
      'pangocairo-coretextfontmap.c',
    ]
  endif

  pango_gir_includes += [
    'cairo-1.0',
  ]
endif

if build_pangoft2
  pango_headers += [
    'pangofc-font.h',
    'pangofc-fontmap.h',
    'pangofc-decoder.h',
  ]

  pango_sources += [
    'pangofc-font.c',
    'pangofc-fontmap.c',
    'pangofc-decoder.c',
    'pango-trace.c',
  ]

  pango_gir_includes += [
    'fontconfig-2.0',
    'freetype2-2.0',
  ]
endif

if host_system == 'windows'
  pango_headers += [
    'pangowin32.h',
  ]

  pango_sources += [
    'pangowin32.c',
    'pangowin32-fontcache.c',
    'pangowin32-fontmap.c',
  ]

  pango_deps += [
    cc.find_library('gdi32'),
  ]

  pangowin32_rc = configure_file(
    input: 'pangowin32.rc.in',
    output: 'pangowin32.rc',
    configuration: pango_features_conf,
  )
  pangowin32_res = import('windows').compile_resources(pangowin32_rc)
  pango_sources += pangowin32_res
endif

pango_installed_headers = pango_headers + [ 'pango-version-macros.h' ]

install_headers(pango_installed_headers, subdir: pango_api_path)

# Features header
pango_features_conf = configuration_data()
pango_features_conf.set('PANGO_VERSION_MAJOR', pango_major_version)
pango_features_conf.set('PANGO_VERSION_MINOR', pango_minor_version)
pango_features_conf.set('PANGO_VERSION_MICRO', pango_micro_version)
pango_features_conf.set('PANGO_API_VERSION', pango_api_version)
pango_features_conf.set('PANGO_CURRENT_MINUS_AGE', '0')

pango_features_h = configure_file(
  input: 'pango-features.h.meson',
  output: 'pango-features.h',
  configuration: pango_features_conf,
  install: true,
  install_dir: join_paths(pango_includedir, pango_api_path),
)

# Enumerations for GType
pango_enums = gnome.mkenums(
  'pango-enum-types',
  sources: pango_headers,
  c_template: 'pango-enum-types.c.template',
  h_template: 'pango-enum-types.h.template',
  install_dir: join_paths(pango_includedir, pango_api_path),
  install_header: true,
)
pango_enum_h = pango_enums[1]

if host_system == 'windows'
  pango_rc = configure_file(
    input: 'pango.rc.in',
    output: 'pango.rc',
    configuration: pango_features_conf,
  )
  pango_res = import('windows').compile_resources(pango_rc)
  pango_sources += pango_res
endif

pango_cflags = [
  '-DG_LOG_DOMAIN="Pango"',
  '-DG_LOG_USE_STRUCTURED=1',
  '-DPANGO_COMPILATION',
  '-DSYSCONFDIR="@0@"'.format(pango_sysconfdir),
  '-DLIBDIR="@0@"'.format(pango_libdir),
]

pango_inc = include_directories('.')

libpango = library(
  pango_api_name,
  sources: pango_sources + pango_enums,
  version: pango_libversion,
  soversion: pango_soversion,
  darwin_versions : pango_osxversion,
  install: true,
  dependencies: pango_deps,
  include_directories: [ root_inc, pango_inc ],
  c_args: common_cflags + pango_debug_cflags + pango_cflags,
  link_args: common_ldflags,
)

pango_dep_sources = [ pango_enum_h ]

gir = find_program('g-ir-scanner', required : get_option('introspection'))
build_gir = gir.found() and (not meson.is_cross_build() or get_option('introspection').enabled())

if build_gir
  gir_args = [
    '--quiet',
  ]
  harfbuzz_gobject_dep = dependency('harfbuzz-gobject',
                                    version: harfbuzz_req_version,
                                    required: false,
                                    fallback: ['harfbuzz', 'libharfbuzz_gobject_dep'])

  if harfbuzz_gobject_dep.found()
    pango_deps += harfbuzz_gobject_dep
  endif
  pango_gir = gnome.generate_gir(
    libpango,
    sources: pango_sources + pango_headers + [ pango_enum_h, pango_features_h ],
    namespace: 'Pango',
    nsversion: pango_api_version,
    identifier_prefix: 'Pango',
    symbol_prefix: 'pango',
    export_packages: 'pango',
    dependencies: pango_deps,
    includes: pango_gir_includes,
    header: 'pango/pango.h',
    install: true,
    extra_args: gir_args,
  )
  pango_gir_dep = declare_dependency(sources: pango_gir)
  pango_dep_sources += pango_gir
endif

libpango_dep = declare_dependency(
  link_with: libpango,
  include_directories: pango_inc,
  dependencies: pango_deps,
  sources: pango_dep_sources,
)
meson.override_dependency('pango', libpango_dep)

pango_pkg_requires = ['gobject-2.0']
if harfbuzz_dep.type_name() == 'pkgconfig'
  pango_pkg_requires += 'harfbuzz'
endif

pkgconfig.generate(libpango,
  name: 'Pango',
  description: 'Internationalized text handling',
  requires: pango_pkg_requires,
  filebase: 'pango',
  subdirs: pango_api_name,
)
