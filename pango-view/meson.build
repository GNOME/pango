pango_view_sources = [
  'pango-view.c',
  'viewer-main.c',
  'viewer-render.c',
]

pango_view_deps = [
  pango_deps,
  libpango_dep,
]

if freetype_dep.found()
  pango_view_sources += 'viewer-pangoft2.c'
  pango_view_deps += libpangoft2_dep
endif

if xft_dep.found()
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
endif

executable('pango-view', pango_view_sources,
           dependencies: pango_view_deps,
           include_directories: [ root_inc ],
           install: true,
           c_args: [
             '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
             '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
           ])
