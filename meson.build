project('pango', 'c', 'cpp',
        version: '1.41.0',
        license: 'LGPLv2.1+',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=c99',
        ],
        meson_version : '>= 0.40.1')

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

pango_interface_age = pango_minor_version.is_odd() ? 0 : pango_micro_version
pango_binary_age = pango_minor_version * 100 + pango_micro_version

pango_api_version = '@0@.0'.format(pango_major_version)
pango_api_path = 'pango-@0@/pango'.format(pango_api_version)

pango_conf = configuration_data()

pango_conf.set('PANGO_BINARY_AGE', pango_binary_age)
pango_conf.set('PANGO_INTERFACE_AGE', pango_interface_age)
pango_conf.set('PANGO_VERSION_MAJOR', pango_major_version)
pango_conf.set('PANGO_VERSION_MINOR', pango_minor_version)
pango_conf.set('PANGO_VERSION_MICRO', pango_micro_version)

# Maintain version scheme with libtool
pango_soversion = 0
pango_libversion = '@0@.@1@.@2@'.format(pango_soversion, (pango_binary_age - pango_interface_age), pango_interface_age)

cc = meson.get_compiler('c')
host_system = host_machine.system()

# Compiler and linker flags
common_cflags = []
common_cppflags = []
common_ldflags = []

# Add more compiler warnings to the default set
if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = []
  add_project_arguments([ '-FImsvc_recommended_pragmas.h' ], language: 'c')
  test_c_only_flags = []
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_c_only_flags = [
    '-Wimplicit-function-declaration',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wno-int-conversion',
    '-Wno-discarded-qualifiers',
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
    '-Werror=redundant-decls',
    '-Werror=implicit',
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
    '-Werror=pointer-to-int-cast',
    '-Werror=empty-body',
    '-Werror=write-strings',
    '-Werror=undef',
  ]

  if host_system == 'windows'
    test_cflags += [ '-mms-bitfields' ]
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
foreach cflag: test_cflags
  if cc.has_argument(cflag)
    common_cflags += [ cflag ]
  endif
endforeach

# Isolate the C++ compiler flags
foreach cflag: common_cflags
  if not test_c_only_flags.contains(cflag)
    common_cppflags += [ cflag ]
  endif
endforeach

# Linker flags
if host_machine.system() == 'linux'
  foreach ldflag: [ '-Wl,-Bsymbolic', '-Wl,-z,relro', '-Wl,-z,now', ]
    if cc.has_argument(ldflag)
      common_ldflags += [ ldflag ]
    endif
  endforeach
endif

# Maintain compatibility with autotools on macOS
if host_machine.system() == 'darwin'
  common_ldflags += [ '-compatibility_version=1', '-current_version=1.0', ]
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

buildtype = get_option('buildtype')
if buildtype.startswith('debug')
  pango_debug_cflags = [ '-DPANGO_ENABLE_DEBUG', ]
elif buildtype == 'release'
  pango_debug_cflags = [ '-DG_DISABLE_CAST_CHECKS', ]
  # TODO: We may want a configuration argument to add `G_DISABLE_CHECKS`
  # and `G_DISABLE_ASSERT` from the build, for specific build environments.
  # On the other hand, those who need these symbols can inject them in their
  # build as well.
else
  pango_debug_cflags = []
endif

# Dependencies
pango_deps = []

glib_req_version = '>= 2.33.12'
libthai_req_version = '>= 0.1.9'
harfbuzz_req_version = '>= 1.2.3'
fontconfig_req_version = '>= 2.11.91'
xft_req_version = '>= 2.0.0'
cairo_req_version = '>= 1.12.10'

# libm
mathlib_dep = cc.find_library('m', required: false)
pango_deps += mathlib_dep

# gobject
gobject_dep = dependency('gobject-2.0', version: glib_req_version)
pango_deps += gobject_dep

thai_dep = dependency('libthai', version: libthai_req_version, required: false)
if thai_dep.found()
  pango_conf.set('HAVE_LIBTHAI', 1)
  pango_deps += thai_dep
endif

harfbuzz_dep = dependency('harfbuzz', version: harfbuzz_req_version, required: false)
if harfbuzz_dep.found()
  pango_deps += harfbuzz_dep
endif

fontconfig_dep = dependency('fontconfig', version: fontconfig_req_version, required: false)
if fontconfig_dep.found()
  pango_deps += fontconfig_dep

  if cc.has_function('FcWeightFromOpenTypeDouble', dependencies: fontconfig_dep)
    pango_conf.set('HAVE_FCWEIGHTFROMOPENTYPEDOUBLE', 1)
  endif
