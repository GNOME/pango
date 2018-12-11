pango_sources = [
  'break.c',
  'ellipsize.c',
  'fonts.c',
  'glyphstring.c',
  'modules.c',
  'pango-attributes.c',
  'pango-bidi-type.c',
  'pango-color.c',
  'pango-context.c',
  'pango-coverage.c',
  'pango-emoji.c',
  'pango-engine.c',
  'pango-fontmap.c',
  'pango-fontset.c',
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
]

pango_headers = [
  'pango.h',
  'pango-attributes.h',
  'pango-bidi-type.h',
  'pango-break.h',
  'pango-context.h',
  'pango-coverage.h',
  'pango-direction.h',
  'pango-engine.h',
  'pango-font.h',
  'pango-fontmap.h',
  'pango-fontset.h',
  'pango-glyph.h',
  'pango-glyph-item.h',
  'pango-gravity.h',
  'pango-item.h',
  'pango-language.h',
  'pango-layout.h',
  'pango-matrix.h',
  'pango-modules.h',
  'pango-renderer.h',
  'pango-script.h',
  'pango-tabs.h',
  'pango-types.h',
  'pango-utils.h',
  'pango-version-macros.h',
]

install_headers(pango_headers, subdir: pango_api_path)

# Features header
pango_features_conf = configuration_data()
pango_features_conf.set('PANGO_VERSION_MAJOR', pango_major_version)
pango_features_conf.set('PANGO_VERSION_MINOR', pango_minor_version)
pango_features_conf.set('PANGO_VERSION_MICRO', pango_micro_version)
pango_features_conf.set('PANGO_API_VERSION', pango_api_version)
pango_features_conf.set('PANGO_CURRENT_MINUS_AGE', '0')

