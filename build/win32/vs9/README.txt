Note that all this is rather experimental.

This VS9 solution and the projects it includes are intented to be used
in a Pango source tree unpacked from a tarball. In a git checkout you
first need to use some Unix-like environment or manual work to expand
files as needed, for instance the .vcprojin files here into .vcproj
files.

Set up the source tree as follows under some arbitrary top folder
<root>:

<root>\pango\<this-pango-source-tree>
<root>\vs9\<PlatformName>

*this* file you are now reading is thus located at
<root>\pango\<this-pango-source-tree>\build\win32\vs9\README.txt.

<PlatformName> is either Win32 or x64, as in VS9 project files.

You should unpack the glib-dev zip file into
<root>\vs9\<PlatformName>, so that for instance glib.h ends up at
<root>\vs9\<PlatformName>\include\glib-2.0\glib.h.

The "install" project will copy build results and headers into their
appropriate location under <root>\vs9\<PlatformName>. For instance,
built DLLs go into <root>\vs9\<PlatformName>\bin, built LIBs into
<root>\vs9\<PlatformName>\lib and headers into
<root>\vs9\<PlatformName>\include\pangpo-1.0. This is then from where
project files higher in the stack are supposed to look for them, not
from a specific Pango source tree like this one. It is important to
keep separate the concept of a "source tree", where also non-public
headers are present, and an "install tree" where only public headers
are present.

--Tor Lillqvist <tml@iki.fi>
