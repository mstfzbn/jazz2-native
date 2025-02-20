target_compile_features(ncine PUBLIC cxx_std_20)
set_target_properties(ncine PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(ncine PUBLIC "NCINE_VERSION=\"${NCINE_VERSION}\"")
if(NCINE_LINUX_PACKAGE)
	message(STATUS "Using custom Linux package name: ${NCINE_LINUX_PACKAGE}")
	target_compile_definitions(ncine PUBLIC "NCINE_LINUX_PACKAGE=\"${NCINE_LINUX_PACKAGE}\"")
endif()

target_compile_definitions(ncine PUBLIC "CMAKE_BUILD")
target_compile_definitions(ncine PUBLIC "$<$<CONFIG:Debug>:NCINE_DEBUG>")

# Override output executable name and force Unicode mode on Windows
if(WIN32)
	set_target_properties(ncine PROPERTIES WIN32_EXECUTABLE TRUE)
	target_compile_definitions(ncine PRIVATE "_UNICODE" "UNICODE")
	
	if(WINDOWS_PHONE OR WINDOWS_STORE)
		target_compile_definitions(ncine PUBLIC "DEATH_TARGET_WINDOWS_RT")
		
		# TODO: Check this
		target_compile_options(ncine PRIVATE "-await")
		# Workaround for "error C2039: 'wait_for': is not a member of 'winrt::impl'"
		target_compile_options(ncine PRIVATE /Zc:twoPhase-)
		target_link_libraries(ncine PRIVATE "WindowsApp.lib" "rpcrt4.lib" "onecoreuap.lib")
		set_target_properties(ncine PROPERTIES VS_GLOBAL_MinimalCoreWin "true")

		# Include dependencies in UWP package
		set(UWP_DEPENDENCIES
			${MSVC_BINDIR}/libEGL.dll
			${MSVC_BINDIR}/libGLESv2.dll
			# TODO: Include also "zlib1.dll"
		)
		
		if(LIBDEFLATE_FOUND)
			list(APPEND UWP_DEPENDENCIES ${MSVC_BINDIR}/libdeflate.dll)
		elseif(ZLIB_FOUND)
			list(APPEND UWP_DEPENDENCIES ${MSVC_BINDIR}/zlib.dll)
		endif()
		
		if(NCINE_WITH_WEBP AND WEBP_FOUND)
			list(APPEND UWP_DEPENDENCIES ${MSVC_BINDIR}/libwebp.dll)
		endif()
		
		if(NCINE_WITH_AUDIO AND OPENAL_FOUND)
			list(APPEND UWP_DEPENDENCIES ${MSVC_BINDIR}/OpenAL32.dll)
			
			if(NCINE_WITH_VORBIS AND VORBIS_FOUND)
				list(APPEND UWP_DEPENDENCIES ${MSVC_BINDIR}/libogg.dll ${MSVC_BINDIR}/libvorbis.dll ${MSVC_BINDIR}/libvorbisfile.dll)
			endif()
		endif()
		
		target_sources(ncine PRIVATE ${UWP_DEPENDENCIES})
		set_property(SOURCE ${UWP_DEPENDENCIES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
		set_property(SOURCE ${UWP_DEPENDENCIES} PROPERTY VS_DEPLOYMENT_LOCATION ".")
		source_group("Dependencies" FILES ${UWP_DEPENDENCIES})
	else()
		set_target_properties(ncine PROPERTIES OUTPUT_NAME "Jazz2")
	endif()
else()
	set_target_properties(ncine PROPERTIES OUTPUT_NAME "jazz2")
endif()

if(EMSCRIPTEN)
	set(EMSCRIPTEN_LINKER_OPTIONS
		"SHELL:-s WASM=1"
		"SHELL:-s ASYNCIFY=1"
		"SHELL:-s DISABLE_EXCEPTION_CATCHING=1"
		"SHELL:-s FORCE_FILESYSTEM=1"
		"SHELL:-s ALLOW_MEMORY_GROWTH=1"
		"SHELL:--bind")

	set(EMSCRIPTEN_LINKER_OPTIONS_DEBUG
		"SHELL:-s ASSERTIONS=1"
		#"SHELL:-s SAFE_HEAP=1"
		"SHELL:-s SAFE_HEAP_LOG=1"
		"SHELL:-s STACK_OVERFLOW_CHECK=2"
		"SHELL:-s GL_ASSERTIONS=1"
		"SHELL:-s DEMANGLE_SUPPORT=1"
		"SHELL:--profiling-funcs")

	string(FIND ${CMAKE_CXX_COMPILER} "fastcomp" EMSCRIPTEN_FASTCOMP_POS)
	if(EMSCRIPTEN_FASTCOMP_POS GREATER -1)
		list(APPEND EMSCRIPTEN_LINKER_OPTIONS "SHELL:-s BINARYEN_TRAP_MODE=clamp")
	else()
		list(APPEND EMSCRIPTEN_LINKER_OPTIONS "SHELL:-mnontrapping-fptoint")
	endif()
	
	# Include all files in specified directory
	list(APPEND EMSCRIPTEN_LINKER_OPTIONS "SHELL:--preload-file ${NCINE_DATA_DIR}@Content/")

	target_link_options(ncine PUBLIC ${EMSCRIPTEN_LINKER_OPTIONS})
	target_link_options(ncine PUBLIC "$<$<CONFIG:Debug>:${EMSCRIPTEN_LINKER_OPTIONS_DEBUG}>")

	if(Threads_FOUND)
		target_link_libraries(ncine PUBLIC Threads::Threads)
	endif()

	if(OPENGL_FOUND)
		target_link_libraries(ncine PUBLIC OpenGL::GL)
	endif()

	if(GLFW_FOUND)
		target_link_libraries(ncine PUBLIC GLFW::GLFW)
	endif()

	if(SDL2_FOUND)
		target_link_libraries(ncine PUBLIC SDL2::SDL2)
	endif()

	if(PNG_FOUND)
		target_link_libraries(ncine PUBLIC PNG::PNG)
	endif()

	#if(ZLIB_FOUND)
	#	target_link_libraries(ncine PUBLIC ZLIB::ZLIB)
	#endif()

	if(VORBIS_FOUND)
		target_link_libraries(ncine PUBLIC Vorbis::Vorbisfile)
	endif()

	#if(OPENMPT_FOUND)
	#	target_link_libraries(ncine PUBLIC libopenmpt::libopenmpt)
	#endif()
	
	target_link_libraries(ncine PUBLIC idbfs.js)
	target_link_libraries(ncine PUBLIC websocket.js)
endif()

if(MSVC)
	# Build with Multiple Processes
	target_compile_options(ncine PRIVATE /MP)
	# Always use the non debug version of the runtime library
	target_compile_options(ncine PUBLIC /MD)
	# Disabling exceptions
	target_compile_definitions(ncine PRIVATE "_HAS_EXCEPTIONS=0")
	target_compile_options(ncine PRIVATE /EHsc)
	# Extra optimizations in release
	target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:/fp:fast /O2 /Qpar>)

	# Enabling Whole Program Optimization
	if(NCINE_LINKTIME_OPTIMIZATION)
		target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:/GL>)
		target_link_options(ncine PRIVATE $<$<CONFIG:Release>:/LTCG>)
	endif()

	if(NCINE_AUTOVECTORIZATION_REPORT)
		target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:/Qvec-report:2 /Qpar-report:2>)
	endif()

	# Suppress linker warning about templates
	target_compile_options(ncine PUBLIC "/wd4251")

	# Disabling incremental linking and manifest generation
	target_link_options(ncine PRIVATE $<$<CONFIG:Debug>:/MANIFEST:NO /INCREMENTAL:NO>)
	target_link_options(ncine PRIVATE $<$<CONFIG:RelWithDebInfo>:/MANIFEST:NO /INCREMENTAL:NO>)

	if(NCINE_WITH_TRACY)
		target_link_options(ncine PRIVATE $<$<CONFIG:Release>:/DEBUG>)
	endif()
