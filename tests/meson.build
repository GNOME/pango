test_cflags = [
  '-DSRCDIR=@0@'.format(meson.current_source_dir()),
]

if xft_dep.found()
  test_cflags += [ '-DHAVE_X', '-DHAVE_XFT' ]
endif

if host_system == 'windows'
  test_cflags += '-DHAVE_WIN32'
endif

test_env = [
  'srcdir=@0@'.format(meson.current_source_dir()),
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

tests = [
  [ 'test-coverage' ],
  [ 'testboundaries' ],
  [ 'testboundaries_ucd' ],
  [ 'testcolor' ],
  [ 'testscript' ],
  [ 'test-harfbuzz', [ 'test-harfbuzz.c' ], [ libpangocairo_dep ] ],
  [ 'cxx-test', [ 'cxx-test.cpp' ], [ libpangocairo_dep ] ],
  [ 'test-break', [ 'test-break.c', 'test-common.c' ], [libpangocairo_dep ] ],
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
    [ 'testiter', [ 'testiter.c' ], [ libpangocairo_dep ] ],
    [ 'test-ellipsize', [ 'test-ellipsize.c' ], [ libpangocairo_dep ] ],
    [ 'markup-parse', [ 'markup-parse.c' , 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-layout', [ 'test-layout.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-itemize', [ 'test-itemize.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-shape', [ 'test-shape.c', 'test-common.c' ], [ libpangocairo_dep ] ],
    [ 'test-font', [ 'test-font.c' ], [ libpangocairo_dep ] ],
    [ 'testattributes', [ 'testattributes.c', 'test-common.c' ], [ libpangocairo_dep ] ],
  ]

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
  'LineBreakTest.txt'
]

test_layouts_data = [
  'layouts/valid-1',
  'layouts/valid-2',
]

test_markups_data = [
  'markups/fail-1',
  'markups/fail-2',
  'markups/fail-3',
  'markups/fail-4',
  'markups/fail-5',
  'markups/valid-1',
  'markups/valid-2',
  'markups/valid-3',
  'markups/valid-4',
  'markups/valid-5',
  'markups/valid-6',
  'markups/valid-7',
  'markups/valid-8',
  'markups/valid-9',
]

test_breaks_data = [
  'breaks/one',
  'breaks/two',
  'breaks/three',
]

test_items_data = [
  'itemize/one',
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

python = import('python3').find_python()
gen_installed_test = files([ 'gen-installed-test.py' ])
gen_all_unicode = files([ 'gen-all-unicode.py' ])

custom_target('all-unicode',
              output: 'all-unicode.txt',
              command: [
                python, gen_all_unicode, '@OUTPUT@'
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
                  python, gen_installed_test,
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

  test(name, bin, env: test_env)
endforeach
