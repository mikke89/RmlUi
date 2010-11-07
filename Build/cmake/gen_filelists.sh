#!/bin/bash

basedir="../.."
file=Build/cmake/FileList.cmake
src='set(lib_SRC_FILES'
hdr='set(lib_HDR_FILES'
pubhdr='set(lib_PUB_HDR_FILES'
srcdir='${PROJECT_SOURCE_DIR}'
srcpath=Source
hdrpath=Include/Rocket
pypath=Python

printfiles() {
    # Print headers
    echo ${hdr/lib/$1} >>$file
    find  $srcpath/$1/ -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
    # Print public headers
    echo ${pubhdr/lib/$1} >>$file
    find  $hdrpath/$1/ -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file
    # Print main public header
    echo '    '$srcdir/Include/Rocket/$1.h >>$file
    echo -e ')\n' >>$file
    # Print source files
    echo ${src/lib/$1} >>$file
    find  $srcpath/$1/ -maxdepth 1 -iname "*.cpp" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
}

printpyfiles() {
    # Print headers
    echo ${hdr/lib/Py${1,}} >>$file
    find  $srcpath/$1/$pypath -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
    # Print public headers
    echo ${pubhdr/lib/Py${1,}} >>$file
    find  $hdrpath/$1/$pypath -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file 2>/dev/null
    echo -e ')\n' >>$file
    # Print source files
    echo ${src/lib/Py${1,}} >>$file
    find  $srcpath/$1/$pypath -iname "*.cpp" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
}

pushd $basedir
echo -e "# This file was auto-generated with gen_filelists.sh\n" >$file
for lib in "Core" "Controls" "Debugger"; do
    printfiles $lib
done

for lib in "Core" "Controls"; do
    printpyfiles $lib
done
popd