else() # GCC and LLVM
	target_compile_options(ncine PRIVATE -fno-exceptions)
	target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-ffast-math>)

	if(NCINE_DYNAMIC_LIBRARY)
		target_compile_options(ncine PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
	endif()

	# Only in debug - preserve debug information
	if(EMSCRIPTEN)
		target_compile_options(ncine PUBLIC $<$<CONFIG:Debug>:-g>)
		target_link_options(ncine PUBLIC $<$<CONFIG:Debug>:-g>)
	endif()

	# Only in debug
	if(NCINE_ADDRESS_SANITIZER)
		# Add ASan options as public so that targets linking the library will also use them
		if(EMSCRIPTEN)
			target_compile_options(ncine PUBLIC $<$<CONFIG:Debug>:-O1 -g -fsanitize=address>) # Needs "ALLOW_MEMORY_GROWTH" which is already passed to the linker
			target_link_options(ncine PUBLIC $<$<CONFIG:Debug>:-fsanitize=address>)
		elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
			target_compile_options(ncine PUBLIC $<$<CONFIG:Debug>:-O1 -g -fsanitize=address -fsanitize-address-use-after-scope -fno-optimize-sibling-calls -fno-common -fno-omit-frame-pointer -rdynamic>)
			target_link_options(ncine PUBLIC $<$<CONFIG:Debug>:-fsanitize=address>)
		endif()
	endif()

	# Only in debug
	if(NCINE_UNDEFINED_SANITIZER)
		# Add UBSan options as public so that targets linking the library will also use them
		if(EMSCRIPTEN)
			target_compile_options(ncine PUBLIC $<$<CONFIG:Debug>:-O1 -g -fsanitize=undefined>)
			target_link_options(ncine PUBLIC $<$<CONFIG:Debug>:-fsanitize=undefined>)
		elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
			target_compile_options(ncine PUBLIC $<$<CONFIG:Debug>:-O1 -g -fsanitize=undefined -fno-omit-frame-pointer>)
			target_link_options(ncine PUBLIC $<$<CONFIG:Debug>:-fsanitize=undefined>)
		endif()
	endif()

	# Only in debug
	if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang") AND NCINE_CODE_COVERAGE)
		# Add code coverage options as public so that targets linking the library will also use them
		target_compile_options(ncine PUBLIC $<$<CONFIG:Debug>:--coverage>)
		target_link_options(ncine PUBLIC $<$<CONFIG:Debug>:--coverage>)
	endif()

	if(NCINE_WITH_TRACY)
		target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-g -fno-omit-frame-pointer -rdynamic>)
		if(MINGW OR MSYS)
			target_link_libraries(ncine PRIVATE ws2_32 dbghelp)
		elseif(NOT ANDROID AND NOT APPLE)
			target_link_libraries(ncine PRIVATE dl)
		endif()
	endif()

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		target_compile_options(ncine PRIVATE -fdiagnostics-color=auto)
		target_compile_options(ncine PRIVATE -Wall -pedantic -Wextra -Wold-style-cast -Wno-long-long -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-variadic-macros -Wcast-align)

		if(NCINE_DYNAMIC_LIBRARY)
			target_link_options(ncine PRIVATE -Wl,--no-undefined)
		endif()

		target_compile_options(ncine PRIVATE $<$<CONFIG:Debug>:-fvar-tracking-assignments>)

		# Extra optimizations in release
		target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-Ofast -funsafe-loop-optimizations -ftree-loop-if-convert-stores>)

		if(NCINE_LINKTIME_OPTIMIZATION AND NOT (MINGW OR MSYS OR ANDROID))
			target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-flto=auto>)
			target_link_options(ncine PRIVATE $<$<CONFIG:Release>:-flto=auto>)
		endif()

		if(NCINE_AUTOVECTORIZATION_REPORT)
			target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-fopt-info-vec-optimized>)
		endif()

		# Enabling strong stack protector of GCC 4.9
		if(NCINE_GCC_HARDENING AND NOT (MINGW OR MSYS))
			target_compile_options(ncine PUBLIC $<$<CONFIG:Release>:-Wformat -Wformat-security -fstack-protector-strong -fPIE -fPIC>)
			target_compile_definitions(ncine PUBLIC "$<$<CONFIG:Release>:_FORTIFY_SOURCE=2>")
			target_link_options(ncine PUBLIC $<$<CONFIG:Release>:-Wl,-z,relro -Wl,-z,now -pie>)
		endif()
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
		target_compile_options(ncine PRIVATE -fcolor-diagnostics)
		target_compile_options(ncine PRIVATE -Wall -pedantic -Wextra -Wold-style-cast -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -Wno-variadic-macros -Wno-c++11-long-long -Wno-missing-braces)

		if(NCINE_DYNAMIC_LIBRARY)
			target_link_options(ncine PRIVATE -Wl,-undefined,error)
		endif()

		# Extra optimizations in release
		if(NOT EMSCRIPTEN)
			target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-Ofast>)
		endif()

		# Enabling ThinLTO of Clang 4
		if(NCINE_LINKTIME_OPTIMIZATION)
			if(EMSCRIPTEN)
				target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-flto>)
				target_link_options(ncine PRIVATE $<$<CONFIG:Release>:-flto>)
			elseif(NOT (MINGW OR MSYS OR ANDROID))
				target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-flto=thin>)
				target_link_options(ncine PRIVATE $<$<CONFIG:Release>:-flto=thin>)
			endif()
		endif()

		if(NCINE_AUTOVECTORIZATION_REPORT)
			target_compile_options(ncine PRIVATE $<$<CONFIG:Release>:-Rpass=loop-vectorize -Rpass-analysis=loop-vectorize>)
		endif()
	endif()
endif()
