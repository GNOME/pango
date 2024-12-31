test_env = environment()
test_env.set('srcdir', meson.current_source_dir())
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('LC_ALL', 'en_US.UTF-8')

tests = [
  [ 'testboundaries' ],
  [ 'testboundaries_ucd' ],
  [ 'testcolor' ],
  [ 'testscript' ],
  [ 'testlanguage' ],
  [ 'testmatrix' ],
  [ 'testtabs' ],
  [ 'test-bidi' ],
  [ 'testcontext' ],
  [ 'testiter' ],
  [ 'test-ellipsize' ],
  [ 'markup-parse' ],
  [ 'test-itemize' ],
  [ 'test-font' ],
  [ 'testattributes' ],
  [ 'cxx-test', [ 'cxx-test.cpp' ] ],
  [ 'test-harfbuzz' ],
  [ 'test-break', [ 'test-break.c', 'test-common.c', 'validate-log-attrs.c' ] ],
  [ 'testserialize' ],
  [ 'testhbfont' ],
  [ 'test-layout' ],
  [ 'test-fontsets' ],
  [ 'testrandom' ],
]

if cairo_dep.found()
  tests += [
    [ 'testmisc' ],
    [ 'test-pangocairo-threads' ],
  ]
endif

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

gen_all_unicode = files([ 'gen-all-unicode.py' ])

custom_target('all-unicode',
              output: 'all-unicode.txt',
              command: [
                gen_all_unicode, '@OUTPUT@'
              ])

foreach t: tests
  name = t[0]
  src = t.get(1, [ '@0@.c'.format(name), 'test-common.c' ])
  deps = t.get(2, [ libpango_dep ])

  bin = executable(name, src,
                   dependencies: deps + [ libpango_dep ],
                   include_directories: root_inc,
                   c_args: common_cflags + pango_debug_cflags,
                   cpp_args: common_cppflags + pango_debug_cflags)

  test(name, bin,
    args: ['-k', '--tap'],
    env: test_env,
    suite: 'pango',
    protocol: 'tap',
  )
endforeach
