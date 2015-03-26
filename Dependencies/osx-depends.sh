#!/usr/bin/env sh

BUILD_FREETYPE2=YES
BUILD_BOOST_PYTHON=YES
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

build_boostpython()
{
	cd "${DEPS_DIR}"
	if [ ! -d "boost" ]; then
		git clone --recursive https://github.com/boostorg/boost.git boost
	fi

	cd boost

	git submodule foreach 'git clean -fdx; git reset --hard'
	git clean -fdx

	./bootstrap.sh
	./b2 architecture=x86 address-model=32_64 --with-python
	##./b2 architecture=x86 address-model=32_64 link=shared --with-python
	##./b2 architecture=x86 address-model=32_64 link=static --with-python


	cp libs/python/include/boost/python.hpp boost/
	cp libs/utility/include/boost/utility/value_init.hpp boost/utility/
	cp libs/lexical_cast/include/boost/lexical_cast.hpp boost/
cp -R libs/lexical_cast/include/boost/lexical_cast boost/
	cp libs/lexical_cast/include/boost/detail/lcast_precision.hpp boost/detail/
#	mkdir -p boost/math
#	cp libs/math/include/boost/math/special_functions/sign.hpp boost/math/special_functions/
#	cp libs/math/include/boost/math/special_functions/math_fwd.hpp boost/math/special_functions/
#	cp libs/math/include/boost/math/special_functions/detail/round_fwd.hpp boost/math/special_functions/detail/
#	cp libs/math/include/boost/math/special_functions/detail/fp_traits.hpp boost/math/special_functions/detail/
	cp -R libs/math/include/boost/math boost/math

#	mkdir -p boost/math/tools
#	cp libs/math/include/boost/math/tools/config.hpp boost/math/tools/
#	cp libs/math/include/boost/math/tools/user.hpp boost/math/tools/
#	cp libs/math/include/boost/math/tools/promotion.hpp boost/math/tools/
	mkdir -p boost/math/policies
	cp libs/math/include/boost/math/policies/policy.hpp boost/math/policies/

	cp libs/predef/include/boost/detail/endian.hpp boost/detail/

	cp libs/math/include/boost/math/special_functions/fpclassify.hpp boost/math/special_functions/

	cp libs/math/include/boost/math/tools/real_cast.hpp boost/math/tools/

	cp libs/lexical_cast/include/boost/detail/basic_pointerbuf.hpp boost/detail/

	cp libs/foreach/include/boost/foreach.hpp boost/

	cp -R boost "${BUILD_OUTPUTDIR}/include/"
	cp -R stage/lib/* "${BUILD_OUTPUTDIR}/lib/"
	cd "${DEPS_DIR}"
}



build_freetype
build_boostpython
