# Link stage for the independently built port components. Keeping this stage
# separate avoids recompiling completed large dependencies while diagnosing
# the platform's missing symbols.
set(LITEVIEW_PORT_BUILD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../build" CACHE PATH
    "Root containing the Windows port component builds")
# The Linux GN list omits libuv, which the Windows message loop uses.
set(uv_names
    fs-poll idna inet random strscpy strtok thread-common threadpool timer
    uv-common uv-data-getter-setters version
    win/async win/core win/detect-wakeup win/dl win/error win/fs win/fs-event
    win/getaddrinfo win/getnameinfo win/handle win/loop-watcher win/pipe win/poll
    win/process win/process-stdio win/signal win/snprintf win/stream win/tcp
    win/thread win/tty win/udp win/util win/winapi win/winsock)
set(uv_sources)
foreach(name IN LISTS uv_names)
    list(APPEND uv_sources "${LITEVIEW_SOURCE}/third_party/libuv/src/${name}.c")
endforeach()
add_library(livi_uv STATIC ${uv_sources})
target_include_directories(livi_uv PRIVATE
    "${LITEVIEW_SOURCE}/third_party/libuv/include"
    "${LITEVIEW_SOURCE}/third_party/libuv/src")
target_compile_definitions(livi_uv PRIVATE WIN32_LEAN_AND_MEAN UNICODE _UNICODE _WIN32_WINNT=0x0A00 _CRT_SECURE_NO_WARNINGS)
target_compile_options(livi_uv PRIVATE /W0)

set(dav1d_root "${LITEVIEW_SOURCE}/third_party/dav1d/libdav1d")
set(dav1d_common_names cdf cpu_dav1d data_dav1d decode_dav1d dequant_tables
    getbits intra_edge itx_1d lf_mask log_dav1d mem_dav1d msac_dav1d obu_dav1d
    picture qm_dav1d ref_dav1d refmvs_ scan_dav1d tables_dav1d thread_task
    warpmv wedge win32/thread_dav1d)
set(dav1d_sources)
foreach(name IN LISTS dav1d_common_names)
    list(APPEND dav1d_sources "${dav1d_root}/src/${name}.c")
endforeach()
add_library(livi_dav1d STATIC ${dav1d_sources})
set(dav1d_template_names cdef_apply cdef fg_apply filmgrain ipred_prepare
    ipred itx lf_apply loopfilter looprestoration lr_apply mc recon)
foreach(depth 8 16)
    set(dav1d_templates)
    foreach(name IN LISTS dav1d_template_names)
        list(APPEND dav1d_templates "${dav1d_root}/src/${name}_tmpl.c")
    endforeach()
    add_library(livi_dav1d_${depth} OBJECT ${dav1d_templates})
    target_compile_definitions(livi_dav1d_${depth} PRIVATE BITDEPTH=${depth})
    target_sources(livi_dav1d PRIVATE $<TARGET_OBJECTS:livi_dav1d_${depth}>)
endforeach()
foreach(target livi_dav1d livi_dav1d_8 livi_dav1d_16)
    target_include_directories(${target} PRIVATE "${dav1d_root}" "${dav1d_root}/include" "${LITEVIEW_SOURCE}"
        "${LITEVIEW_SOURCE}/third_party/dav1d/version")
    target_compile_definitions(${target} PRIVATE WIN32 _WIN32_WINNT=0x0A00
        CONFIG_LOG=0 STACK_ALIGNMENT=16 DAV1D_API= _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${target} PRIVATE /W0)
endforeach()
add_library(livicore SHARED DllMain.cpp $<TARGET_OBJECTS:livicore_port_check>)
target_link_libraries(livicore PRIVATE
    livi_uv livi_dav1d
    "${LITEVIEW_PORT_BUILD_ROOT}/b/livi_blink.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/bg/livi_blink_generated.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-port/livi_v8.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-base/livi_base.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-icu/livi_icu.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-graphics/livi_skia.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-graphics/livi_freetype.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-util/livi_util.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-util/livi_ssl.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-gl/livi_angle.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-gl/livi_gl.lib"
    "${LITEVIEW_PORT_BUILD_ROOT}/windows-media/livi_media_base.lib"
    user32 gdi32 imm32 comdlg32 comctl32 advapi32 ole32 oleaut32
    shell32 shlwapi ws2_32 crypt32 bcrypt ncrypt winmm version dbghelp
    psapi iphlpapi userenv setupapi cfgmgr32 propsys powrprof dwmapi
    dwrite d3d11 d3d12 d3dcompiler dxgi dxguid opengl32 usp10 msimg32
    winspool secur32 wtsapi32 runtimeobject wldap32)
target_link_options(livicore PRIVATE /INCREMENTAL:NO /OPT:REF /OPT:ICF /DEBUG /MAP /errorlimit:0)
set_target_properties(livicore PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
