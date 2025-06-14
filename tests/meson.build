test_cflags = [
  '-DSRCDIR=@0@'.format(meson.current_source_dir()),
]

if xft_dep.found()
  test_cflags += [ '-DHAVE_X', '-DHAVE_XFT' ]
endif

if host_system == 'windows'
  test_cflags += '-DHAVE_WIN32'
elif host_system == 'darwin'
  test_cflags += '-DHAVE_CARBON'
endif

test_env = environment()
test_env.set('srcdir', meson.current_source_dir())
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('LC_ALL', 'en_US.UTF-8')

tests = [
  [ 'test-coverage' ],
  [ 'testboundaries' ],
  [ 'testboundaries_ucd' ],
  [ 'testcolor' ],
  [ 'testscript' ],
  [ 'testlanguage' ],
  [ 'testmatrix' ],
  [ 'testtabs' ],
]

# deps for test-common.c
common_deps = []

if host_system == 'windows'
  common_deps += [ libpangowin32_dep ]
endif

if build_pangoft2
  test_cflags += '-DHAVE_FREETYPE'
  tests += [
    [ 'test-ot-tags', [ 'test-ot-tags.c' ], [ libpangoft2_dep ] ],
  ]
  common_deps += [ libpangoft2_dep ]
endif

if cairo_dep.found()
  test_cflags += '-DHAVE_CAIRO'
  tests += [
    [ 'test-font-data', [ 'test-font-data.c', 'test-common.c' ], [ libpangocairo_dep ] + common_deps ],
    [ 'test-bidi', [ 'test-bidi.c' ], [ libpangocairo_dep ] ],
    [ 'testcontext', [ 'testcontext.c' ], [ libpangocairo_dep ] ],
    [ 'testiter', [ 'testiter.c' ], [ libpangocairo_dep ] ],
    [ 'test-ellipsize', [ 'test-ellipsize.c' ], [ libpangocairo_dep ] ],
    [ 'markup-parse', [ 'markup-parse.c' , 'test-common.c' ], [ libpangocairo_dep ] + common_deps ],
    [ 'test-itemize', [ 'test-itemize.c', 'test-common.c' ], [ libpangocairo_dep ] + common_deps ],
    [ 'test-shape', [ 'test-shape.c', 'test-common.c' ], [ libpangocairo_dep ] + common_deps ],
    [ 'test-font', [ 'test-font.c', 'test-common.c' ], [ libpangocairo_dep ] + common_deps ],
    [ 'testattributes', [ 'testattributes.c', 'test-common.c' ], [ libpangocairo_dep ] + common_deps ],
    [ 'cxx-test', [ 'cxx-test.cpp' ], [ libpangocairo_dep, gobject_dep, harfbuzz_dep ] ],
    [ 'test-harfbuzz', [ 'test-harfbuzz.c' ], [ libpangocairo_dep, gobject_dep, harfbuzz_dep ] ],
    [ 'test-break', [ 'test-break.c', 'test-common.c', 'validate-log-attrs.c' ], [libpangocairo_dep, glib_dep, harfbuzz_dep ] + common_deps ],
  ]

  if build_pangoft2
    tests += [
      [ 'testmisc', [ 'testmisc.c' ], [ libpangocairo_dep, libpangoft2_dep, glib_dep, harfbuzz_dep ] ],
      [ 'testserialize', [ 'testserialize.c' ], [ libpangocairo_dep, libpangoft2_dep ] ],
    ]

    if host_system != 'darwin'
      tests += [
        [ 'test-layout', [ 'test-layout.c', 'test-common.c' ], [ libpangocairo_dep, libpangoft2_dep ] + common_deps ],
        [ 'test-layout2', [ 'test-layout2.c'], [ libpangocairo_dep, libpangoft2_dep ] + common_deps ],
        [ 'test-fonts', [ 'test-fonts.c', 'test-common.c' ], [ libpangocairo_dep, libpangoft2_dep ] + common_deps ],
        [ 'test-no-fonts', [ 'test-no-fonts.c' ], [ libpangocairo_dep, libpangoft2_dep ] ],
      ]
    endif
  endif

  if false and host_system != 'darwin'
    tests += [
      [ 'testrandom', [ 'testrandom.c' ], [ libpangocairo_dep, gio_dep ] ],
    ]
  endif

  if pango_cairo_backends.contains('png')
    tests += [
      [ 'test-pangocairo-threads', [ 'test-pangocairo-threads.c' ], [ libpangocairo_dep, cairo_dep ] ],
    ]
  endif
endif

gen_all_unicode = files([ 'gen-all-unicode.py' ])

custom_target('all-unicode',
              output: 'all-unicode.txt',
              command: [
                gen_all_unicode, '@OUTPUT@'
              ])

foreach t: tests
  name = t[0]
  src = t.get(1, [ '@0@.c'.format(name) ])
  deps = t.get(2, [ libpango_dep ])

  bin = executable(name, src,
                   dependencies: deps,
                   include_directories: root_inc,
                   c_args: common_cflags + pango_debug_cflags + test_cflags,
                   cpp_args: common_cppflags + pango_debug_cflags + test_cflags)

  test(name, bin,
    args: ['-k', '--tap'],
    env: test_env,
    suite: 'pango',
    protocol: 'tap',
  )
endforeach