endif

# The first version of freetype with a pkg-config file is 2.1.5
freetype_dep = dependency('freetype2', required: false)
if freetype_dep.found()
  pango_conf.set('HAVE_FREETYPE', 1)
  pango_deps += freetype_dep
endif

xft_dep = dependency('xft', version: xft_req_version, required: false)
if xft_dep.found()
  pango_conf.set('HAVE_XFT', 1)
  pango_deps += dependency('xrender', required: false)
  pango_deps += xft_dep
endif

if host_system == 'darwin'
  has_core_text = cc.links('''#include <Carbon/Carbon.h>
                              int main (void) {
                                CTGetCoreTextVersion ();
                                return 0;
                              }''',
                           name: 'CoreText availability',
                           dependencies: dependency('appleframeworks', modules: 'ApplicationServices'))
  if has_core_text
    pango_conf.set('HAVE_CORE_TEXT', 1)
  endif
endif

cairo_dep = dependency('cairo', version: cairo_req_version, required: false)
if cairo_dep.found()
  # Check the following Cairo font backends
  # - dependency
  # - version
  # - define
  # - backend name
  cairo_font_backends = [
    [ 'cairo-ft', cairo_req_version, 'HAVE_CAIRO_FREETYPE', 'freetype', ],
    [ 'cairo-win32', cairo_req_version, 'HAVE_CAIRO_WIN32', 'win32', ],
    [ 'cairo-quartz', cairo_req_version, 'HAVE_CAIRO_QUARTZ', 'quartz', ],
  ]

  pango_font_backends = []

  foreach b: cairo_font_backends
    dep = dependency(b[0], version: b[1], required: false)
    if dep.found()
      pango_conf.set(b[2], 1)
      pango_font_backends += b[3]
    endif
  endforeach

  if pango_font_backends.length() == 0
    error('No Cairo font backends found')
  endif

  # Check the following Cairo surface backends
  # - dependency
  # - version
  # - define
  # - backend name
  cairo_surface_backends = [
    [ 'cairo-png', cairo_req_version, 'HAVE_CAIRO_PNG', 'png', ],
    [ 'cairo-ps', cairo_req_version, 'HAVE_CAIRO_PS', 'ps', ],
    [ 'cairo-pdf', cairo_req_version, 'HAVE_CAIRO_PDF', 'pdf', ],
    [ 'cairo-xlib', cairo_req_version, 'HAVE_CAIRO_XLIB', 'xlib', ],
  ]

  pango_cairo_backends = []

  foreach b: cairo_surface_backends
    dep = dependency(b[0], version: b[1], required: false)
    if dep.found()
      pango_conf.set(b[2], 1)
      pango_cairo_backends += b[3]
    endif
  endforeach

  pango_conf.set('HAVE_CAIRO', 1)
  pango_deps += cairo_dep

  pangocairo_requires = []

  if pango_font_backends.contains('freetype')
    pangocairo_requires += 'pangoft2'
  endif

  if pango_font_backends.contains('win32')
    pangocairo_requires += 'pangowin32'
  endif
endif

# Compat variables for pkgconfig
pkgconf = configuration_data()
pkgconf.set('prefix', pango_prefix)
pkgconf.set('exec_prefix', pango_prefix)
pkgconf.set('libdir', pango_libdir)
pkgconf.set('includedir', pango_includedir)
pkgconf.set('VERSION', meson.project_version())
pkgconf.set('PANGO_API_VERSION', pango_api_version)
pkgconf.set('PKGCONFIG_CAIRO_REQUIRES', pangocairo_requires)

pkgconf_files = [
  [ 'pango.pc' ],
  [ 'pangowin32.pc', host_system == 'windows' ],
  [ 'pangoft2.pc', freetype_dep.found() ],
  [ 'pangoxft.pc', xft_dep.found() ],
  [ 'pangocairo.pc', cairo_dep.found() ],
]

foreach pkg: pkgconf_files
  pkg_name = pkg[0]
  pkg_enabled = pkg.get(1, true)

  if pkg_enabled
    configure_file(input: pkg_name + '.in',
                   output: pkg_name,
                   configuration: pkgconf,
                   install: true,
                   install_dir: join_paths(pango_libdir, 'pkgconfig'))
  endif
endforeach

gnome = import('gnome')

# Internal configuration header
configure_file(output: 'config.h', configuration: pango_conf)

root_inc = include_directories('.')

subdir('pango')
subdir('examples')
subdir('pango-view')
subdir('tests')
subdir('tools')

if get_option('enable_docs')
  subdir('docs')
endif
