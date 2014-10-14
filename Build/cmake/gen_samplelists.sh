#!/usr/bin/env bash

basedir="../.."
file=Build/cmake/SampleFileList.cmake
src='set(sample_SRC_FILES'
hdr='set(sample_HDR_FILES'
srcdir='${PROJECT_SOURCE_DIR}'
srcpath=Samples
samples=( 'shell'
	'basic/customlog' 'basic/drag' 'basic/loaddocument' 'basic/ogre3d' 'basic/treeview' 'basic/transform'
	'basic/sdl2' 'basic/sfml' 'basic/sfml2'
	'basic/directx' 'basic/directx10'
	'tutorial/template' 'tutorial/datagrid' 'tutorial/datagrid_tree' 'tutorial/tutorial_drag'
	'invaders' 'luainvaders' 'pyinvaders'
)

printfiles() {
    # Print headers
    name=${1//basic\//} #substitute basic/ for nothing
    name=${name//tutorial\/} #substitute tutorial/ for nothing
    echo ${hdr/sample/$name} >>$file
    find  $srcpath/$1/src -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; 2>/dev/null | sort -f >>$file
    find  $srcpath/$1/include -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; 2>/dev/null | sort -f >>$file 2>/dev/null
    echo -e ')\n' >>$file
    # Print source files
    echo ${src/sample/$name} >>$file
    find  $srcpath/$1/src -maxdepth 1 -iname "*.cpp" -exec echo '    '$srcdir/{} \; 2>/dev/null | sort -f >>$file
    echo -e ')\n' >>$file
}

pushd $basedir
echo -e "# This file was auto-generated with gen_samplelists.sh\n" >$file
for sample in ${samples[@]}; do
    printfiles $sample
done

echo '# Deal with platform specific sources for sample shell' >> $file
echo 'if(WIN32)' >> $file
echo '       list(APPEND shell_SRC_FILES' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/win32/ShellWin32.cpp' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/win32/InputWin32.cpp' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/win32/ShellRenderInterfaceExtensionsOpenGL_Win32.cpp' >> $file
echo '       )' >> $file
echo '       list(APPEND shell_HDR_FILES' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/include/win32/InputWin32.h' >> $file
echo '       )' >> $file
echo 'elseif(APPLE)' >> $file
echo '       list(APPEND shell_SRC_FILES' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/macosx/ShellMacOSX.cpp' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/macosx/InputMacOSX.cpp' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/macosx/ShellRenderInterfaceExtensionsOpenGL_MacOSX.cpp' >> $file
echo '       )' >> $file
echo '       list(APPEND shell_HDR_FILES' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/include/macosx/InputMacOSX.h' >> $file
echo '       )' >> $file
echo 'else()' >> $file
echo '       list(APPEND shell_SRC_FILES' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/x11/ShellX11.cpp' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/x11/InputX11.cpp' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/src/x11/ShellRenderInterfaceExtensionsOpenGL_X11.cpp' >> $file
echo '       )' >> $file
echo '       list(APPEND shell_HDR_FILES' >> $file
echo '               ${PROJECT_SOURCE_DIR}/Samples/shell/include/x11/InputX11.h' >> $file
echo '       )' >> $file
echo 'endif()' >> $file

popd

