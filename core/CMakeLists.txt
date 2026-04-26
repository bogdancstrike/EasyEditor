# core/CMakeLists.txt — the shared engine.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(VX_PUBLIC_HEADERS
    include/vx/public_api.h
    include/vx/version.h
)

# Layer libraries. Each layer is its own static lib so include-visibility
# can be enforced via target_include_directories(... PRIVATE/PUBLIC).
add_library(vx_util STATIC
    src/util/log.cpp
    src/util/log.h
    src/util/error.h
    src/util/uuid.cpp
    src/util/uuid.h
)
target_include_directories(vx_util PUBLIC src)
target_include_directories(vx_util PUBLIC include)

add_library(vx_domain STATIC
    src/domain/time.cpp
    src/domain/time.h
    src/domain/rational.h
    src/domain/clip.h
    src/domain/sequence.h
    src/domain/project.cpp
    src/domain/project.h
    src/domain/project_migration.cpp
    src/domain/project_migration.h
    src/domain/command.h
    src/domain/command_stack.cpp
    src/domain/command_stack.h
)
target_link_libraries(vx_domain PUBLIC vx_util)
target_include_directories(vx_domain PUBLIC src)
target_include_directories(vx_domain PRIVATE third_party)

add_library(vx_platform_iface INTERFACE)
target_include_directories(vx_platform_iface INTERFACE src)
# Platform interface headers live under src/platform/*.h (no .cpp here).

add_library(vx_platform_mock STATIC
    src/platform/mock/mock_gpu_backend.cpp
    src/platform/mock/mock_gpu_backend.h
)
target_link_libraries(vx_platform_mock PUBLIC vx_platform_iface vx_util)
target_include_directories(vx_platform_mock PUBLIC src)

add_library(vx_application STATIC
    src/application/project_service.cpp
    src/application/project_service.h
    src/application/render_graph.cpp
    src/application/render_graph.h
    src/application/nodes/sequence_node.cpp
    src/application/nodes/sequence_node.h
    src/application/nodes/lut_node.cpp
    src/application/nodes/lut_node.h
)
target_link_libraries(vx_application PUBLIC vx_domain vx_platform_iface)

# FFI is the only layer that exposes extern "C". Bundles everything below.
add_library(videoeditor SHARED
    src/ffi/exception_guard.h
    src/ffi/init.cpp
    src/ffi/project_ffi.cpp
    src/ffi/version_ffi.cpp
)
target_link_libraries(videoeditor PRIVATE vx_application vx_domain vx_util)
target_include_directories(videoeditor PUBLIC include)
set_target_properties(videoeditor PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Platform-concrete backends. Selected at configure time.
if (ANDROID)
    add_library(vx_platform_vulkan STATIC
        src/platform/vulkan/vulkan_backend.cpp
        src/platform/vulkan/vulkan_backend.h
    )
    target_link_libraries(vx_platform_vulkan PUBLIC vx_platform_iface PRIVATE vx_util vulkan)
    target_include_directories(vx_platform_vulkan PUBLIC src)
    
    add_library(vx_platform_gles STATIC
        src/platform/gles/gles_backend.cpp
        src/platform/gles/gles_backend.h
    )
    target_link_libraries(vx_platform_gles PUBLIC vx_platform_iface PRIVATE vx_util GLESv3)
    target_include_directories(vx_platform_gles PUBLIC src)

    add_library(vx_platform_android STATIC
        src/platform/android/android_codec_backend.cpp
        src/platform/android/android_codec_backend.h
    )
    target_link_libraries(vx_platform_android PUBLIC vx_platform_iface PRIVATE vx_util mediandk android)
    target_include_directories(vx_platform_android PUBLIC src)

    target_link_libraries(videoeditor PRIVATE vx_platform_vulkan vx_platform_gles vx_platform_android android)
    # add_subdirectory(src/platform/mediacodec)    # TODO Phase 0
endif()

if (APPLE)
    # add_subdirectory(src/platform/metal)         # TODO Phase 5
    # add_subdirectory(src/platform/avfoundation)  # TODO Phase 5
endif()

if (VX_BUILD_TESTS AND NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
