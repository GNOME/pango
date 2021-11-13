gtk_css_public_sources = files([
  'gtkcsslocation.c',
  'gtkcsserror.c',
  'gtkcsssection.c',
])

gtk_css_private_sources = files([
  'gtkcssdataurl.c',
  'gtkcssparser.c',
  'gtkcsstokenizer.c',
  'gtkcssserializer.c',
])

gtk_css_public_headers = files([
  'gtkcssenums.h',
  'gtkcsserror.h',
  'gtkcsslocation.h',
  'gtkcsssection.h',
])

gtk_css_deps = [
  libm,
  glib_dep,
  gobject_dep,
  platform_gio_dep,
]

gtk_css_enums = gnome.mkenums('gtkcssenumtypes',
  sources: gtk_css_public_headers,
  c_template: 'gtkcssenumtypes.c.template',
  h_template: 'gtkcssenumtypes.h.template',
)

gtk_css_enum_h = gtk_css_enums[1]

libgtk_css = static_library('gtk_css',
  sources: [
    gtk_css_public_sources,
    gtk_css_private_sources,
    gtk_css_enums,
  ],
  dependencies: gtk_css_deps,
  include_directories: [ confinc, ],
  c_args: [
    '-DGTK_COMPILATION',
    '-DG_LOG_DOMAIN="Gtk"',
  ] + common_cflags,
)

libgtk_css_dep = declare_dependency(include_directories: [ confinc, ],
  sources: [ gtk_css_enum_h ],
  dependencies: gtk_css_deps,
  link_with: libgtk_css,
)
