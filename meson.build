project('pango', 'c', 'cpp',
        version: '1.90.0',
        license: 'LGPLv2.1+',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          # We only need c99, but glib needs GNU-specific features
          # https://github.com/mesonbuild/meson/issues/2289
          'c_std=gnu99',
        ],
        meson_version : '>= 0.55.3')

add_project_arguments([ '-D_POSIX_C_SOURCE=200809L', '-D_POSIX_THREAD_SAFE_FUNCTIONS', '-D_GNU_SOURCE', ], language: 'c')

pango_prefix = get_option('prefix')
pango_libdir = join_paths(pango_prefix, get_option('libdir'))
pango_sysconfdir = join_paths(pango_prefix, get_option('sysconfdir'))
pango_includedir = join_paths(pango_prefix, get_option('includedir'))
pango_datadir = join_paths(pango_prefix, get_option('datadir'))
pango_libexecdir = join_paths(pango_prefix, get_option('libexecdir'))

version = meson.project_version().split('.')
pango_major_version = version[0].to_int()
pango_minor_version = version[1].to_int()
pango_micro_version = version[2].to_int()

if pango_minor_version >= 90
  api_major = pango_major_version + 1
  api_minor = 0
else
  api_major = pango_major_version
  api_minor = pango_minor_version
endif

pango_interface_age = pango_minor_version.is_odd() ? 0 : pango_micro_version
pango_binary_age = api_minor * 100 + pango_micro_version

pango_api_version = '@0@.0'.format(api_major)
pango_api_name = 'pango-@0@'.format(pango_api_version)
pango_api_path = join_paths(pango_api_name, 'pango')

pango_conf = configuration_data()

pango_conf.set('PANGO_BINARY_AGE', pango_binary_age)
pango_conf.set('PANGO_INTERFACE_AGE', pango_interface_age)
pango_conf.set('PANGO_VERSION_MAJOR', pango_major_version)
pango_conf.set('PANGO_VERSION_MINOR', pango_minor_version)
pango_conf.set('PANGO_VERSION_MICRO', pango_micro_version)

# Maintain version scheme with libtool
pango_soversion = 0
pango_libversion = '@0@.@1@.@2@'.format(pango_soversion, (pango_binary_age - pango_interface_age), pango_interface_age)
osx_current = pango_binary_age - pango_interface_age + 1
pango_osxversion = [osx_current, '@0@.@1@.0'.format(osx_current, pango_interface_age)]

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
host_system = host_machine.system()

# Compiler and linker flags
common_cflags = []
common_cppflags = []
common_ldflags = []

# Add more compiler warnings to the default set
if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = ['-FImsvc_recommended_pragmas.h', '-utf-8']
  add_project_arguments(cc.get_supported_arguments(test_cflags), language: ['c', 'cpp'])
  test_c_only_flags = []
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_c_only_flags = [
    '-Wno-c++11-extensions',
    '-Wno-missing-include-dirs',
    '-Wno-typedef-redefinition',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wformat=2',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Wignored-qualifiers',
    '-Wimplicit-function-declaration',
    '-Wlogical-op',
    '-Wmisleading-indentation',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    '-Wswitch-enum',
    '-Wundef',
    '-Wuninitialized',
    '-Wunused',
    '-Werror=address',
    '-Werror=array-bounds',
    '-Werror=empty-body',
    '-Werror=implicit',
    '-Werror=implicit-fallthrough',
    '-Werror=init-self',
    '-Werror=int-to-pointer-cast',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=missing-declarations',
    '-Werror=missing-prototypes',
    '-Werror=nonnull',
    '-Werror=pointer-to-int-cast',
    '-Werror=redundant-decls',
    '-Werror=return-type',
    '-Werror=sequence-point',
    '-Werror=trigraphs',
    '-Werror=vla',
    '-Werror=write-strings',
  ]

  test_cflags = test_c_only_flags + [
    '-fno-strict-aliasing',
    '-Wpointer-arith',
    '-Wmissing-declarations',
    '-Wformat=2',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Wunused',
    '-Wcast-align',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wlogical-op',
    '-Wno-uninitialized',
    '-Wno-shadow',
    '-Werror=implicit-fallthrough',
    '-Werror=nonnull',
    '-Werror=init-self',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=sequence-point',
    '-Werror=return-type',
    '-Werror=trigraphs',
    '-Werror=array-bounds',
    '-Werror=write-strings',
    '-Werror=address',
    '-Werror=int-to-pointer-cast',
    '-Werror=empty-body',
    '-Werror=write-strings',
    '-Werror=unused-but-set-variable',
    '-Wundef', # FIXME: https://bugzilla.gnome.org/show_bug.cgi?id=792481
  ]

  if host_system == 'windows'
    test_cflags += [ '-mms-bitfields' ]
  else
    test_cflags += [ '-Werror=redundant-decls' ]
  endif
