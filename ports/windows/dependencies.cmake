# The supplied GN targets contain literal source lists. Keep their common
# sources and replace the platform translation units explicitly for Windows.
function(livi_read_sources output gn_file)
    file(STRINGS "${LITEVIEW_SOURCE}/${gn_file}" lines
        REGEX "^[ ]*\"[^\"]+\\.(cc|cpp|c)\"")
    get_filename_component(directory "${gn_file}" DIRECTORY)
    set(result)
    foreach(line IN LISTS lines)
        if(line MATCHES "^[ ]*\"([^\"]+\\.(cc|cpp|c))\"")
            set(source "${CMAKE_MATCH_1}")
            if(source MATCHES "^//")
                string(SUBSTRING "${source}" 2 -1 source)
            elseif(NOT directory STREQUAL "")
                set(source "${directory}/${source}")
            endif()
            list(APPEND result "${source}")
        endif()
    endforeach()
    if(NOT result)
        message(FATAL_ERROR "No literal sources found in ${gn_file}")
    endif()
    set(${output} "${result}" PARENT_SCOPE)
endfunction()

function(livi_copy_compile_config target)
    foreach(property INCLUDE_DIRECTORIES COMPILE_OPTIONS COMPILE_DEFINITIONS)
        get_target_property(value livicore_port_check ${property})
        set_property(TARGET ${target} PROPERTY ${property} "${value}")
    endforeach()
endfunction()

function(livi_gn_compile_config target gn_file)
    livi_copy_compile_config(${target})
    get_filename_component(directory "${gn_file}" DIRECTORY)
    file(STRINGS "${LITEVIEW_SOURCE}/${gn_file}" lines REGEX "^[ ]*\"[^\"]+\"")
    foreach(line IN LISTS lines)
        string(REGEX REPLACE "^[ ]*\"([^\"]+)\".*" "\\1" value "${line}")
        if(value MATCHES "^-D(.+)")
            set(definition "${CMAKE_MATCH_1}")
            if(definition MATCHES "^(_GNU_SOURCE|_GLIBCXX|OS_LINUX|USE_OZONE|USE_GLIB|USE_ALSA|V8_|CPPGC_|BUILDING_V8|BUILDING_UV|_HAS_CONSTEXPR|_M_X64|__POINT_SIZE__|__USE_OPENGL__|ENABLE_MB_MEDIA|SK_GRAPHITE_ENABLED|SK_[RGBA]32_SHIFT)")
                continue()
            endif()
            target_compile_definitions(${target} PRIVATE "${definition}")
        elseif(NOT value MATCHES "[$]" AND NOT value MATCHES "^-")
            if(value MATCHES "^//")
                string(SUBSTRING "${value}" 2 -1 value)
            else()
                set(value "${directory}/${value}")
            endif()
            string(REPLACE "v8x64linux" "v8x64win" value "${value}")
            if(value MATCHES "(^linux$|/fontconfig(/|$)|/libxml/linux(/|$))")
                continue()
            endif()
            if(IS_DIRECTORY "${LITEVIEW_SOURCE}/${value}")
                target_include_directories(${target} PRIVATE "${LITEVIEW_SOURCE}/${value}")
            endif()
        endif()
    endforeach()
endfunction()

if(LITEVIEW_BUILD_UTIL_PORT)
    livi_read_sources(util_sources third_party/build_util/BUILD.gn)
    list(FILTER util_sources EXCLUDE REGEX "^third_party/fontconfig/")
    list(TRANSFORM util_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_util STATIC ${util_sources})
    livi_gn_compile_config(livi_util third_party/build_util/BUILD.gn)
    get_target_property(util_includes livi_util INCLUDE_DIRECTORIES)
    list(FILTER util_includes EXCLUDE REGEX "/(v8|gen/v8[^/]*)(/|$)")
    set_property(TARGET livi_util PROPERTY INCLUDE_DIRECTORIES "${util_includes}")
    set_source_files_properties("${LITEVIEW_SOURCE}/third_party/zlib/cpu_features.c"
        PROPERTIES COMPILE_DEFINITIONS X86_WINDOWS)
    target_include_directories(livi_util PRIVATE
        "${LITEVIEW_SOURCE}/third_party/libxml"
        "${LITEVIEW_SOURCE}/third_party/dav1d/libdav1d")
