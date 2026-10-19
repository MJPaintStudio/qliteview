qt_add_executable(qliteview_smoke smoke.cpp smoke.h)
set_target_properties(qliteview_smoke PROPERTIES WIN32_EXECUTABLE FALSE)
target_link_libraries(qliteview_smoke PRIVATE Qt6::Quick Qt6::WebChannel Qt6::Test Qt6::Network)
target_compile_options(qliteview_smoke PRIVATE /utf-8)
target_compile_definitions(qliteview_smoke PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN
    QLITEVIEW_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(qliteview_smoke_runtime
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:qliteview>" "$<TARGET_FILE_DIR:qliteview_smoke>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QLITEVIEW_RUNTIME_DIR}/livicore.dll" "$<TARGET_FILE_DIR:qliteview_smoke>"
    DEPENDS qliteview
)
add_dependencies(qliteview_smoke qliteview_smoke_runtime)
add_test(NAME qliteview_smoke COMMAND qliteview_smoke -o "${CMAKE_CURRENT_BINARY_DIR}/smoke-result.txt,txt")
set_tests_properties(qliteview_smoke PROPERTIES TIMEOUT 180)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME qliteview_runtime_deployment
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/runtime_deployment.py"
        "$<TARGET_FILE:qliteview_smoke>" "$<TARGET_FILE:qliteview>" "${QLITEVIEW_RUNTIME_DIR}"
)
set_tests_properties(qliteview_runtime_deployment PROPERTIES TIMEOUT 60)
