function(add_vx_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE videoeditor vx_domain vx_application vx_platform_mock)
    target_include_directories(${target} PRIVATE
        ../include
        ../src
    )
    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_vx_test(vx_time_test time_test.cpp)
add_vx_test(vx_command_stack_test command_stack_test.cpp)
add_vx_test(vx_project_test project_test.cpp)
add_vx_test(vx_mock_gpu_backend_test mock_gpu_backend_test.cpp)
add_vx_test(vx_ffi_test ffi_test.cpp)
add_vx_test(vx_render_graph_test render_graph_test.cpp)
add_vx_test(vx_lut_node_test lut_node_test.cpp)
