project('pango', 'c', 'cpp',
        version: '1.57.1',
        license: 'LGPLv2.1+',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=gnu11',
        ],
        meson_version : '>= 1.2.0')

add_project_arguments([
    '-D_POSIX_C_SOURCE=200809L',
    '-D_POSIX_THREAD_SAFE_FUNCTIONS',
    '-D_GNU_SOURCE',
  ],
  language: 'c'
)

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
cpp = meson.get_compiler('cpp')
host_system = host_machine.system()

if host_system == 'windows'
  add_project_arguments('-D_WIN32_WINNT=_WIN32_WINNT_WIN10', language: 'c')
endif

# Compiler and linker flags
common_cflags = []
common_cppflags = []
common_ldflags = []

# Add more compiler warnings to the default set
if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = ['-FImsvc_recommended_pragmas.h', '-utf-8']
  add_project_arguments(cc.get_supported_arguments(test_cflags), language: 'c')
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

glib_major_req = 2
glib_minor_req = 82

glib_req       = '>= @0@.@1@'.format(glib_major_req, glib_minor_req)
gi_req         = '>= 1.83.2'
fribidi_req    = '>= 1.0.6'
libthai_req    = '>= 0.1.9'
harfbuzz_req   = '>= 8.4.0'
fontconfig_req = '>= 2.17.0'
xft_req        = '>= 2.0.0'
cairo_req      = '>= 1.18.0'

add_project_arguments([
    '-DG_LOG_USE_STRUCTURED=1',
    '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req),
    '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req),

  ],
  language: 'c'
)

