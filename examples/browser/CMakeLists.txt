qt_add_executable(qliteview_example main.cpp)
qt_add_qml_module(qliteview_example
    URI QLiteViewExample
    VERSION 1.0
    DEPENDENCIES TARGET qliteview
    QML_FILES Main.qml Browser.qml
    RESOURCES pages/index.html pages/second.html
)
target_link_libraries(qliteview_example PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::WebChannel)
target_compile_options(qliteview_example PRIVATE /utf-8)
add_dependencies(qliteview_example qliteview)
# Keep both browser DLLs beside the executable when either runtime changes.
set_property(TARGET qliteview_example APPEND PROPERTY LINK_DEPENDS
    "$<TARGET_FILE:qliteview>" "${QLITEVIEW_RUNTIME_DIR}/livicore.dll")
add_custom_command(TARGET qliteview_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:qliteview> $<TARGET_FILE_DIR:qliteview_example>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QLITEVIEW_RUNTIME_DIR}/livicore.dll" "$<TARGET_FILE_DIR:qliteview_example>"
)
