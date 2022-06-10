examples = []

if pango_cairo_backends.contains('png')
  examples += [
    'cairoshape',
    'cairosimple',
    'cairotwisted',
  ]
endif

examples_deps = [ libpango_dep, libpangocairo_dep, ]

if pango_font_backends.contains('freetype')
  examples_deps += libpangoft2_dep
endif

# somebody will have to fix this
# if host_system == 'windows'
#   # This hasn't been built in years, most likely, but try now
#   examples += [ 'pangowin32tobmp' ]
#   examples_deps += [ libpangowin32_dep ]
# endif

foreach e: examples
  executable(e, '@0@.c'.format(e),
             dependencies: examples_deps + pango_deps,
             c_args: common_cflags,
             include_directories: root_inc,
             install: false)
endforeach
