cmake_minimum_required(VERSION 4.1)
project(tdcf_test)

set(CMAKE_CXX_STANDARD 17)

FILE(GLOB_RECURSE SRC_LIST "*.cpp" ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB_RECURSE HEADER_LIST "*.hpp" ${CMAKE_CURRENT_SOURCE_DIR})

set(EXCLUDE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/cmake-build*")

list(FILTER SRC_LIST EXCLUDE REGEX "^${EXCLUDE_PREFIX}")

list(FILTER HEADER_LIST EXCLUDE REGEX "^${EXCLUDE_PREFIX}")

message(STATUS "Header files: ${HEADER_LIST}")

message(STATUS "Source files: ${SRC_LIST}")

unset(USE_TLOG CACHE)
find_package(tlog QUIET)
if (tlog_FOUND)
    message(STATUS "tlog found, using tlog for logging")
    configure_file("${PROJECT_SOURCE_DIR}/cmake_config/log1.hpp.in" "${PROJECT_SOURCE_DIR}/log.hpp")
    option(USE_TLOG "Enable tlog library instead of printf for logging" ON)
else ()
    message(WARNING "tlog not found (USE_TLOG=ON but library missing), fallback to printf")
    configure_file("${PROJECT_SOURCE_DIR}/cmake_config/log2.hpp.in" "${PROJECT_SOURCE_DIR}/log.hpp")
    option(USE_TLOG "Enable tlog library instead of printf for logging" OFF)
endif ()


find_package(tdcf REQUIRED)

add_executable(${PROJECT_NAME} ${HEADER_LIST} ${SRC_LIST} log.hpp)

target_include_directories(${PROJECT_NAME}
        PRIVATE tdcf
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE tdcf
)

if (USE_TLOG)
    target_include_directories(${PROJECT_NAME}
            PRIVATE tlog
    )

    target_link_libraries(${PROJECT_NAME}
            PRIVATE tlog
    )
endif ()