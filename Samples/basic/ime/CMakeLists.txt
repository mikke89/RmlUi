set(SAMPLE_NAME "ime")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

if(RMLUI_IME_SAMPLE_USE_NOTO_FONTS)
	add_executable(${TARGET_NAME} WIN32
		src/main.cpp
	)
else()
	add_executable(${TARGET_NAME} WIN32
		src/SystemFontWin32.cpp
		src/SystemFontWin32.h
		src/main.cpp
	)
endif()

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell)

install_sample_target(${TARGET_NAME})

if(RMLUI_IME_SAMPLE_USE_NOTO_FONTS)
	target_compile_definitions(${TARGET_NAME} PRIVATE "RMLUI_IME_SAMPLE_USE_NOTO_FONTS")
	set(external_assets_dir "${CMAKE_SOURCE_DIR}/Samples/assets/external")
	file(DOWNLOAD "https://github.com/google/fonts/raw/47831f08ec6d6d7ad6b465f23dc9f9a890a2a04b/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf" "${external_assets_dir}/NotoSansSC[wght].ttf"
		EXPECTED_HASH SHA256=a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da
	)
	file(DOWNLOAD "https://github.com/google/fonts/raw/47831f08ec6d6d7ad6b465f23dc9f9a890a2a04b/ofl/notosansjp/NotoSansJP%5Bwght%5D.ttf" "${external_assets_dir}/NotoSansJP[wght].ttf"
		EXPECTED_HASH SHA256=c2f3b4d463500a2ddcd3849cded1fceeb9fd6d1c32e6cbecd568453ba50fc68f
	)
	file(DOWNLOAD "https://github.com/google/fonts/raw/47831f08ec6d6d7ad6b465f23dc9f9a890a2a04b/ofl/notosanskr/NotoSansKR%5Bwght%5D.ttf" "${external_assets_dir}/NotoSansKR[wght].ttf"
		EXPECTED_HASH SHA256=194018e6b2b293a7964f037b25c0249ce1418bc9ab3c971060a03aa57861e252
	)
	file(DOWNLOAD "https://github.com/google/fonts/raw/47831f08ec6d6d7ad6b465f23dc9f9a890a2a04b/ofl/notosansthai/NotoSansThai%5Bwdth,wght%5D.ttf" "${external_assets_dir}/NotoSansThai[wdth,wght].ttf"
		EXPECTED_HASH SHA256=5a1c559bb539583c8a1fd99d1c5b9491e5e14478c9cd2bd0970d5c3096cc9ef8
	)
	file(DOWNLOAD "https://github.com/google/fonts/raw/47831f08ec6d6d7ad6b465f23dc9f9a890a2a04b/ofl/notosansarabic/NotoSansArabic%5Bwdth,wght%5D.ttf" "${external_assets_dir}/NotoSansArabic[wdth,wght].ttf"
		EXPECTED_HASH SHA256=63111b5b2e074dd48cc67692e0a2726d86ee94c1c37fe8598257b7b4e87e869e
	)
endif()
