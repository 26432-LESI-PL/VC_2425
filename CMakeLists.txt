cmake_minimum_required(VERSION 3.21)  # Use 3.21 or later
project(VisaoComputador LANGUAGES C CXX)

add_subdirectory(lib) # Make sure this defines the 'vclib' target

find_package(OpenCV REQUIRED)

# Optional: Print a status message if found
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV Version: ${OpenCV_VERSION}")
    message(STATUS "OpenCV Include Directories: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV Libraries: ${OpenCV_LIBS}")
endif()

include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(VisaoComputador main.cpp)

# Add include directories (OpenCV headers and potentially vclib headers if needed)
target_include_directories(VisaoComputador PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    # Add ${CMAKE_CURRENT_SOURCE_DIR}/lib if vclib headers are there and not handled by vclib itself
)

# Link libraries using the KEYWORD signature ONLY
# Combine vclib and OpenCV libraries in one call
target_link_libraries(VisaoComputador PUBLIC
    vclib                   # Your library from the 'lib' subdirectory
    ${OpenCV_LIBS}          # The list of OpenCV libraries found by find_package
)