mathlib_dep = cc.find_library('m', required: false)
glib_dep = dependency('glib-2.0', version: glib_req)
gobject_dep = dependency('gobject-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: glib_req)
gi_dep = dependency('gobject-introspection-1.0', version: gi_req, required: get_option('introspection').enabled())
fribidi_dep = dependency('fribidi', version: fribidi_req, default_options: ['docs=false'])
pango_deps += [mathlib_dep, glib_dep, gobject_dep, gio_dep, fribidi_dep]

thai_dep = dependency('libthai', version: libthai_req, required: get_option('libthai'))
if thai_dep.found()
  pango_conf.set('HAVE_LIBTHAI', 1)
  pango_deps += thai_dep

  if cc.has_function('th_brk_find_breaks', dependencies: thai_dep)
    pango_conf.set('HAVE_TH_BRK_FIND_BREAKS', 1)
  endif
endif

# These are for the various .pc files so that things will link
# properly, depending on whether we have the pkg-config files
# for those non-GNOME dependencies, or when we find them manually
# for MSVC builds, as their MSVC build systems do not generate
# pkg-config files for them
freetype2_pc = ''
freetype_lib = []

# Platforms where fontconfig is required
fontconfig_required = host_system not in ['windows', 'darwin']
# Disable fontconfig by default on platforms where it is optional
fontconfig_option = get_option('fontconfig').disable_auto_if(not fontconfig_required)

harfbuzz_fallback_options = [
  'coretext=enabled',
  'directwrite=enabled',
  'gdi=enabled',
  'cairo=disabled',
  'docs=disabled',
  'utilities=disabled',
  'tests=disabled',
]

# Enable FreeType support in HarfBuzz if building it as a
# subproject and FontConfig support is required or requested
if fontconfig_required or fontconfig_option.enabled()
  harfbuzz_fallback_options += 'freetype=enabled'
else
  harfbuzz_fallback_options += 'freetype=disabled'
endif

harfbuzz_dep = dependency('harfbuzz',
                          version: harfbuzz_req,
                          default_options: harfbuzz_fallback_options)

pango_deps += harfbuzz_dep
# In meson 1.1.0, this check can be replaced with .enable_if() to simplify the
# logic in the fallback handling.
if fontconfig_option.disabled() and fontconfig_required
  error('Fontconfig support cannot be disabled on this platform')
endif

# Override user option 'auto' to 'enabled' on platforms that always
# require fontconfig.
fontconfig_option = fontconfig_required ? true : fontconfig_option
fontconfig_dep = dependency('fontconfig', version: fontconfig_req,
                            required: fontconfig_option)

if fontconfig_dep.found()
  pango_deps += fontconfig_dep
endif

# If option is 'auto' or 'enabled' it is not required to find freetype2 on the
# system because a fallback is done at the end. Override 'disabled' option on
# if fontconfig has been found.
freetype_option = get_option('freetype')
freetype_required = fontconfig_dep.found()
if not freetype_option.disabled() or freetype_required
  freetype_option = false
endif

# The first version of freetype with a pkg-config file is 2.1.5,
# CMake uses 'freetype' rather than 'freetype2' for the package name
# Disable HarfBuzz support in FreeType if HarfBuzz *and* FreeType are
# being built as subprojects
freetype_fallback_options = []
if harfbuzz_dep.type_name() == 'internal'
  freetype_fallback_options = ['harfbuzz=disabled']
endif
freetype_dep = dependency(['freetype2', 'freetype'],
                          required: freetype_option,
                          default_options: freetype_fallback_options)

if freetype_dep.found() and freetype_dep.type_name() in ['pkgconfig', 'internal']
  freetype2_pc = 'freetype2'
else
  freetype_lib = freetype_dep
endif

# To build pangoft2, we need HarfBuzz, FontConfig and FreeType
build_pangoft2 = fontconfig_dep.found() and freetype_dep.found()
if build_pangoft2
  pango_conf.set('HAVE_FREETYPE', 1)
  pango_deps += freetype_dep
endif

build_pangoxft = false
xft_dep = dependency('xft', version: xft_req, required: get_option('xft'))
if xft_dep.found() and fontconfig_dep.found() and freetype_dep.found()
  pango_conf.set('HAVE_XFT', 1)
  pango_deps += dependency('xrender', required: false)
  pango_deps += xft_dep
  build_pangoxft = true
endif

has_core_text = false
if host_system == 'darwin'
  # for viewer-cocoa.m
  add_languages('objc', native : false)

  has_core_text = cc.links('''#include <CoreText/CoreText.h>
                              int main (void) {
                                CTGetCoreTextVersion ();
                                return 0;
                              }''',
                           name: 'CoreText availability',
                           dependencies: dependency('appleframeworks', modules: 'ApplicationServices'))
  if has_core_text
    pango_conf.set('HAVE_CORE_TEXT', 1)
  endif

  pango_deps += dependency('appleframeworks', modules: [ 'CoreFoundation', 'ApplicationServices' ])
endif

# Check for dwrite_3.h (from more recent Windows SDK or mingw-w64)
if host_system == 'windows' and not cpp.has_header('dwrite_3.h')
  error('Windows backend enabled but dwrite_3.h not found.')
endif

# Enable cairo-ft with FreeType and FontConfig support if
# building as a subproject and FontConfig support is required
# or requested
cairo_fallback_options = []
if fontconfig_required or fontconfig_option.enabled()
  cairo_fallback_options = [
    'freetype=enabled',
    'fontconfig=enabled',
  ]
endif

cairo_dep = dependency(
  'cairo',
  version: cairo_req,
  required: get_option('cairo'),
  default_options: cairo_fallback_options,
)
cairo_dwrite_dep = disabler() # Only for Windows
cairo_found_type = cairo_dep.type_name()

# Check for Windows-only features from HarfBuzz
if host_system == 'windows'
  if cairo_dep.found()
    # cairo < 1.17.8 used a different name
    cairo_dwrite_dep = dependency(
      ['cairo-dwrite-font', 'cairo-win32-dwrite-font'],
      version: cairo_req,
      required: false
    )
  endif

  # Use hb-directwrite if we are also using cairo-dwrite-font,
  # or if we are (unlikely) not enabling Cairo support
  if not cairo_dep.found() or cairo_dwrite_dep.found()
    if harfbuzz_dep.type_name() == 'internal' or \
       cpp.has_function('hb_directwrite_face_create', dependencies: harfbuzz_dep)
      pango_conf.set('USE_HB_DWRITE', 1)
    endif
  else
    if harfbuzz_dep.type_name() == 'internal' or \
       cc.has_function('hb_gdi_face_create', dependencies: harfbuzz_dep)
      pango_conf.set('USE_HB_GDI', 1)
    endif
  endif
endif

pango_font_backends = []
pango_cairo_backends = []

if cairo_dep.found()
  # Check the following Cairo font backends
  # - dependency
  # - version
  # - define
  # - backend name
  # Note that Cairo can be built with FreeType but without FontConfig

  cairo_font_backends = [
    [ 'cairo-ft', cairo_req, 'HAVE_CAIRO_FREETYPE', 'freetype', ],
    [ 'cairo-win32', cairo_req, 'HAVE_CAIRO_WIN32', 'win32', ],
    [ 'cairo-quartz', cairo_req, 'HAVE_CAIRO_QUARTZ', 'quartz', ],
  ]

  if cairo_found_type == 'internal'
    cairo_features = subproject('cairo').get_variable('built_features')
  endif

  foreach b: cairo_font_backends
    if cairo_found_type == 'pkgconfig'
      dep = dependency(b[0], version: b[1], required: false)
    else
      dep = dependency('', required: false)
      foreach f: cairo_features
        if f['name'] == b[0]
          dep = cairo_dep
          message('Cairo font backend "@0@" enabled'.format(b))
        endif
      endforeach
    endif
    if dep.found()
      if b[0] == 'cairo-ft'
        if not build_pangoft2
          continue
        else
          # Check external cairo-ft has the required FontConfig support
          if dep.type_name() != 'internal'
            cairo_fc_test = '''
#include <cairo-ft.h>
int main() {
  FcPattern *p = NULL;
  cairo_ft_font_face_create_for_pattern (p);
  return 0;
}'''
            if not cc.links(cairo_fc_test,
                            dependencies: dep,
                            name : 'Cairo is built with FreeType and FontConfig support')
              error('@0@ does not have the required FontConfig support'.format(b[0]))
            endif
          endif
        endif
      endif
      if b[0] == 'cairo-quartz' and not pango_conf.has('HAVE_CORE_TEXT')
        continue
      endif
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
    [ 'cairo-png', cairo_req, 'HAVE_CAIRO_PNG', 'png', ],
    [ 'cairo-ps', cairo_req, 'HAVE_CAIRO_PS', 'ps', ],
    [ 'cairo-pdf', cairo_req, 'HAVE_CAIRO_PDF', 'pdf', ],
    [ 'cairo-xlib', cairo_req, 'HAVE_CAIRO_XLIB', 'xlib', ],
  ]

  foreach b: cairo_surface_backends
    if cairo_found_type == 'pkgconfig'
      dep = dependency(b[0], version: b[1], required: false)
    else
      dep = dependency('', required: false)
      foreach f: cairo_features
        if f['name'] == b[0]
          dep = cairo_dep
          message('Cairo surface backend "@0@" enabled'.format(b))
        endif
      endforeach
    endif
    if dep.found()
      pango_conf.set(b[2], 1)
      pango_cairo_backends += b[3]
    endif
  endforeach
