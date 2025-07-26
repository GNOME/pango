pango_view_sources = [
  'pango-view.c',
  'viewer-main.c',
  'viewer-render.c',
]

pango_view_deps = [
  pango_deps,
  libpango_dep,
]

if build_pangoft2
  pango_view_sources += 'viewer-pangoft2.c'
  pango_view_deps += libpangoft2_dep
endif

if xft_dep.found() and build_pangoft2
  pango_view_sources += [
    'viewer-pangoxft.c',
    'viewer-x.c',
  ]
  pango_view_deps += [ libpangoft2_dep, libpangoxft_dep, ]
endif

if cairo_dep.found()
  pango_view_sources += [
    'viewer-cairo.c',
    'viewer-pangocairo.c',
  ]
  pango_view_deps += libpangocairo_dep

  if pango_conf.has('HAVE_CAIRO_QUARTZ')
    viewer_cocoa_resources = import('gnome').compile_resources(
      'viewer-cocoa-resources', 'viewer-cocoa.gresource.xml',
      c_name : 'viewer_cocoa',
    )
    pango_view_sources += ['viewer-cocoa.m', viewer_cocoa_resources]
    pango_view_deps += dependency('Cocoa')
  endif
endif

pango_view = executable('pango-view', pango_view_sources,
                        dependencies: pango_view_deps,
                        include_directories: [ root_inc ],
                        install: true,
                        c_args: common_cflags + pango_debug_cflags + [
                          '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
                          '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
                        ])

if cairo_dep.found()
  pango_list_sources = [
    'pango-list.c',
  ]

  pango_list_deps = [
    pango_deps,
    libpango_dep,
    libpangocairo_dep
  ]

  pango_list = executable('pango-list', pango_list_sources,
                          dependencies: pango_list_deps,
                          include_directories: [ root_inc ],
                          install: true,
                          c_args: common_cflags + pango_debug_cflags + [
                           '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
                           '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
                          ])

  pango_segmentation_sources = [
    'pango-segmentation.c',
  ]

  pango_segmentation_deps = [
    pango_deps,
    libpango_dep,
    libpangocairo_dep
  ]

  pango_segmentation = executable('pango-segmentation',
                                  pango_segmentation_sources,
                                  dependencies: pango_segmentation_deps,
                                  include_directories: [ root_inc ],
                                  install: true,
                                  c_args: common_cflags + pango_debug_cflags + [
                                   '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
                                   '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
                                  ])

endif
