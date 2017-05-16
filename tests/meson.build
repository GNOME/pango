test_cflags = [
  '-DSRCDIR=@0@'.format(meson.current_source_dir()),
]

if xft_dep.found()
  test_cflags += [ '-DHAVE_X', '-DHAVE_XFT' ]
endif

if freetype_dep.found()
  test_cflags += '-DHAVE_FREETYPE'
endif

if host_system.contains('windows')
  test_cflags += '-DHAVE_WIN32'
endif

test_env = [
  'srcdir=@0@'.format(meson.current_source_dir()),
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

tests = [
  [ 'testboundaries' ],
  [ 'testboundaries_ucd' ],
  [ 'testcolor' ],
  [ 'testscript' ],
]

if freetype_dep.found()
  tests += [
    [ 'test-ot-tags', [ 'test-ot-tags.c' ], [ libpangoft2_dep ] ],
  ]
endif

if cairo_dep.found()
  tests += [
    [ 'testiter', [ 'testiter.c' ], [ libpangocairo_dep ] ],
    [ 'test-pangocairo-threads', [ 'test-pangocairo-threads.c' ], [ libpangocairo_dep, cairo_dep ] ],
    [ 'markup-parse', [ 'markup-parse.c' ], [ libpangocairo_dep ] ],
    [ 'test-layout', [ 'test-layout.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-font', [ 'test-font.c' ], [ libpangocairo_dep ] ],
    [ 'testattributes', [ 'testattributes.c', 'test-common.c' ], [ libpangocairo_dep ] ],
  ]
endif

foreach t: tests
  name = t[0]
  src = t.get(1, [ '@0@.c'.format(name) ])
  deps = t.get(2, [ libpango_dep ])

  bin = executable(name, src,
                   dependencies: deps,
                   include_directories: root_inc,
                   c_args: common_cflags + pango_debug_cflags)
  test(name, bin, env: test_env)
endforeach
