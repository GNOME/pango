Using Pango for Windows
=======================

The Pango backend written for Win32 is pangowin32. Pangowin32 uses
the Win32 DirectWrite font API. GTK+ 2.8 and later on Win32 however
actually uses the pangocairo backend (which then uses only small parts of
pangowin32). Much of the DirectWrite font API calls are in cairo.

Building Pango for Windows
==========================

You need to have gcc (mingw-w64) or Visual Studio 2015 or later, along with
Python 3.6.x+ and Meson 0.55.3 at the time of writing.  However, if you do
not have HarfBuzz installed and are building it as part of building Pango
with Visual Studio, you will need to use Visual Studio 2017 (the latest
15.9.x release is recommended) or later.

If building the introspection files, you will also need to ensure that
the Python release series and build architecture (i.e. Win32 (x86)/
x64 (amd64/x86-64) matches the Python release series and build
architecture that was used to build GObject-Introspection.  Check whether
the Python installation/interpreter path indicated by the shebang line in
the `g-ir-scanner` script in your `%PATH%` matches your build
architecture.

The Ninja build utility must also be in your PATH, unless using the
Visual Studio IDE as noted below.

Cross-compiling Pango for ARM64/aarch64 with Visual Studio 2017 or later
is also supported, provided if a cross compilation file for Meson is
properly setup for such builds (please refer to the Meson documentation
for further instructions).  Support for introspection for ARM64 builds is
currently not supported, due to the lack of an official build of Python
for ARM64 Windows, and introspection builds are currently not supported
for any cross-builds.

You will also need the following libraries installed with their headers
and import libraries, and their DLLs, if applicable, needs to be found in
`%PATH%`.  All of their required dependencies are required as well.  Their
pkg-config files are needed on all builds:

- GLib
- Fribidi
- Cairo (With Win32+DirectWrite support built in, optionally libpng support)
- HarfBuzz (GLib, GObject and DirectWrite support required)

Please see `meson.build` to see what versions are needed for these dependencies.

Follow the following steps to build Pango:

1) Invoke the Meson configuration as follows, in a directory separate from the sources:

   (With MinGW, please adjust the paths accordingly, in a MSYS/MSYS2 bash prompt)
    ```
   PATH=/devel/dist/glib-2.8.0/bin:$PATH ACLOCAL_FLAGS="-I /devel/dist/glib-2.8.0/share/aclocal" PKG_CONFIG_PATH=/devel/dist/glib-2.8.0/lib/pkgconfig:$PKG_CONFIG_PATH CC='gcc -mtune=pentium3' CPPFLAGS='-I/opt/gnu/include' LDFLAGS='-L/opt/gnu/lib' CFLAGS=-O meson $(PATH_TO_SRC) --buildtype=$(buildtype) --prefix=$(PREFIX)
   ```

   (With Visual Studio, set the `INCLUDE`, `LIB`, `PATH` and envvars as
   needed before running the following in a Visual Studio command prompt)
   ```
   meson $(PATH_TO_SRC) --buildtype=$(buildtype) --prefix=$(PREFIX) [--pkg-config-path=...]
   ```

   For Visual Studio builds, support for building using the Visual Studio IDE
   is also supported to some extent.  Append `--backend=vs` to the Meson
   configuration command above to use this support.  Note that this support
   may not work as well as the builds that are carried out by Ninja, and
   issues in regards to building with Meson using Visual Studio with the IDE (i.e.
   `msbuild`) should be reported to the Meson project.

2) Build Pango by running Ninja, or by opening and building the generated
   Visual Studio solution file.
	
3) Run tests and/or install the build using the `test` and `install` targets
   or sub-projects respectively.

See the following GNOME Live! page for a more detailed description of building
Pango's dependencies with Visual Studio:

https://live.gnome.org/GTK%2B/Win32/MSVCCompilationOfGTKStack
