cmake_minimum_required(VERSION 3.24)
project(LiteViewWindowsPort LANGUAGES C CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This port targets native Windows x64.")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "Use clang-cl with the Visual Studio x64 environment.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(LITEVIEW_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../liteview" ABSOLUTE)

# Use the upstream's explicit source list, excluding its Linux platform files
# and platform-specific V8 blobs. Do not glob the tree: it includes alternatives
# and tests that are not part of livicore.
file(STRINGS "${LITEVIEW_SOURCE}/BUILD.gn" native_source_lines
    REGEX "^[ ]*\"//(api|content|mbnet)/.*\\.(cpp|cc)\"")
set(native_sources)
foreach(line IN LISTS native_source_lines)
    string(REGEX REPLACE "^[ ]*\"//([^\"]+)\".*" "\\1" source "${line}")
    if(source MATCHES "^content/resources/" OR source MATCHES "Linux\\.cpp$" OR source STREQUAL "content/browser/DomStorageProviderImpl.cpp")
        continue()
    endif()
    list(APPEND native_sources "${LITEVIEW_SOURCE}/${source}")
endforeach()
list(REMOVE_DUPLICATES native_sources)
# This target is deliberately an object library: a successful compile is not
# yet a linked or runnable browser engine.
add_library(livicore_port_check OBJECT ${native_sources}
    ToolTip.cpp
    qlitebridge.cpp
    DomStorageProviderImpl.cpp
    WindowsTls.cpp
    "${LITEVIEW_SOURCE}/content/browser/SharedTimerWin.cpp"
    IcuData.cpp
    "${LITEVIEW_SOURCE}/content/resources/V8108SnapshotBlobBinX64.cpp"
    "${LITEVIEW_SOURCE}/content/resources/V8108EmbeddedX64Win.cpp"
)
target_include_directories(livicore_port_check PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${LITEVIEW_SOURCE}"
    "${LITEVIEW_SOURCE}/gen"
    "${LITEVIEW_SOURCE}/gen/v8x64win"
    "${LITEVIEW_SOURCE}/gen/v8x64win/include"
    "${LITEVIEW_SOURCE}/gen/v8/include"
    "${LITEVIEW_SOURCE}/v8"
    "${LITEVIEW_SOURCE}/v8/include"
    "${LITEVIEW_SOURCE}/third_party/abseil-cpp"
    "${LITEVIEW_SOURCE}/third_party/skia"
    "${LITEVIEW_SOURCE}/third_party/icu/source/common"
    "${LITEVIEW_SOURCE}/third_party/icu/source/i18n"
    "${LITEVIEW_SOURCE}/third_party/angle/include"
    "${LITEVIEW_SOURCE}/third_party/khronos"
    "${LITEVIEW_SOURCE}/third_party/khronos/noninclude"
    "${LITEVIEW_SOURCE}/third_party/zlib"
    "${LITEVIEW_SOURCE}/third_party/libyuv/include"
    "${LITEVIEW_SOURCE}/third_party/libxml/src/include"
    "${LITEVIEW_SOURCE}/third_party/libxml/win32/include"
    "${LITEVIEW_SOURCE}/third_party/libxslt/src"
    "${LITEVIEW_SOURCE}/third_party/openssl/openssl/include"
    "${LITEVIEW_SOURCE}/third_party/openssl"
    "${LITEVIEW_SOURCE}/third_party/libcurl/include"
    "${LITEVIEW_SOURCE}/third_party/libcurl/src"
)
target_compile_options(livicore_port_check PRIVATE
    /utf-8 /bigobj /W0 /GR- /clang:-fno-exceptions
)
target_compile_definitions(livicore_port_check PRIVATE
    WIN32 OS_WIN=1 UNICODE _UNICODE _WIN32_WINNT=0x0A00 WINVER=0x0A00 NOMINMAX WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS _HAS_EXCEPTIONS=0
    ENABLE_MB=1 USE_AURA INSIDE_BLINK BLINK_IMPLEMENTATION
    DYNAMIC_ANNOTATIONS_ENABLED=0 U_STATIC_IMPLEMENTATION U_DEFINE_FALSE_AND_TRUE
    LIBXML_STATIC LIBXSLT_STATIC CURL_STATICLIB
    SK_SUPPORT_GPU=1 SK_GL USE_EGL SK_SUPPORT_LEGACY_DRAWLOOPER
    SK_SUPPORT_LEGACY_CREATESHADER_PTR=1 SK_SUPPORT_LEGACY_TYPEFACE_PTR=1
    SK_SUPPORT_LEGACY_SLUG_CONVERT SK_B32_SHIFT=0 SK_G32_SHIFT=8 SK_R32_SHIFT=16 SK_A32_SHIFT=24
    V8_OS_WIN V8_TARGET_OS_WIN V8_HAVE_TARGET_OS V8_HOST_ARCH_X64 V8_TARGET_ARCH_X64
    V8_TYPED_ARRAY_MAX_SIZE_IN_HEAP=64 V8_INTL_SUPPORT V8_USE_EXTERNAL_STARTUP_DATA
    V8_ATOMIC_OBJECT_FIELD_WRITES V8_ENABLE_LAZY_SOURCE_POSITIONS V8_SHARED_RO_HEAP
    V8_ENABLE_REGEXP_INTERPRETER_THREADED_DISPATCH V8_SHORT_BUILTIN_CALLS
    V8_ENABLE_SYSTEM_INSTRUMENTATION V8_ENABLE_WEBASSEMBLY V8_ALLOCATION_FOLDING
    V8_ALLOCATION_SITE_TRACKING V8_ADVANCED_BIGINT_ALGORITHMS V8_USE_ZLIB
    V8_ENABLE_MAGLEV V8_EXTERNAL_CODE_SPACE V8_DEPRECATION_WARNINGS
    V8_COMPRESS_POINTERS V8_COMPRESS_POINTERS_IN_SHARED_CAGE V8_31BIT_SMIS_ON_64BIT_ARCH
    CPPGC_CAGED_HEAP CPPGC_YOUNG_GENERATION CPPGC_POINTER_COMPRESSION
)

option(LITEVIEW_BUILD_V8_PORT "Build the Windows V8 static library" OFF)
if(LITEVIEW_BUILD_V8_PORT)
    set(v8_sources)
    foreach(domain Console Debugger HeapProfiler Profiler Protocol Runtime Schema)
        list(APPEND v8_sources "${LITEVIEW_SOURCE}/gen/v8x64win/src/inspector/protocol/${domain}.cpp")
    endforeach()
    foreach(gn_file v8/BUILD.gn gen/v8x64linux/BUILD.gn)
        get_filename_component(gn_directory "${gn_file}" DIRECTORY)
        file(STRINGS "${LITEVIEW_SOURCE}/${gn_file}" source_lines
            REGEX "^[ ]*\"[^\"]+\\.cc\"")
        foreach(line IN LISTS source_lines)
            string(REGEX REPLACE "^[ ]*\"([^\"]+)\".*" "\\1" source "${line}")
            string(REPLACE "v8x64linux" "v8x64win" source "${source}")
            if(source MATCHES "^//")
                string(SUBSTRING "${source}" 2 -1 source)
            else()
                set(source "${gn_directory}/${source}")
            endif()
            if(source MATCHES "platform-(posix|linux)(-time)?\\.cc$")
                continue()
            endif()
            string(REPLACE "stack_trace_posix.cc" "stack_trace_win.cc" source "${source}")
            string(REPLACE "handler-inside-posix.cc" "handler-inside-win.cc" source "${source}")
            string(REPLACE "handler-outside-posix.cc" "handler-outside-win.cc" source "${source}")
            list(APPEND v8_sources "${LITEVIEW_SOURCE}/${source}")
        endforeach()
    endforeach()
    list(APPEND v8_sources "${LITEVIEW_SOURCE}/v8/src/base/platform/platform-win32.cc")
    list(APPEND v8_sources "${LITEVIEW_SOURCE}/v8/src/diagnostics/unwinding-info-win64.cc")
    list(REMOVE_DUPLICATES v8_sources)
    # clang-cl defaults to Intel assembly, whereas this upstream TU is AT&T.
    set_source_files_properties(
        "${LITEVIEW_SOURCE}/v8/src/heap/base/asm/x64/push_registers_asm_x64.cc"
        PROPERTIES COMPILE_OPTIONS "/clang:-masm=att")
    add_library(livi_v8 STATIC ${v8_sources})
    foreach(property COMPILE_OPTIONS COMPILE_DEFINITIONS)
        get_target_property(value livicore_port_check ${property})
        set_property(TARGET livi_v8 PROPERTY ${property} "${value}")
    endforeach()
    target_include_directories(livi_v8 PRIVATE
        "${LITEVIEW_SOURCE}/v8"
        "${LITEVIEW_SOURCE}/v8/include"
        "${LITEVIEW_SOURCE}/gen/v8x64win"
        "${LITEVIEW_SOURCE}/gen/v8x64win/include"
        "${LITEVIEW_SOURCE}/third_party/icu/source/common"
        "${LITEVIEW_SOURCE}/third_party/icu/source/i18n"
        "${LITEVIEW_SOURCE}"
        "${LITEVIEW_SOURCE}/gen"
    )
    target_compile_options(livi_v8 PRIVATE /GR)
    target_compile_definitions(livi_v8 PRIVATE
        ENABLE_DISASSEMBLER OBJECT_PRINT ENABLE_HANDLE_ZAPPING
        ICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_SHARED UCONFIG_NO_SERVICE=1
        U_ENABLE_DYLOAD=0 U_HAVE_STD_STRING=1 UCONFIG_NO_BREAK_ITERATION=0
    )
endif()

option(LITEVIEW_BUILD_BASE_PORT "Build the Windows base/compositor static library" OFF)
option(LITEVIEW_BUILD_ICU_PORT "Build the Windows ICU static library" OFF)
option(LITEVIEW_BUILD_GRAPHICS_PORT "Build the Windows graphics static libraries" OFF)
option(LITEVIEW_BUILD_BLINK_PORT "Build the Windows Blink static libraries" OFF)
option(LITEVIEW_BUILD_UTIL_PORT "Build the Windows codec/network static library" OFF)
option(LITEVIEW_BUILD_SSL_PORT "Build the Windows OpenSSL static library" OFF)
option(LITEVIEW_BUILD_GL_PORT "Build the Windows GL static libraries" OFF)
option(LITEVIEW_BUILD_MEDIA_BASE_PORT "Build media data types used by Blink/Skia" OFF)
include("${CMAKE_CURRENT_LIST_DIR}/dependencies.cmake")
option(LITEVIEW_LINK_ENGINE "Link the Windows engine from built components" OFF)
if(LITEVIEW_LINK_ENGINE)
    include("${CMAKE_CURRENT_LIST_DIR}/engine.cmake")
endif()

option(LITEVIEW_BUILD_V8_SMOKE "Link a real V8 initialization/execution check" OFF)
if(LITEVIEW_BUILD_V8_SMOKE)
    if(NOT TARGET livi_v8)
        message(FATAL_ERROR "The V8 smoke check requires LITEVIEW_BUILD_V8_PORT.")
    endif()
    add_executable(livi_v8_smoke v8-smoke.cpp
        "${LITEVIEW_SOURCE}/patch_code/atomic_xp.cpp"
        "${LITEVIEW_SOURCE}/content/resources/icudtl.cpp"
        "${LITEVIEW_SOURCE}/content/resources/V8108SnapshotBlobBinX64.cpp"
        "${LITEVIEW_SOURCE}/content/resources/V8108EmbeddedX64Win.cpp")
    foreach(property INCLUDE_DIRECTORIES COMPILE_OPTIONS COMPILE_DEFINITIONS)
        get_target_property(value livi_v8 ${property})
        set_property(TARGET livi_v8_smoke PROPERTY ${property} "${value}")
    endforeach()
    target_link_libraries(livi_v8_smoke PRIVATE livi_v8
        "${CMAKE_CURRENT_LIST_DIR}/../../build/windows-icu/livi_icu.lib"
        "${CMAKE_CURRENT_LIST_DIR}/../../build/windows-util/livi_util.lib"
        winmm dbghelp shlwapi advapi32)
endif()
