mini_fribidi_sources = [
  'fribidi.c',
  'fribidi_char_type.c',
  'fribidi_types.c',
]

mini_fribidi_lib = static_library('libmini-fribidi', sources: mini_fribidi_sources,
                                  dependencies: [ mathlib_dep, gobject_dep ],
                                  include_directories: root_inc,
                                  c_args: common_cflags + [
                                    '-DSYSCONFDIR="@0@"'.format(pango_sysconfdir),
                                    '-DLIBDIR="@0@"'.format(pango_libdir),
                                  ])

mini_fribidi_dep = declare_dependency(link_with: mini_fribidi_lib,
                                      dependencies: [ mathlib_dep, gobject_dep ],
                                      include_directories: include_directories('.'))
