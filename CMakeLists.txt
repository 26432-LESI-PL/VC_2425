cmake_minimum_required(VERSION 3.21)  # Use 3.21 or later
project(VisaoComputador LANGUAGES C CXX)

add_subdirectory(lib)

find_package(OpenCV REQUIRED)

if(OpenCV_FOUND)
    message(STATUS "Found OpenCV Version: ${OpenCV_VERSION}")
    message(STATUS "OpenCV Include Directories: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV Libraries: ${OpenCV_LIBS}")
endif()

include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(VisaoComputador main.cpp)

target_include_directories(VisaoComputador PUBLIC
    ${OpenCV_INCLUDE_DIRS}
)


target_link_libraries(VisaoComputador PUBLIC
    vclib
    ${OpenCV_LIBS}
)

# Adicionar os ficheiros de vídeo ao diretório de saída
set(VIDEO_FILES
    "${CMAKE_SOURCE_DIR}/video1.mp4"
    "${CMAKE_SOURCE_DIR}/video2.mp4"
)

foreach(VIDEO_FILE ${VIDEO_FILES})
    get_filename_component(FILENAME ${VIDEO_FILE} NAME) # Extract only the filename
    add_custom_command(
        TARGET VisaoComputador POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VIDEO_FILE}" "$<TARGET_FILE_DIR:VisaoComputador>"
        COMMENT "Copying video file to output directory"
    )
endforeach()