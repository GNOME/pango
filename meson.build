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

# These are for the various .pc files so that things will link
# properly, depending on whether we have the pkg-config files
# for those non-GNOME dependencies, or when we find them manually
# for MSVC builds, as their MSVC build systems do not generate
# pkg-config files for them
cairo_pc = ''
cairo_lib = ''
freetype2_pc=''
fontconfig_pc=''

harfbuzz_dep = dependency('harfbuzz',
                          version: harfbuzz_req_version,
                          required: true,
                          fallback: ['harfbuzz', 'libharfbuzz_dep'],
                          default_options: ['coretext=enabled'])

pango_deps += harfbuzz_dep

# If option is 'auto' or 'enabled' it is not required to find fontconfig on the
# system because a fallback is done at the end. Override 'disabled' option on
# platforms that requires it.
fontconfig_option = get_option('fontconfig')
fontconfig_required = host_system not in ['windows', 'darwin']
if not fontconfig_option.disabled() or fontconfig_required
  fontconfig_option = false
endif

fontconfig_dep = dependency('fontconfig', version: fontconfig_req_version, required: fontconfig_option)
if fontconfig_dep.found()
  fontconfig_pc = 'fontconfig'
else
  if cc.get_id() == 'msvc' and cc.has_header('fontconfig/fontconfig.h')
    # Look for the Visual Studio-style import library if FontConfig's .pc file cannot be
    # found on Visual Studio
    fontconfig_dep = cc.find_library('fontconfig', required: fontconfig_option)
  endif
endif

# Do the fallback now if fontconfig has not been found on the system. Override
# user option on platforms that always require fontconfig.
fontconfig_option = fontconfig_required ? true : get_option('fontconfig')
if not fontconfig_dep.found()
  fontconfig_dep = dependency('fontconfig', version: fontconfig_req_version,
                              fallback: ['fontconfig', 'fontconfig_dep'],
                              required: fontconfig_option)
endif

if fontconfig_dep.found()
  pango_deps += fontconfig_dep

  if fontconfig_dep.type_name() != 'library'
    if fontconfig_dep.version().version_compare('>=2.12.92')
      pango_conf.set('HAVE_FCWEIGHTFROMOPENTYPEDOUBLE', 1)
    endif
  elif cc.has_function('FcWeightFromOpenTypeDouble', dependencies: fontconfig_dep)
    pango_conf.set('HAVE_FCWEIGHTFROMOPENTYPEDOUBLE', 1)
  endif
endif

if pango_conf.has('HAVE_FCWEIGHTFROMOPENTYPEDOUBLE')
  res = 'YES'
else
  res = 'NO'
endif
message('fontconfig has FcWeightFromOpenTypeDouble: ' + res)

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
freetype_package_name = cc.get_argument_syntax() == 'msvc' ? \
                        'freetype' : 'freetype2'

freetype_dep = dependency(freetype_package_name,
                          required: freetype_option,
                          fallback: ['freetype2', 'freetype_dep'])

if freetype_dep.found() and freetype_dep.type_name() == 'pkgconfig'
  freetype2_pc = 'freetype2'
endif

# To build pangoft2, we need HarfBuzz, FontConfig and FreeType
build_pangoft2 = fontconfig_dep.found() and freetype_dep.found()
if build_pangoft2
  pango_conf.set('HAVE_FREETYPE', 1)
  pango_deps += freetype_dep
endif

x11_dep = dependency('x11', required: false)

has_core_text = false
if host_system == 'darwin'
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

# If option is 'auto' or 'enabled' it is not required to find cairo on the
# system because a fallback is done at the end.
cairo_option = get_option('cairo')
if not cairo_option.disabled()
  cairo_option = false
endif

cairo_found_type = ''
cairo_dep = dependency('cairo', version: cairo_req_version, required: cairo_option)

if cairo_dep.found()
  cairo_found_type = cairo_dep.type_name()
else
  if cc.get_id() == 'msvc' and cc.has_header('cairo.h')
    cairo_dep = cc.find_library('cairo', required: cairo_option)
    cairo_found_type = 'library'
  endif
endif

# Remove once Meson gains capability to declare dependencies
# in a declarative way
if not cairo_dep.found()
  cairo_dep = dependency('cairo', version: cairo_req_version,
                         fallback: ['cairo', 'libcairo_dep'], required: get_option('cairo'))
  cairo_found_type = cairo_dep.type_name()
endif

if cc.has_function('cairo_user_font_face_set_render_color_glyph_func', args: '-lcairo', dependencies: cairo_dep)
  pango_conf.set('HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC', 1)
endif

pango_font_backends = []
pango_cairo_backends = []