endif()

if(LITEVIEW_BUILD_GL_PORT)
    livi_read_sources(angle_sources third_party/angle/BUILD.gn)
    list(FILTER angle_sources EXCLUDE REGEX "system_utils_(linux|posix)\\.cpp$")
    list(FILTER angle_sources EXCLUDE REGEX "TranslatorMetalDirect/")
    list(APPEND angle_sources third_party/angle/src/common/system_utils_win.cpp
        third_party/angle/src/common/system_utils_win32.cpp)
    list(TRANSFORM angle_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_angle STATIC ${angle_sources})
    livi_gn_compile_config(livi_angle third_party/angle/BUILD.gn)
    livi_read_sources(gl_sources ui/gl/BUILD.gn)
    list(FILTER gl_sources EXCLUDE REGEX "^(ui/ozone|gen/ui/ozone)/|_ozone\\.cc$")
    list(APPEND gl_sources
        ui/gl/init/gl_factory_win.cc ui/gl/init/gl_initializer_win.cc
        ui/gl/gl_angle_util_win.cc ui/gl/gl_fence_win.cc
        ui/gl/child_window_win.cc ui/gl/vsync_thread_win.cc
        ui/gl/vsync_provider_win.cc ui/gl/hdr_metadata_helper_win.cc
        ui/gl/direct_composition_support.cc ui/gl/direct_composition_surface_win.cc
        ui/gl/direct_composition_child_surface_win.cc ui/gl/dcomp_surface_registry.cc)
    list(TRANSFORM gl_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_gl STATIC ${gl_sources})
    livi_gn_compile_config(livi_gl ui/gl/BUILD.gn)
endif()

if(LITEVIEW_BUILD_MEDIA_BASE_PORT)
    livi_read_sources(media_sources media/BUILD.gn)
    # Playback is disabled in the reference qwebkit product. Blink still needs
    # media data structures (frames, color formats, MIME/codecs and timestamps).
    list(FILTER media_sources INCLUDE REGEX "^media/(base|formats|parsers)/")
    list(FILTER media_sources EXCLUDE REGEX "user_input_monitor_linux\\.cc$")
    list(APPEND media_sources
        media/renderers/video_resource_updater.cc
        media/renderers/paint_canvas_video_renderer.cc
        media/renderers/video_frame_yuv_mailboxes_holder.cc
        media/renderers/video_frame_yuv_converter.cc
        media/filters/stream_parser_factory.cc
        media/video/h264_parser.cc
        media/video/h264_bit_reader.cc
        media/video/h265_nalu_parser.cc
        media/video/h265_parser.cc
        media/video/half_float_maker.cc)
    list(TRANSFORM media_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_media_base STATIC ${media_sources})
    livi_gn_compile_config(livi_media_base media/BUILD.gn)
endif()

if(LITEVIEW_BUILD_SSL_PORT)
    livi_read_sources(ssl_sources third_party/openssl/openssl/BUILD.gn)
    # Embed OpenSSL's default provider, not a separately signed FIPS module
    # with its own DllMain and module-integrity initialization.
    list(FILTER ssl_sources EXCLUDE REGEX "/providers/fips/")
    # des_enc.c includes this implementation directly in a no-assembly build.
    list(FILTER ssl_sources EXCLUDE REGEX "/crypto/des/ncbc_enc.c$")
    list(TRANSFORM ssl_sources REPLACE "securitycheck_fips.c$" "securitycheck_default.c")
    list(APPEND ssl_sources third_party/openssl/openssl/providers/prov_running.c)
    list(TRANSFORM ssl_sources REPLACE "async_posix.c$" "async_win.c")
    list(TRANSFORM ssl_sources REPLACE "rand_unix.c$" "rand_win.c")
    list(TRANSFORM ssl_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_ssl STATIC ${ssl_sources})
    livi_gn_compile_config(livi_ssl third_party/openssl/openssl/BUILD.gn)
    get_target_property(ssl_includes livi_ssl INCLUDE_DIRECTORIES)
    list(TRANSFORM ssl_includes REPLACE "VC-WIN32" "VC-WIN64A")
    set_property(TARGET livi_ssl PROPERTY INCLUDE_DIRECTORIES "${ssl_includes}")