else
  test_cflags = []
  test_c_only_flags = []
endif

# Symbol visibility
if get_option('default_library') != 'static'
  if host_system == 'windows'
    pango_conf.set('DLL_EXPORT', true)
    pango_conf.set('_PANGO_EXTERN', '__declspec(dllexport) extern')
    if cc.get_id() != 'msvc'
      test_cflags += ['-fvisibility=hidden']
    endif
  else
    pango_conf.set('_PANGO_EXTERN', '__attribute__((visibility("default"))) extern')
    test_cflags += ['-fvisibility=hidden']
  endif
endif

# Check all compiler flags
common_cflags += cc.get_supported_arguments(test_cflags)

# Isolate the C++ compiler flags
foreach cflag: common_cflags
  if not test_c_only_flags.contains(cflag)
    common_cppflags += [ cflag ]
  endif
endforeach

# Linker flags
if host_machine.system() == 'linux'
  common_ldflags += cc.get_supported_link_arguments([ '-Wl,-Bsymbolic', '-Wl,-z,relro', '-Wl,-z,now', ])
endif

# Functions
checked_funcs = [
  'sysconf',
  'getpagesize',
  'flockfile',
  'strtok_r',
]

foreach f: checked_funcs
  if cc.has_function(f)
    pango_conf.set('HAVE_' + f.underscorify().to_upper(), 1)
  endif
endforeach

# Headers
checked_headers = [
  'unistd.h',
  'sys/mman.h',
  'dirent.h',
]

foreach h: checked_headers
  if cc.has_header(h)
    pango_conf.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

# Use debug/optimization flags to determine whether to enable debug or disable
# cast checks
pango_debug_cflags = []
if get_option('debug')
  pango_debug_cflags = [ '-DPANGO_ENABLE_DEBUG' ]
  message('Enabling various debug infrastructure')
elif get_option('optimization') in ['2', '3', 's']
  pango_debug_cflags = [ '-DG_DISABLE_CAST_CHECKS' ]
  message('Disabling cast checks')
  # TODO: We may want a configuration argument to add `G_DISABLE_CHECKS`
  # and `G_DISABLE_ASSERT` from the build, for specific build environments.
  # On the other hand, those who need these symbols can inject them in their
  # build as well.
endif

# Dependencies
pango_deps = []

glib_req_version = '>= 2.62'
fribidi_req_version = '>= 1.0.6'
libthai_req_version = '>= 0.1.9'
harfbuzz_req_version = '>= 3.3.0'
fontconfig_req_version = '>= 2.13.0'
cairo_req_version = '>= 1.12.10'

# libm
mathlib_dep = cc.find_library('m', required: false)
pango_deps += mathlib_dep

# gobject
glib_dep = dependency('glib-2.0', version: glib_req_version,
                      fallback: ['glib', 'libglib_dep'])
gmodule_dep = dependency('gmodule-2.0', version: glib_req_version,
                         fallback : ['glib', 'libgmodule_dep'])
gobject_dep = dependency('gobject-2.0', version: glib_req_version,
                         fallback: ['glib', 'libgobject_dep'])
gio_dep = dependency('gio-2.0', version: glib_req_version,
                         fallback: ['glib', 'libgio_dep'])
pango_deps += [glib_dep, gmodule_dep, gobject_dep, gio_dep]

fribidi_dep = dependency('fribidi', version: fribidi_req_version,
                         fallback: ['fribidi', 'libfribidi_dep'],
                         default_options: ['docs=false'])
pango_deps += fribidi_dep

thai_dep = dependency('libthai', version: libthai_req_version, required: get_option('libthai'))
if thai_dep.found()
  pango_conf.set('HAVE_LIBTHAI', 1)
  pango_deps += thai_dep

  if cc.has_function('th_brk_find_breaks', dependencies: thai_dep)
    pango_conf.set('HAVE_TH_BRK_FIND_BREAKS', 1)
  endif
endif

x11_dep = dependency('x11', required: false)

