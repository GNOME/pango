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

if build_pangoft2
  test_cflags += '-DHAVE_FREETYPE'
  tests += [
    [ 'test-ot-tags', [ 'test-ot-tags.c' ], [ libpangoft2_dep ] ],
  ]
endif

if cairo_dep.found()
  test_cflags += '-DHAVE_CAIRO'
  tests += [
    [ 'test-bidi', [ 'test-bidi.c' ], [ libpangocairo_dep ] ],
    [ 'testcontext', [ 'testcontext.c' ], [ libpangocairo_dep ] ],
    [ 'testiter', [ 'testiter.c' ], [ libpangocairo_dep ] ],
    [ 'test-ellipsize', [ 'test-ellipsize.c' ], [ libpangocairo_dep ] ],
    [ 'markup-parse', [ 'markup-parse.c' , 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-itemize', [ 'test-itemize.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-shape', [ 'test-shape.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-font', [ 'test-font.c' ], [ libpangocairo_dep ] ],
    [ 'testattributes', [ 'testattributes.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'cxx-test', [ 'cxx-test.cpp' ], [ libpangocairo_dep, gobject_dep, harfbuzz_dep ] ],
    [ 'test-harfbuzz', [ 'test-harfbuzz.c' ], [ libpangocairo_dep, gobject_dep, harfbuzz_dep ] ],
    [ 'test-break', [ 'test-break.c', 'test-common.c', 'validate-log-attrs.c' ], [libpangocairo_dep, glib_dep, harfbuzz_dep ] ],
    [ 'testserialize', [ 'testserialize.c' ], [ libpangocairo_dep ] ],
  ]

  if build_pangoft2
    tests += [
      [ 'testmisc', [ 'testmisc.c' ], [ libpangocairo_dep, libpangoft2_dep, glib_dep, harfbuzz_dep ] ],
      [ 'test-layout', [ 'test-layout.c', 'test-common.c' ], [ libpangocairo_dep, libpangoft2_dep ] ],
    ]
  endif

  if host_system != 'darwin'
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

installed_test_data = [
  'boundaries.utf8',
  'GraphemeBreakTest.txt',
  'WordBreakTest.txt',
  'SentenceBreakTest.txt',
  'EmojiBreakTest.txt',
  'CharBreakTest.txt',
]

test_layouts_data = [
  'layouts/valid-1',
  'layouts/valid-2',
  'layouts/valid-3',
  'layouts/valid-4',
  'layouts/valid-5',
  'layouts/valid-6',
  'layouts/valid-7',
  'layouts/valid-8',
  'layouts/valid-9',
  'layouts/valid-10',
  'layouts/valid-11',
  'layouts/valid-12',
  'layouts/valid-13',
  'layouts/valid-14',
  'layouts/valid-15',
  'layouts/valid-16',
  'layouts/valid-17',
  'layouts/valid-18',
  'layouts/valid-19',
  'layouts/valid-20',
  'layouts/valid-21',
]

test_markups_data = [
  'markups/fail-1',
  'markups/fail-2',
  'markups/fail-3',
  'markups/fail-4',
  'markups/fail-5',
  'markups/fail-6',
  'markups/fail-7',
  'markups/fail-8',
  'markups/fail-9',
  'markups/fail-10',
  'markups/fail-11',
  'markups/fail-12',
  'markups/fail-13',
  'markups/fail-14',
  'markups/fail-15',
  'markups/fail-16',
  'markups/fail-17',
  'markups/fail-20',
  'markups/fail-21',
  'markups/fail-22',
  'markups/fail-23',
  'markups/fail-24',
  'markups/fail-25',
  'markups/fail-26',
  'markups/fail-27',
  'markups/fail-28',
  'markups/fail-29',
  'markups/fail-30',
  'markups/fail-31',
  'markups/fail-32',
  'markups/fail-33',
  'markups/fail-34',
  'markups/fail-35',
  'markups/fail-36',
  'markups/fail-37',
  'markups/fail-38',
  'markups/fail-39',
  'markups/valid-1',
  'markups/valid-2',
  'markups/valid-3',
  'markups/valid-4',
  'markups/valid-5',
  'markups/valid-6',
  'markups/valid-7',
  'markups/valid-8',
  'markups/valid-9',
  'markups/valid-10',
  'markups/valid-11',
  'markups/valid-12',
  'markups/valid-13',
  'markups/valid-14',
  'markups/valid-15',
  'markups/valid-16',
  'markups/valid-17',
  'markups/valid-18',
  'markups/valid-19',
  'markups/valid-20',
  'markups/valid-21',
  'markups/valid-22',
  'markups/valid-23',
  'markups/valid-24',
]

test_breaks_data = [
  'breaks/one',
  'breaks/two',
  'breaks/three',
  'breaks/nine',
  'breaks/ten',
  'breaks/eleven',
]

if thai_dep.found()
  test_breaks_data += [ 'breaks/four', ]
endif

test_items_data = [
  'itemize/one',
  'itemize/two',
]

installed_test_layouts_data = []
foreach d: test_layouts_data
  installed_test_layouts_data += d + '.markup'
  installed_test_layouts_data += d + '.expected'
endforeach

installed_test_markups_data = []
foreach d: test_markups_data
  installed_test_markups_data += d + '.markup'
  installed_test_markups_data += d + '.expected'
endforeach

installed_test_breaks_data = []
foreach d: test_breaks_data
  installed_test_breaks_data += d + '.break'
  installed_test_breaks_data += d + '.expected'
endforeach

installed_test_items_data = []
foreach d: test_items_data
  installed_test_items_data += d + '.items'
  installed_test_items_data += d + '.expected'
endforeach

installed_test_datadir = join_paths(pango_datadir, 'installed-tests', 'pango')
installed_test_bindir = join_paths(pango_libexecdir, 'installed-tests', 'pango')

if get_option('install-tests')
  install_data(installed_test_data, install_dir: installed_test_bindir)
  install_data(installed_test_layouts_data, install_dir: join_paths(installed_test_bindir, 'layouts'))
  install_data(installed_test_markups_data, install_dir: join_paths(installed_test_bindir, 'markups'))
  install_data(installed_test_breaks_data, install_dir: join_paths(installed_test_bindir, 'breaks'))
  install_data(installed_test_items_data, install_dir: join_paths(installed_test_bindir, 'itemize'))
endif

gen_installed_test = files([ 'gen-installed-test.py' ])
gen_all_unicode = files([ 'gen-all-unicode.py' ])

custom_target('all-unicode',
              output: 'all-unicode.txt',
              command: [
                gen_all_unicode, '@OUTPUT@'
              ],
              install: get_option('install-tests'),
              install_dir: installed_test_bindir)

foreach t: tests
  name = t[0]
  src = t.get(1, [ '@0@.c'.format(name) ])
  deps = t.get(2, [ libpango_dep ])

  custom_target(name + '.test',
                output: name + '.test',
                command: [
                  gen_installed_test,
                  installed_test_bindir,
                  name,
                  '@OUTDIR@',
                ],
                install: get_option('install-tests'),
                install_dir: installed_test_datadir)

  bin = executable(name, src,
                   dependencies: deps,
                   include_directories: root_inc,
                   c_args: common_cflags + pango_debug_cflags + test_cflags,
                   cpp_args: common_cppflags + pango_debug_cflags + test_cflags,
                   install: get_option('install-tests'),
                   install_dir: installed_test_bindir)

  test(name, bin,
    args: ['-k', '--tap'],
    env: test_env,
    suite: 'pango',
    protocol: 'tap',
  )
endforeach
