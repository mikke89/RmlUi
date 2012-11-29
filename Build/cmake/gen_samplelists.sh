#!/bin/bash

basedir="../.."
file=Build/cmake/SampleFileList.cmake
src='set(sample_SRC_FILES'
hdr='set(sample_HDR_FILES'
srcdir='${PROJECT_SOURCE_DIR}'
srcpath=Samples
samples=('basic/customlog' 'basic/directx' 'basic/drag' 'basic/loaddocument'
        'basic/ogre3d' 'basic/treeview' 'invaders' 'pyinvaders' 'shell'
	'tutorial/template' 'tutorial/datagrid' 'tutorial/datagrid_tree' 'tutorial/tutorial_drag'
)

printfiles() {
    # Print headers
    name=${1//basic\//} #substitute basic/ for nothing
    name=${name//tutorial\/} #substitute tutorial/ for nothing
    echo ${hdr/sample/$name} >>$file
    find  $srcpath/$1/src -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file
    find  $srcpath/$1/include -maxdepth 1 -iname "*.h" -exec echo '    '$srcdir/{} \; >>$file 2>/dev/null
    echo -e ')\n' >>$file
    # Print source files
    echo ${src/sample/$name} >>$file
    find  $srcpath/$1/src -maxdepth 1 -iname "*.cpp" -exec echo '    '$srcdir/{} \; >>$file
    echo -e ')\n' >>$file
}

pushd $basedir
echo -e "# This file was auto-generated with gen_samplelists.sh\n" >$file
for sample in ${samples[@]}; do
    printfiles $sample
done
popd

