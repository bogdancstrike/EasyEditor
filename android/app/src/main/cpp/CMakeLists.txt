cmake_minimum_required(VERSION 3.22)
project(video_editor_android LANGUAGES C CXX)

add_subdirectory(
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../core
    ${CMAKE_CURRENT_BINARY_DIR}/core
)

target_sources(videoeditor PRIVATE
    jni_bindings.cpp
)

target_link_libraries(videoeditor PRIVATE
    log
)
