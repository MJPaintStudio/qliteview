qt_add_qml_module(qliteview
    SHARED
    URI QWebKit
    VERSION 1.0
    PLUGIN_TARGET qliteview
    DEPENDENCIES QtQuick QtWebChannel
    SOURCES
        webengineview.h webengineview.cpp
        webenginesettings.h
        webengineprofile.h webengineprofile.cpp
        webenginetypes.h webenginetypes.cpp
        liteviewbackend.h liteviewbackend.cpp
        qwebkit.h qwebkit.cpp
)
target_link_libraries(qliteview PRIVATE
    Qt6::Quick Qt6::WebChannel user32 gdi32 imm32 comdlg32 delayimp
    "${QLITEVIEW_ENGINE_BUILD}/livicore.lib"
)
target_include_directories(qliteview PRIVATE
    "${CMAKE_SOURCE_DIR}/liteview"
    "${CMAKE_SOURCE_DIR}/ports/windows"
)
target_compile_definitions(qliteview PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN QWEBKIT_BUILD ENABLE_MB=0)
target_compile_options(qliteview PRIVATE /utf-8)
target_link_options(qliteview PRIVATE /DELAYLOAD:livicore.dll)

if(QLITEVIEW_EMBED_RUNTIME)
    set(manifest "${QLITEVIEW_RUNTIME_DIR}/manifest.json")
    if(NOT EXISTS "${manifest}")
        message(FATAL_ERROR "Run tools/Prepare-Runtime.ps1 before embedding the native runtime.")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${manifest}")
    file(READ "${manifest}" runtime_json)
    string(JSON runtime_count LENGTH "${runtime_json}" files)
    if(runtime_count LESS 1)
        message(FATAL_ERROR "The runtime manifest is empty.")
    endif()
    math(EXPR runtime_last "${runtime_count} - 1")
    set(runtime_files "${manifest}")
    foreach(index RANGE 0 ${runtime_last})
        string(JSON runtime_file GET "${runtime_json}" files ${index} path)
        list(APPEND runtime_files "${QLITEVIEW_RUNTIME_DIR}/${runtime_file}")
    endforeach()
    qt_add_resources(qliteview "qliteview_runtime"
        BIG_RESOURCES
        PREFIX "/qliteview/runtime"
        BASE "${QLITEVIEW_RUNTIME_DIR}"
        FILES ${runtime_files}
        # Let the installer compress the native PE directly. Compressing it here
        # first prevents LZMA2 from finding redundancy in its code and data.
        OPTIONS --no-compress
    )
    target_compile_definitions(qliteview PRIVATE QWEBKIT_EMBED_RUNTIME)
else()
    target_compile_definitions(qliteview PRIVATE QWEBKIT_DEV_RUNTIME_DIR="${QLITEVIEW_RUNTIME_DIR}")
endif()
