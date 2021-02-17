gidocgen = find_program('gi-docgen', required: get_option('gtk_doc'))

pango_content_files = [
  'pango_rendering.md',
  'pango_markup.md',
  'pango-name.png',
  'layout.png',
  'pipeline.png',
  'rects1.png',
  'rects2.png',
  'rects3.png',
]

doc_conf = configuration_data()
doc_conf.set('PANGO_VERSION', meson.project_version())

pango_toml = configure_file(input: 'pango.toml.in',
                            output: 'pango.toml',
                            configuration: doc_conf)

custom_target('pango-doc',
  input: pango_gir[0],
  output: 'Pango',
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
    '--config', pango_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT@',
  ],
  depend_files: [ pango_toml, pango_content_files],
  build_by_default: true,
)

if build_pangoft2
  pangoft2_toml = configure_file(input: 'pangoft2.toml.in',
                                 output: 'pangoft2.toml',
                                 configuration: doc_conf)

  custom_target('pangoft-doc',
    input: pangoft2_gir[0],
    output: 'PangoFT2',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
      '--config', pangoft2_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ pangoft2_toml, 'pango-name.png' ],
    build_by_default: true,
  )
endif

if xft_dep.found() and fontconfig_dep.found()
  pangoxft_toml = configure_file(input: 'pangoxft.toml.in',
                                 output: 'pangoxft.toml',
                                 configuration: doc_conf)

  custom_target('pangoxft-doc',
    input: pangoxft_gir[0],
    output: 'PangoXft',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
      '--config', pangoxft_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ pangoxft_toml, 'pango-name.png' ],
    build_by_default: true,
  )
endif

if cairo_dep.found()
  pangocairo_content_files = [
    'pango-name.png',
    'rotated-text.png',
  ]

  pangocairo_toml = configure_file(input: 'pangocairo.toml.in',
                                   output: 'pangocairo.toml',
                                   configuration: doc_conf)

  custom_target('pangocairo-doc',
    input: pangocairo_gir[0],
    output: 'PangoCairo',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
      '--config', pangocairo_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ pangocairo_toml, pangocairo_content_files ],
    build_by_default: true,
  )
endif

pangoot_toml = configure_file(input: 'pangoot.toml.in',
                              output: 'pangoot.toml',
                              configuration: doc_conf)

custom_target('pangoot-doc',
  input: pangoot_gir[0],
  output: 'PangoOT',
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
    '--config', pangoot_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT@',
  ],
  depend_files: [ pangoot_toml, 'pango-name.png' ],
  build_by_default: true,
)

if fontconfig_dep.found()
  pangofc_toml = configure_file(input: 'pangofc.toml.in',
                                output: 'pangofc.toml',
                                configuration: doc_conf)

  custom_target('pangofc-doc',
    input: pangofc_gir[0],
    output: 'PangoFc',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
      '--config', pangofc_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ pangofc_toml, 'pango-name.png' ],
    build_by_default: true,
  )
endif
