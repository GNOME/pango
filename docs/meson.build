gidocgen = find_program('gi-docgen', required: get_option('documentation'), native: true)

gidocgen_common_args = [
  '--quiet',
  '--no-namespace-dir',
]

if get_option('werror')
  gidocgen_common_args += ['--fatal-warnings']
endif

docs_dir = pango_datadir / 'doc'

if get_option('documentation') and not build_gir
  error('API reference requires introspection.')
endif

pango_content_files = [
  'pango_rendering.md',
  'pango_markup.md',
  'pango_fonts.md',
  'pango_bidi.md',
  'pango-name.png',
  'layout-light.png',
  'layout-dark.png',
  'pipeline-light.png',
  'pipeline-dark.png',
  'rects1.png',
  'rects2.png',
  'rects3.png',
  'arabic-markup.png',
  'blue-text.png',
  'align-left.png',
  'align-center.png',
  'align-right.png',
  'align-left-justify.png',
  'align-center-justify.png',
  'align-right-justify.png',
  'vertical-light.png',
  'vertical-dark.png',
  'm-south.png',
  'm-west.png',
  'm-north.png',
  'm-east.png',
  'bidi-input-light.png',
  'bidi-input-dark.png',
  'bidi-annotate-light.png',
  'bidi-annotate-dark.png',
  'split-cursor.png',
  'split-cursor-light.png',
  'split-cursor-dark.png',
  'fontmetrics-light.png',
  'fontmetrics-dark.png',
  'caret-metrics-light.png',
  'caret-metrics-dark.png',
  'cursor-positions-light.png',
  'cursor-positions-dark.png',
  'glyphstring-positions-light.png',
  'glyphstring-positions-dark.png',
  'baseline-shift-light.png',
  'baseline-shift-dark.png',
]

doc_conf = configuration_data()
doc_conf.set('PANGO_VERSION', meson.project_version())

if get_option('documentation')
  pango_toml = configure_file(input: 'pango.toml.in',
                              output: 'pango.toml',
                              configuration: doc_conf)

  custom_target('pango-doc',
    input: pango_gir[0],
    output: 'Pango',
    command: [
      gidocgen,
      'generate',
      gidocgen_common_args,
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
      '--config', pango_toml,
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ pango_toml, pango_content_files],
    build_by_default: true,
    install: true,
    install_dir: docs_dir,
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
        gidocgen_common_args,
        '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
        '--config', pangoft2_toml,
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT@',
      ],
      depend_files: [ pangoft2_toml, 'pango-name.png' ],
      build_by_default: true,
      install: true,
      install_dir: docs_dir,
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
        gidocgen_common_args,
        '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
        '--config', pangoxft_toml,
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT@',
      ],
      depend_files: [ pangoxft_toml, 'pango-name.png' ],
      build_by_default: true,
      install: true,
      install_dir: docs_dir,
    )
  endif

  if cairo_dep.found()
    pangocairo_content_files = [
      'pango_cairo.md',
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
        gidocgen_common_args,
        '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
        '--config', pangocairo_toml,
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT@',
      ],
      depend_files: [ pangocairo_toml, pangocairo_content_files ],
      build_by_default: true,
      install: true,
      install_dir: docs_dir,
    )
  endif

  if fontconfig_dep.found()
    pangoot_toml = configure_file(input: 'pangoot.toml.in',
                                  output: 'pangoot.toml',
                                  configuration: doc_conf)

    custom_target('pangoot-doc',
      input: pangoot_gir[0],
      output: 'PangoOT',
      command: [
        gidocgen,
        'generate',
        gidocgen_common_args,
        '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
        '--config', pangoot_toml,
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT@',
      ],
      depend_files: [ pangoot_toml, 'pango-name.png' ],
      build_by_default: true,
      install: true,
      install_dir: docs_dir,
    )

    pangofc_toml = configure_file(input: 'pangofc.toml.in',
                                  output: 'pangofc.toml',
                                  configuration: doc_conf)

    custom_target('pangofc-doc',
      input: pangofc_gir[0],
      output: 'PangoFc',
      command: [
        gidocgen,
        'generate',
        gidocgen_common_args,
        '--add-include-path=@0@'.format(meson.current_build_dir() / '../pango'),
        '--config', pangofc_toml,
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT@',
      ],
      depend_files: [ pangofc_toml, 'pango-name.png' ],
      build_by_default: true,
      install: true,
      install_dir: docs_dir,
    )
  endif
endif

rst2man = find_program('rst2man', 'rst2man.py', required: get_option('man-pages'))
rst2html5 = find_program('rst2html5', 'rst2html5.py', required: get_option('documentation'))

rst_files = [
  [ 'pango-view', '1' ],
  [ 'pango-list', '1' ],
  [ 'pango-segmentation', '1' ],
]

rst2x_flags = [
  '--syntax-highlight=none',
]

if get_option('man-pages')

  foreach rst: rst_files
    man_name = rst[0]
    man_section = rst.get(1, '1')

    custom_target('man-@0@'.format(man_name),
      input: '@0@.rst'.format(man_name),
      output: '@0@.@1@'.format(man_name, man_section),
      command: [
        rst2man,
        rst2x_flags,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: get_option('mandir') / 'man@0@'.format(man_section),
      install_tag: 'doc',
    )
  endforeach
endif

if get_option('documentation')

  foreach rst: rst_files
    man_name = rst[0]

    custom_target(
      input: '@0@.rst'.format(man_name),
      output: '@0@.html'.format(man_name),
      command: [
        rst2html5,
        rst2x_flags,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: docs_dir / 'Pango',
      install_tag: 'doc',
    )
  endforeach
endif
