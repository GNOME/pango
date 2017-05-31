if fontconfig_dep.found()
  executable('gen-script-for-lang', 'gen-script-for-lang.c',
             dependencies: pango_deps + [ libpango_dep ],
             c_args: common_cflags + pango_debug_cflags + [ '-DPANGO_DISABLE_DEPRECATED' ],
             include_directories: root_inc,
             install: false)
endif