if ['pkgconfig', 'internal'].contains(cairo_found_type)
  # Check the following Cairo font backends
  # - dependency
  # - version
  # - define
  # - backend name
  # Note that Cairo can be built with FreeType but without FontConfig

  cairo_font_backends = [
    [ 'cairo-ft', cairo_req_version, 'HAVE_CAIRO_FREETYPE', 'freetype', ],
    [ 'cairo-win32', cairo_req_version, 'HAVE_CAIRO_WIN32', 'win32', ],
    [ 'cairo-quartz', cairo_req_version, 'HAVE_CAIRO_QUARTZ', 'quartz', ],
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
      if b[0] == 'cairo-ft' and not build_pangoft2
        continue
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
    [ 'cairo-png', cairo_req_version, 'HAVE_CAIRO_PNG', 'png', ],
    [ 'cairo-ps', cairo_req_version, 'HAVE_CAIRO_PS', 'ps', ],
    [ 'cairo-pdf', cairo_req_version, 'HAVE_CAIRO_PDF', 'pdf', ],
    [ 'cairo-xlib', cairo_req_version, 'HAVE_CAIRO_XLIB', 'xlib', ],
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

  # This is to set up pangocairo.pc so that things that refer to
  # it will link correctly
  cairo_pc = 'cairo'
elif cairo_found_type == 'library'
  # Fallback: Look for Cairo items manually
  # We need to check for headers for some
  cairo_headers = [ 'win32', 'quartz', 'ps', 'pdf', 'xlib' ]

  foreach header : cairo_headers
    if cc.has_header('cairo-@0@.h'.format(header))
      pango_conf.set('HAVE_CAIRO_@0@'.format(header.underscorify().to_upper()), 1)
      if header == 'win32'
        pango_font_backends += header
      elif header == 'quartz'
        if pango_conf.has('HAVE_CORE_TEXT')
          pango_font_backends += header
        endif
      else
        pango_cairo_backends += header
      endif
    endif
  endforeach

  # Other cairo features that we need to check for symbols
  # Note that Cairo with FreeType support can be without FontConfig
  # support, so we must check for FontConfig support in Cairo
  if cc.links('''#include <cairo-ft.h>
                 int main() {
                   FcPattern *p = NULL;
                   cairo_ft_font_face_create_for_pattern (p);
                   return 0;
                 }''',
                 dependencies: cairo_dep,
                 name : 'Cairo is built with FreeType and FontConfig support')
    if build_pangoft2
      pango_conf.set('HAVE_CAIRO_FREETYPE', 1)
      pango_font_backends += 'freetype'
    endif
  endif

  if pango_font_backends.length() == 0
    error('Cairo found, but no Cairo font backends found')
  endif

  # Check for Cairo's libpng output surface support
  if cc.links('''#include <cairo.h>
                 #include <stdio.h>
                 int main() {
                   cairo_surface_t *s = NULL;
                   const char *f = "abc.png";
                   cairo_surface_write_to_png (s, f);
                   return 0;
                 }''',
                 dependencies: cairo_dep,
                 name : 'Cairo is built with PNG output surface support')
    pango_conf.set('HAVE_CAIRO_PNG', 1)
    pango_cairo_backends += 'png'
  endif

  # This is to set up pangocairo.pc so that things that refer to
  # it will link correctly, when we don't have pkg-config files for cairo
  cairo_lib = '-lcairo'
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
                          required: get_option('gtk_doc'))

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

if get_option('gtk_doc')
  subdir('docs')
endif

if not meson.is_subproject()
  meson.add_dist_script('build-aux/meson/dist-docs.py')
endif

summary('Font backends', pango_font_backends)

summary('Cairo support', cairo_dep.found(), section: 'Features')
summary('Fontconfig support', fontconfig_dep.found(), section: 'Features')
summary('Freetype support', freetype_dep.found(), section: 'Features')
summary('Thai support', thai_dep.found(), section: 'Features')
summary('Sysprof support', libsysprof_capture_dep.found(), section: 'Features')

summary('Compiler', cc.get_id(), section: 'Toolchain')
summary('Linker', cc.get_linker_id(), section: 'Toolchain')

summary('Debugging', get_option('debug'), section: 'Build')
summary('Optimization', get_option('optimization'), section: 'Build')
summary('Introspection', get_option('introspection').enabled(), section: 'Build')
summary('Documentation', get_option('gtk_doc'), section: 'Build')
summary('Install tests', get_option('install-tests'), section: 'Build')

summary('prefix', pango_prefix, section: 'Directories')
summary('includedir', pango_includedir, section: 'Directories')
summary('libdir', pango_libdir, section: 'Directories')
summary('datadir', pango_datadir, section: 'Directories')