endif()

if(LITEVIEW_BUILD_GRAPHICS_PORT)
    livi_read_sources(skia_sources third_party/skia/BUILD.gn)
    list(FILTER skia_sources EXCLUDE REGEX "(SkFontMgr_fontconfig|SkFontConfigInterface_direct(_factory)?|SkOSFile_posix|SkDebug_stdio)\\.cpp$")
    list(FILTER skia_sources EXCLUDE REGEX "/graphite/|[Gg]raphite[^/]*\\.cpp$")
    list(TRANSFORM skia_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_skia STATIC ${skia_sources})
    livi_gn_compile_config(livi_skia third_party/skia/BUILD.gn)
    # Link the bundled FreeType 2.9.1 directly; Windows has no dlopen.
    set_source_files_properties(
        "${LITEVIEW_SOURCE}/third_party/skia/src/ports/SkFontHost_FreeType.cpp"
        PROPERTIES COMPILE_DEFINITIONS "SK_FREETYPE_MINIMUM_RUNTIME_VERSION=0x02090100")

    livi_read_sources(freetype_sources third_party/freetype/BUILD.gn)
    list(TRANSFORM freetype_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_freetype STATIC ${freetype_sources})
    livi_gn_compile_config(livi_freetype third_party/freetype/BUILD.gn)
endif()

if(LITEVIEW_BUILD_BLINK_PORT)
    livi_read_sources(blink_sources BUILD.gn)
    list(FILTER blink_sources EXCLUDE REGEX "^(api|content|mbnet|shell|linux)/")
    # The root GN file repeats curl without the utility target's TLS defines.
    # Link the single correctly configured copy from livi_util.
    list(FILTER blink_sources EXCLUDE REGEX "^third_party/libcurl/")
    list(FILTER blink_sources EXCLUDE REGEX "/fonts/linux/")
    list(TRANSFORM blink_sources REPLACE "sync_socket_posix.cc$" "sync_socket_win.cc")
    list(TRANSFORM blink_sources REPLACE "memory_usage_monitor_posix.cc$" "memory_usage_monitor_win.cc")
    list(TRANSFORM blink_sources REPLACE "platform_mime_util_linux.cc$" "platform_mime_util_win.cc")
    list(TRANSFORM blink_sources REPLACE "layout_theme_linux.cc$" "layout_theme_win.cc")
    list(APPEND blink_sources
        third_party/blink/renderer/platform/fonts/win/font_unique_name_lookup_win.cc
        third_party/blink/renderer/platform/fonts/win/font_platform_data_win.cc
        third_party/blink/renderer/platform/fonts/win/fallback_family_style_cache_win.cc
        third_party/blink/renderer/platform/fonts/win/font_fallback_win.cc
        third_party/blink/renderer/platform/fonts/win/dwrite_font_format_support.cc
        third_party/blink/renderer/platform/fonts/win/font_cache_skia_win.cc
        third_party/blink/renderer/core/layout/web_font_rendering_win.cc
        third_party/blink/common/dwrite_rasterizer_support/dwrite_rasterizer_support.cc
        third_party/webrtc_overrides/low_precision_timer.cc
        mojo/public/cpp/mojo_lite_msg.cc
    )
    list(REMOVE_DUPLICATES blink_sources)
    list(TRANSFORM blink_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_blink STATIC ${blink_sources})
    livi_gn_compile_config(livi_blink BUILD.gn)

    livi_read_sources(blink_generated_sources gen/third_party/blink/renderer/BUILD.gn)
    list(TRANSFORM blink_generated_sources PREPEND "${LITEVIEW_SOURCE}/")
    # Some generated union names alone exceed Windows object path limits.
    # Compile a short wrapper so the original includes and diagnostics remain
    # intact, without renaming or modifying generated upstream sources.
    set(short_generated_sources)
    foreach(source IN LISTS blink_generated_sources)
        get_filename_component(filename "${source}" NAME)
        string(LENGTH "${filename}" filename_length)
        if(filename_length GREATER 100)
            string(SHA256 source_hash "${source}")
            string(SUBSTRING "${source_hash}" 0 16 source_hash)
            set(wrapper "${CMAKE_CURRENT_BINARY_DIR}/short/${source_hash}.cc")
            file(GENERATE OUTPUT "${wrapper}" CONTENT "#include \"${source}\"\n")
            list(APPEND short_generated_sources "${wrapper}")
        else()
            list(APPEND short_generated_sources "${source}")
        endif()
    endforeach()
    add_library(livi_blink_generated STATIC ${short_generated_sources})
    livi_gn_compile_config(livi_blink_generated gen/third_party/blink/renderer/BUILD.gn)