endif

if cairo_dep.found()
  pango_conf.set('HAVE_CAIRO', 1)
  pango_deps += cairo_dep

  pangocairo_requires = ''

  if pango_font_backends.contains('freetype')
    pangocairo_requires += 'pangoft2 '
  endif

  if pango_font_backends.contains('win32')
    pangocairo_requires += 'pangowin32 '
  endif
endif

if cairo_dwrite_dep.found()
  pango_conf.set('HAVE_CAIRO_WIN32_DIRECTWRITE', 1)
endif

# introspection
gir = find_program('g-ir-scanner', required : get_option('introspection'))

if not gir.found() and get_option('introspection').enabled()
  error('Introspection enabled, but g-ir-scanner not found.')
endif

build_gir = gi_dep.found() and gir.found() and (get_option('introspection').enabled() or
                                               (get_option('introspection').allowed() and get_option('documentation')))

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
)
pango_conf.set('HAVE_SYSPROF', libsysprof_capture_dep.found())
pango_deps += libsysprof_capture_dep

gnome = import('gnome')
pkgconfig = import('pkgconfig')

# Internal configuration header
configure_file(output: 'config.h', configuration: pango_conf)

root_inc = include_directories('.')
pango_inc = include_directories('pango')

subdir('pango')
subdir('utils')
subdir('tools')
subdir('docs')
if get_option('build-testsuite')
  subdir('tests')
endif
if get_option('build-examples')
  subdir('examples')
endif

if not meson.is_subproject()
  meson.add_dist_script('build-aux/meson/dist-docs.py')
endif

summary('Fontconfig', pango_font_backends.contains('freetype'), section: 'Font backends')
summary('DirectWrite', pango_font_backends.contains('win32'), section: 'Font backends')
summary('CoreText', pango_font_backends.contains('quartz'), section: 'Font backends')

summary('Cairo', cairo_dep.found(), section: 'Features')
summary('Thai', thai_dep.found(), section: 'Features')
summary('Sysprof', libsysprof_capture_dep.found(), section: 'Features')

summary('Compiler', cc.get_id(), section: 'Toolchain')
summary('Linker', cc.get_linker_id(), section: 'Toolchain')

summary('Debugging', get_option('debug'), section: 'Build')
summary('Optimization', get_option('optimization'), section: 'Build')
summary('Introspection', build_gir, section: 'Build')
summary('Documentation', get_option('documentation'), section: 'Build')
summary('Man pages', get_option('man-pages'), section: 'Build')
summary('Tests', get_option('build-testsuite'), section: 'Build')
summary('Examples', get_option('build-examples'), section: 'Build')

summary('prefix', pango_prefix, section: 'Directories')
summary('includedir', pango_includedir, section: 'Directories')
summary('libdir', pango_libdir, section: 'Directories')
summary('datadir', pango_datadir, section: 'Directories')
