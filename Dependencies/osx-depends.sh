#!/usr/bin/env sh

BUILD_FREETYPE2=YES
BUILD_LUA=YES
BUILD_PLATFORM=osx


#Get depends root directory

pushd `dirname $0` > /dev/null
SCRIPT_PATH=`pwd`
popd > /dev/null

DEPS_DIR=${SCRIPT_PATH}

BUILD_OUTPUTDIR=${DEPS_DIR}/${BUILD_PLATFORM}

if [ ! -d "${BUILD_OUTPUTDIR}" ]; then
	mkdir "${BUILD_OUTPUTDIR}"
fi
if [ ! -d "${BUILD_OUTPUTDIR}/lib" ]; then
	mkdir "${BUILD_OUTPUTDIR}/lib"
fi
if [ ! -d "${BUILD_OUTPUTDIR}/include" ]; then
	mkdir "${BUILD_OUTPUTDIR}/include"
fi

create_ios_outdir_lipo()
{
        for lib_i386 in `find $LOCAL_OUTDIR/i386 -name "lib*.a"`; do
                lib_arm7=`echo $lib_i386 | sed "s/i386/arm7/g"`
                lib_arm7s=`echo $lib_i386 | sed "s/i386/arm7s/g"`
                lib=`echo $lib_i386 | sed "s/i386//g"`
                xcrun -sdk iphoneos lipo -arch armv7s $lib_arm7s -arch armv7 $lib_arm7 -create -output $lib
        done
}

build_freetype()
{

	cd "${DEPS_DIR}"
	if [ ! -d "freetype2" ]; then
		git clone --recursive git://git.sv.nongnu.org/freetype/freetype2.git freetype2
	fi

	cd freetype2

	cmake CMakeLists.txt -DBUILD_SHARED_LIBS:BOOL=false
	make

	cmake CMakeLists.txt -DBUILD_SHARED_LIBS:BOOL=true
	make

	if [ ! -d "${BUILD_OUTPUTDIR}/include/freetype2" ]; then
		mkdir "${BUILD_OUTPUTDIR}/include/freetype2"
	fi
	cp -Rp include/* "${BUILD_OUTPUTDIR}/include/freetype2/"
	cp libfreetype.a "${BUILD_OUTPUTDIR}/lib/"
	cp libfreetype.dylib "${BUILD_OUTPUTDIR}/lib/"

	cd "${DEPS_DIR}"
}


build_freetype
