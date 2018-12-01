cmake_minimum_required(VERSION 3.4)
project(wstest)

set(Libwebsockets_DIR /home/yi/workspace/libwebsockets/install/lib/cmake/libwebsockets)

set(MY_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
set(CMAKE_INSTALL_PREFIX "${MY_INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

find_package(Libwebsockets CONFIG REQUIRED NO_DEFAULT_PATH)
include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})

include_directories(include)

aux_source_directory(src SRC_FILES)

add_executable(wsserver  ${SRC_FILES})
# target_compile_options(wsserver PRIVATE -fpermissive)
target_link_libraries(wsserver ${LIBWEBSOCKETS_LIBRARIES})