endif()

if(LITEVIEW_BUILD_ICU_PORT)
    livi_read_sources(icu_sources third_party/icu/BUILD.gn)
    list(TRANSFORM icu_sources PREPEND "${LITEVIEW_SOURCE}/")
    add_library(livi_icu STATIC ${icu_sources})
    target_include_directories(livi_icu PRIVATE
        "${LITEVIEW_SOURCE}"
        "${LITEVIEW_SOURCE}/third_party/icu/source/common"
        "${LITEVIEW_SOURCE}/third_party/icu/source/i18n"
        "${LITEVIEW_SOURCE}/third_party/icu/source/io"
    )
    target_compile_definitions(livi_icu PRIVATE
        U_STATIC_IMPLEMENTATION U_IO_IMPLEMENTATION U_COMMON_IMPLEMENTATION
        U_I18N_IMPLEMENTATION U_DEFINE_FALSE_AND_TRUE _CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(livi_icu PRIVATE /utf-8 /W0)
endif()

if(LITEVIEW_BUILD_BASE_PORT)
    livi_read_sources(base_sources base/BUILD.gn)
    list(FILTER base_sources EXCLUDE REGEX "^media/media_empty.cc$")
    set(windows_base_sources)
    foreach(source IN LISTS base_sources)
        if(source MATCHES "^base/(posix|nix)/" OR source MATCHES "^ui/gfx/linux/"
            OR source STREQUAL "ui/gfx/native_pixmap_handle.cc"
            OR source STREQUAL "base/debug/elf_reader.cc"
            OR source MATCHES "^base/allocator/partition_allocator/partition_alloc_base/posix/"
            OR source MATCHES "(scoped_file_linux|file_descriptor_watcher_posix|madv_free_discardable_memory_posix|watchable_io_message_pump_posix|internal_linux|platform_thread_internal_posix|page_allocator_internals_posix|file_descriptor_posix|file_descriptor_set_posix)\\.cc$")
            continue()
        endif()
        string(REPLACE "message_pump_glib.cc" "message_pump_win.cc" source "${source}")
        string(REGEX REPLACE "time_(now|exploded|conversion)_posix\\.cc$" "time_win.cc" source "${source}")
        string(REGEX REPLACE "_(posix|linux)\\.cc$" "_win.cc" source "${source}")
        if(NOT EXISTS "${LITEVIEW_SOURCE}/${source}")
            message(FATAL_ERROR "Missing Windows base source: ${source}")
        endif()
        list(APPEND windows_base_sources "${LITEVIEW_SOURCE}/${source}")
    endforeach()
    include("${CMAKE_CURRENT_LIST_DIR}/base-windows-sources.cmake")
    foreach(source IN LISTS base_windows_additions)
        list(APPEND windows_base_sources "${LITEVIEW_SOURCE}/${source}")
    endforeach()
    list(REMOVE_DUPLICATES windows_base_sources)
    add_library(livi_base STATIC ${windows_base_sources})
    livi_copy_compile_config(livi_base)
    target_compile_definitions(livi_base PRIVATE
        NO_TCMALLOC=1 ANGLE_ENABLE_GLSL=1 MEDIA_IMPLEMENTATION=1
    )
    target_compile_options(livi_base PRIVATE /EHs-c-)
    target_include_directories(livi_base PRIVATE
        "${LITEVIEW_SOURCE}/third_party"
        "${LITEVIEW_SOURCE}/third_party/angle/src"
        "${LITEVIEW_SOURCE}/gpu"
    )
endif()
