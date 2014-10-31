Building in VC7.1
-----------------

The .sln and .vcproj in this folder have been designed to replicate
the build environment normally created by the hcustom tool - this is
to allow more convenient building for the rest of us who like to use
our IDEs, and also allows more control over some of the build
settings.

The project is currently set up identically to the CFLAGS reported
from "hcustom -d", with one difference - when building in debug mode
we use the Debug CRT runtime library (/MDd). This is required for
successfully linking to the debug Ogrelibraries. If this ends up
conflicting with Houdini we may have to rethink how we link in debug
mode.

There are 2 requirements to launch the build:
1. You've defined an environment variable 'HOUDINI_HOME' which points at your Houdini root directory.
2. You've defined an environment variable 'HOUDINI_OGREMAIN' which points at your OgreMain root folder (should have subdirs include and lib).

For installing the built exporter, we suggest using the NSIS
(http://nsis.sourceforge.net/Main_Page) script in the installer
subdirectory.  This will create a windows MSI installer.