configure_file(
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
  '-DPANGO_ENABLE_BACKEND',
  '-DPANGO_ENABLE_ENGINE',
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

pango_dep_sources = [pango_enum_h]

build_gir = get_option('gir')

if build_gir
  gir_args = [
    '--quiet',
    '-DPANGO_ENABLE_BACKEND',
    '-DPANGO_ENABLE_ENGINE',
  ]

  pango_gir = gnome.generate_gir(
    libpango,
    sources: pango_sources + pango_headers + [ pango_enum_h ],
    namespace: 'Pango',
    nsversion: pango_api_version,
    identifier_prefix: 'Pango',
    symbol_prefix: 'pango',
    export_packages: 'pango',
    includes: [ 'GObject-2.0', 'cairo-1.0', ],
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

pkgconfig.generate(libpango,
  name: 'Pango',
  description: 'Internationalized text handling',
  version: meson.project_version(),
  filebase: 'pango',
  subdirs: pango_api_name,
  install_dir: join_paths(pango_libdir, 'pkgconfig'),
)

# FreeType
if build_pangoft2
  pangoft2_headers = [
    'pango-ot.h',
    'pangofc-font.h',
    'pangofc-fontmap.h',
    'pangofc-decoder.h',
    'pangoft2.h',
  ]

  pangoft2_public_sources = [
    'pangofc-font.c',
    'pangofc-fontmap.c',
    'pangofc-decoder.c',
    'pangofc-shape.c',
    'pangoft2.c',
  ]

  pangoft2_sources = pangoft2_public_sources + [
    'pangoft2-fontmap.c',
    'pangoft2-render.c',
    'pango-ot-buffer.c',
    'pango-ot-info.c',
    'pango-ot-ruleset.c',
    'pango-ot-tag.c',
  ]

  if host_system == 'windows'
    pangoft2_rc = configure_file(
      input: 'pangoft2.rc.in',
      output: 'pangoft2.rc',
      configuration: pango_features_conf,
    )
    pangoft2_res = import('windows').compile_resources(pangoft2_rc)
    pangoft2_sources += pangoft2_res
  endif

  install_headers(pangoft2_headers, subdir: pango_api_path)

  libpangoft2 = library(
    'pangoft2-@0@'.format(pango_api_version),
    sources: pangoft2_sources,
    version: pango_libversion,
    soversion: pango_soversion,
    darwin_versions : pango_osxversion,
    install: true,
    dependencies: pango_deps + [ libpango_dep ],
    include_directories: [ root_inc, pango_inc ],
    c_args: common_cflags + pango_debug_cflags + pango_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags,
  )

  pangoft2_dep_sources = []

  if build_gir
    pangoft2_gir = gnome.generate_gir(
      libpangoft2,
      sources: pangoft2_headers + pangoft2_public_sources,
      dependencies: [ libpango_dep, pango_gir_dep ],
      namespace: 'PangoFT2',
      nsversion: pango_api_version,
      identifier_prefix: 'PangoFT2',
      symbol_prefix: 'pango_ft2',
      export_packages: 'pangoft2',
      includes: [ pango_gir[0], 'GObject-2.0', 'cairo-1.0', 'freetype2-2.0', 'fontconfig-2.0', ],
      header: 'pango/pangoft2.h',
      install: true,
      extra_args: gir_args,
    )
    pangoft2_gir_dep = declare_dependency(sources: pangoft2_gir)
    pangoft2_dep_sources += pangoft2_gir
  endif

  libpangoft2_dep = declare_dependency(
    link_with: libpangoft2,
    include_directories: pango_inc,
    dependencies: pango_deps + [ libpango_dep ],
    sources: pangoft2_dep_sources,
  )

  pkgconfig.generate(libpangoft2,
    name: 'Pango FT2 and Pango Fc',
    description: 'Freetype 2.0 and fontconfig font support for Pango',
    version: meson.project_version(),
    filebase: 'pangoft2',
    subdirs: pango_api_name,
    requires: [ 'pango', freetype2_pc, fontconfig_pc ],
    install_dir: join_paths(pango_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libpangoft2_dep = dependency('', required : false)
  # Needed for making ft2 no-op while generating pangoxft_gir below
  pangoft2_gir_dep = dependency('', required : false)
  pangoft2_gir = [[]]
endif

# Xft
if xft_dep.found() and fontconfig_dep.found()
  pangoxft_headers = [
    'pangoxft.h',
    'pangoxft-render.h',
  ]

  pangoxft_sources = [
    'pangoxft-font.c',
    'pangoxft-fontmap.c',
    'pangoxft-render.c',
  ]

  install_headers(pangoxft_headers, subdir: pango_api_path)

  libpangoxft = library(
    'pangoxft-@0@'.format(pango_api_version),
    sources: pangoxft_sources,
    version: pango_libversion,
    soversion: pango_soversion,
    darwin_versions : pango_osxversion,
    install: true,
    dependencies: pango_deps + [ libpango_dep, libpangoft2_dep ],
    include_directories: [ root_inc, pango_inc ],
    c_args: common_cflags + pango_debug_cflags + pango_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags,
  )
  pangoxft_dep_sources = []

  if build_gir
    pangoxft_gir = gnome.generate_gir(
      libpangoxft,
      sources: pangoxft_headers + pangoxft_sources,
      dependencies: [ libpango_dep, libpangoft2_dep, pango_gir_dep, pangoft2_gir_dep ],
      namespace: 'PangoXft',
      nsversion: pango_api_version,
      identifier_prefix: 'PangoXft',
      symbol_prefix: 'pango_xft',
      export_packages: 'pangoxft',
      includes: [ pango_gir[0], pangoft2_gir[0], 'GObject-2.0', 'xft-2.0', 'xlib-2.0' ],
      header: 'pango/pangoxft.h',
      install: true,
      extra_args: gir_args,
    )
    pangoxft_dep_sources += pangoxft_gir
  endif

  libpangoxft_dep = declare_dependency(
    link_with: libpangoxft,
    include_directories: root_inc,
    dependencies: pango_deps + [ libpango_dep, libpangoft2_dep ],
    sources: pangoxft_dep_sources,
  )

  pkgconfig.generate(libpangoxft,
    name: 'Pango Xft',
    description: 'Xft font support for Pango',
    version: meson.project_version(),
    filebase: 'pangoxft',
    subdirs: pango_api_name,
    requires: [ 'pangoft2', 'xft' ],
    install_dir: join_paths(pango_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libpangoxft_dep = dependency('', required: false)
endif

# Win32
if host_system == 'windows'
  pangowin32_headers = [
    'pangowin32.h',
  ]

  pangowin32_sources = [
    'pangowin32.c',
    'pangowin32-fontcache.c',
    'pangowin32-fontmap.c',
    'pangowin32-shape.c',
  ]

  # usp10 needs to be before gdi32
  # see https://bugzilla.gnome.org/show_bug.cgi?id=795045#c0
  pangowin32_deps = pango_deps + [
    libpango_dep,
    cc.find_library('usp10'),
    cc.find_library('gdi32'),
  ]

  pangowin32_rc = configure_file(
    input: 'pangowin32.rc.in',
    output: 'pangowin32.rc',
    configuration: pango_features_conf,
  )
  pangowin32_res = import('windows').compile_resources(pangowin32_rc)
  pangowin32_sources += pangowin32_res

  install_headers(pangowin32_headers, subdir: pango_api_path)

  libpangowin32 = library(
    'pangowin32-@0@'.format(pango_api_version),
    sources: pangowin32_sources,
    version: pango_libversion,
    soversion: pango_soversion,
    install: true,
    dependencies: pangowin32_deps,
    include_directories: [ root_inc, pango_inc ],
    c_args: common_cflags + pango_debug_cflags + pango_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags,
  )

  libpangowin32_dep = declare_dependency(
    link_with: libpangowin32,
    include_directories: root_inc,
    dependencies: pango_deps + [ libpango_dep ],
  )

  pkgconfig.generate(libpangowin32,
    name: 'Pango Win32',
    description: 'Win32 GDI font support for Pango',
    version: meson.project_version(),
    filebase: 'pangowin32',
    subdirs: pango_api_name,
    requires: 'pango',
    install_dir: join_paths(pango_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libpangowin32_dep = dependency('', required: false)
endif

# Cairo
if cairo_dep.found()
  pangocairo_headers = [
    'pangocairo.h',
  ]

  pangocairo_sources = [
    'pangocairo-context.c',
    'pangocairo-font.c',
    'pangocairo-fontmap.c',
    'pangocairo-render.c',
  ]

  pangocairo_deps = pango_deps + [ libpango_dep ]

  if pango_font_backends.contains('freetype')
    pangocairo_sources += [
      'pangocairo-fcfont.c',
      'pangocairo-fcfontmap.c',
    ]

    pangocairo_deps += libpangoft2_dep
  endif

  if host_system == 'windows' and pango_font_backends.contains('win32')
    pangocairo_sources += [
      'pangocairo-win32font.c',
      'pangocairo-win32fontmap.c',
    ]

    pangocairo_rc = configure_file(
      input: 'pangocairo.rc.in',
      output: 'pangocairo.rc',
      configuration: pango_features_conf,
    )
    pangocairo_res = import('windows').compile_resources(pangocairo_rc)
    pangocairo_sources += pangocairo_res

    pangocairo_deps += cc.find_library('gdi32')
    pangocairo_deps += libpangowin32_dep
  endif

  if host_system == 'darwin' and pango_font_backends.contains('quartz')
    pangocairo_sources += [
      'pangocoretext.c',
      'pangocoretext-fontmap.c',
      'pangocoretext-shape.c',
      'pangocairo-coretextfont.c',
      'pangocairo-coretextfontmap.c',
    ]
  endif

  install_headers(pangocairo_headers, subdir: pango_api_path)

  libpangocairo = library(
    'pangocairo-@0@'.format(pango_api_version),
    sources: pangocairo_sources,
    version: pango_libversion,
    soversion: pango_soversion,
    darwin_versions : pango_osxversion,
    install: true,
    dependencies: pango_deps + pangocairo_deps,
    include_directories: [ root_inc, pango_inc ],
    c_args: common_cflags + pango_debug_cflags + pango_cflags + [
      '-DPANGO_DISABLE_DEPRECATION_WARNINGS',
    ],
    link_args: common_ldflags
  )
  pangocairo_dep_sources = []

  if build_gir
    pangocairo_gir = gnome.generate_gir(
      libpangocairo,
      sources: pangocairo_headers + pangocairo_sources,
      dependencies: [ pangocairo_deps, pango_gir_dep ],
      namespace: 'PangoCairo',
      nsversion: pango_api_version,
      identifier_prefix: 'PangoCairo',
      symbol_prefix: 'pango_cairo',
      export_packages: 'pangocairo',
      includes: [ pango_gir[0], 'GObject-2.0', 'cairo-1.0' ],
      header: 'pango/pangocairo.h',
      install: true,
      extra_args: gir_args,
    )
    pangocairo_dep_sources += pangocairo_gir
  endif

  libpangocairo_dep = declare_dependency(
    link_with: libpangocairo,
    include_directories: root_inc,
    dependencies: pango_deps + [ libpango_dep ],
    sources: pangocairo_dep_sources,
  )

  pkgconfig.generate(libpangocairo,
    name: 'Pango Cairo',
    description: 'Cairo rendering support for Pango',
    version: meson.project_version(),
    filebase: 'pangocairo',
    subdirs: pango_api_name,
    requires: [ 'pango', cairo_pc ],
    install_dir: join_paths(pango_libdir, 'pkgconfig'),
  )
else
  # For usage as a subproject
  libpangocairo_dep = dependency('', required: false)
endif
