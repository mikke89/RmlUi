#!/usr/bin/env bash

basedir=".."
file=CMake/SampleFileList.cmake
src='set(sample_SRC_FILES'
hdr='set(sample_HDR_FILES'
srcdir='${PROJECT_SOURCE_DIR}'
srcpath=Samples
samples=( 'shell'
	'basic/animation' 'basic/benchmark' 'basic/bitmapfont' 'basic/customlog' 'basic/databinding' 'basic/demo' 'basic/drag' 'basic/loaddocument' 'basic/treeview' 'basic/transform'
	'basic/harfbuzzshaping' 'basic/lottie' 'basic/svg'
	'tutorial/template' 'tutorial/drag'
	'invaders' 'luainvaders'
)

printfiles() {
    # Print headers
    name=${1//basic\//} #substitute basic/ for nothing
    name=${name//tutorial\//tutorial_} #substitute 'tutorial/' for 'tutorial_'
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
