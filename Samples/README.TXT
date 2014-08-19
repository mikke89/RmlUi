=====================================================================
 libRocket Sample Applications
=====================================================================

This directory contains a collection of sample applications
that demonstrate the use of libRocket in small, easy
to understand applications.

To build the samples, you can either use Microsoft Visual Studio
(using the project files found within each directory), or use SCons
(SConstruct files are present in directories except those for
Windows-specific samples).

If you build with Visual Studio, make sure the actual sample is the
'startup project' (displayed in bold) rather than the sample shell
project.

If you build with SCons, be sure to compile the sample shell (found
in /samples/shell/) before you compile other samples. All the
SConstructs will build a debug build by default; to build a release
version, run 'scons release'.

When compiling Python examples, you will need to set up your build
paths to point at the local copy of Python and Boost. If using Visual
Studio, change these in the project's include and library paths. If
using SCons, edit /build/config.py and change the paths as
appropriate.

Directory Overview:

 * assets     - This directory contains the assets shared by
                all the sample applications.
 * basic      - This directory contains basic applications
                that demonstrate initialisation, shutdown and
                installing custom interfaces.

                  * customlog    - setting up custom logging
                  * directx      - using DirectX 9 as a renderer
                  * directx10    - using DirectX 10 as a renderer
                  * drag         - dragging elements between containers
                  * loaddocument - loading your first document
                  * ogre3d       - interfacing with Ogre3D engine
                  * treeview     - using a nested DataSource

 * invaders   - A full implementation of the 1970s classic
                Space Invaders using the libRocket interface.

 * pyinvaders - Python version of the above (only installed
                with the Python plugin).

 * luainvaders- Lua version of the invaders (only installed
                with the Lua plugin).

 * shell      - Common platform specific code used by all the 
                samples for open windows, processing input and 
                access files.
                Supports Windows, MacOSX and Linux

 * tutorial   - Tutorial code that should be used in conjunction
                with the tutorials on the libRocket website.
                http://www.librocket.com/wiki/documentation

=====================================================================
