Freetype 2.4.10 is required to compile this project.


The project searches the ../../../support folder. (Create a folder called support, next to the LibRocket repository folder)

Compile freetype v2.4.10 and copy the following files to the 'support/lib' folder:

   freetype2410.lib
   freetype2410_D.lib



The lib search path (relative to project file) is: 
   ../../../support/lib

Also required is the freetype includes. Copy the 'include' folder in the freetype repository root folder and paste it in the following path:
  support/freetype-2.4.10/


The freetype library can be downloaded from
   http://sourceforge.net/projects/freetype/files/

Or navigated to via
   http://www.freetype.org


Two options in the project has to be updated if an older or newer version of freetype is desired to be used.
RocketCore -> Properties -> C/C++ -> General -> Additional Include Directies:
  update the path to ..\..\..\support\freetype-2.4.10\include to the new path, -same for all configurations-

RocketCore -> Properties -> Linker -> Input -> Additional Dependencies:
  update separately for debug and release build to the new freetype*.lib version.