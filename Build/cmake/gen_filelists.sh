#!/bin/bash

basedir="../.."
file=Build/cmake/FileList.cmake
src='set(lib_SRC_FILES'
hdr='set(lib_HDR_FILES'
pubhdr='set(lib_PUB_HDR_FILES'
srcdir='${PROJECT_SOURCE_DIR}'

printfiles() {
    echo ${hdr/lib/$1} >>$file
    find  Source/$1/ -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
    echo ${pubhdr/lib/$1} >>$file
    find  Include/Rocket/$1/ -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file
    echo '    '$srcdir/Include/Rocket/$1.h >>$file
    echo -e ')\n' >>$file
    echo ${src/lib/$1} >>$file
    find  Source/$1/ -maxdepth 1 -iname "*.cpp" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
}

pushd $basedir
echo -e "# This file was auto-generated with gen_filelists.sh\n" >$file
for lib in "Core" "Controls" "Debugger"; do
    printfiles $lib
done
popd