harfbuzz_dep = dependency('harfbuzz',
                          version: harfbuzz_req_version,
                          required: true,
                          fallback: ['harfbuzz', 'libharfbuzz_dep'],
                          default_options: ['coretext=enabled'])

pango_deps += harfbuzz_dep

if host_system == 'linux'
  fontconfig_dep = dependency('fontconfig', version: fontconfig_req_version, required: true)

  pango_deps += fontconfig_dep
  pango_conf.set('HAVE_FONTCONFIG', 1)
endif

if host_system == 'darwin'
  if not cc.links('''#include <CoreText/CoreText.h>
                              int main (void) {
                                CTGetCoreTextVersion ();
                                return 0;
                              }''',
                           name: 'CoreText availability',
                           dependencies: dependency('appleframeworks', modules: 'ApplicationServices'))
    error ('CoreText is required')
  endif

  pango_conf.set('HAVE_CORE_TEXT', 1)

  pango_deps += dependency('appleframeworks', modules: [ 'CoreFoundation', 'ApplicationServices' ])
endif

if host_system == 'windows'
  pango_conf.set('HAVE_DIRECT_WRITE', 1)

  pango_deps += [
    cc.find_library('gdi32'),
    cc.find_library('dwrite'),
  ]
endif

# If option is 'auto' or 'enabled' it is not required to find cairo on the
# system because a fallback is done at the end.
cairo_option = get_option('cairo')
if not cairo_option.disabled()
  cairo_option = false
endif

cairo_dep = dependency('cairo', version: cairo_req_version,
                       fallback: ['cairo', 'libcairo_dep'], required: get_option('cairo'))

pango_conf.set('HAVE_CAIRO', cairo_dep.found ())
pango_deps += cairo_dep

cairo_png_dep = dependency('cairo-png', required: false)
if cairo_png_dep.found()
  pango_conf.set('HAVE_CAIRO_PNG', 1)
endif

cairo_xlib_dep = dependency('cairo-xlib', required: false)
if cairo_xlib_dep.found()
  pango_conf.set('HAVE_CAIRO_XLIB', 1)
endif

# libsysprof-capture support
libsysprof_capture_dep = dependency('sysprof-capture-4',
  required: get_option('sysprof'),
  default_options: [
    'enable_examples=false',
    'enable_gtk=false',
    'enable_tests=false',
    'enable_tools=false',
    'libsysprof=false',
    'with_sysprofd=none',
    'help=false',
  ],
  fallback: ['sysprof', 'libsysprof_capture_dep'],
)
pango_conf.set('HAVE_SYSPROF', libsysprof_capture_dep.found())
pango_deps += libsysprof_capture_dep

gidocgen_dep = dependency('gi-docgen', version: '>= 2021.1',
                          fallback: ['gi-docgen', 'dummy_dep'],
                          required: get_option('docs'))

gnome = import('gnome')
pkgconfig = import('pkgconfig')

# Internal configuration header
configure_file(output: 'config.h', configuration: pango_conf)

root_inc = include_directories('.')
pango_inc = include_directories('pango')

subdir('pango')
subdir('utils')
subdir('examples')
subdir('tests')
subdir('tools')

if get_option('docs')
  subdir('docs')
endif

if not meson.is_subproject()
  meson.add_dist_script('build-aux/meson/dist-docs.py')
endif

summary('Fontconfig support', host_system == 'linux', section: 'Features')
summary('CoreText support', host_system == 'darwin', section: 'Features')
summary('DirectWrite support', host_system == 'windows', section: 'Features')
summary('Cairo support', cairo_dep.found(), section: 'Features')
summary('Thai support', thai_dep.found(), section: 'Features')
summary('Sysprof support', libsysprof_capture_dep.found(), section: 'Features')

summary('Compiler', cc.get_id(), section: 'Toolchain')
summary('Linker', cc.get_linker_id(), section: 'Toolchain')

summary('Debugging', get_option('debug'), section: 'Build')
summary('Optimization', get_option('optimization'), section: 'Build')
summary('Introspection', get_option('introspection').enabled(), section: 'Build')
summary('Documentation', get_option('docs'), section: 'Build')
summary('Install tests', get_option('install-tests'), section: 'Build')

summary('prefix', pango_prefix, section: 'Directories')
summary('includedir', pango_includedir, section: 'Directories')
summary('libdir', pango_libdir, section: 'Directories')
summary('datadir', pango_datadir, section: 'Directories')
