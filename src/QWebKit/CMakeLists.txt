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

if(NOT EXISTS "${QLITEVIEW_RUNTIME_DIR}/livicore.dll")
    message(FATAL_ERROR "Run tools/Prepare-Runtime.ps1 before building QLiteView.")
endif()
add_custom_target(qliteview_runtime
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:qliteview>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QLITEVIEW_RUNTIME_DIR}/livicore.dll" "$<TARGET_FILE_DIR:qliteview>/livicore.dll"
    VERBATIM
)
add_dependencies(qliteview qliteview_runtime)
