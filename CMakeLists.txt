cmake_minimum_required(VERSION 3.6)
project(netexec)

set(CMAKE_C_STANDARD 11)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

    SET (CMAKE_C_COMPILER             "/usr/bin/clang-5.0")
#    SET (CMAKE_C_FLAGS                "-Wall -std=c11 -Wall -Wsign-compare -pipe -g3 -fvisibility=hidden -O3 -funroll-loops")
#    SET (CMAKE_C_FLAGS_DEBUG          "-g -Wall -Wsign-compare -pipe -g3 -fvisibility=hidden -O3 -funroll-loops")
#    SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG -Wall -Wsign-compare -pipe -g3 -fvisibility=hidden -O3 -funroll-loops")
#    SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG -Wall -Wsign-compare -pipe -g3 -fvisibility=hidden -O3 -funroll-loops")
#    SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -Wall -Wsign-compare -pipe -g3 -fvisibility=hidden -O3 -funroll-loops")

#    SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++5.0")
#    SET (CMAKE_CXX_FLAGS                "-Wall")
#    SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
#    SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#    SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
#    SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
#
#    SET (CMAKE_AR      "/usr/bin/llvm-ar")
#    SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
#    SET (CMAKE_NM      "/usr/bin/llvm-nm")
#    SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
#    SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")


include(CheckIncludeFiles)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(LibEv REQUIRED)

# Add include dirs
include_directories(src/deps)


# b64.c lib files
include(src/deps/CMakeLists.txt)

# netexec
set(SOURCE_FILES
        src/main.c
        src/global.h
        src/log.h
        src/log.c
        src/list.h
        src/http.c
        src/http.h
        src/http_parser.c
        src/ssl.c
        src/ssl.h
        src/cJSON.c
        src/cJSON.h
        src/job.c
        src/job.h
        src/ev_handlers.c
        src/ev_handlers.h
        src/jsonrpc.c
        src/jsonrpc.h
        src/jrpc_methods.c
        src/jrpc_methods.h
        )
add_executable(netexec ${SOURCE_FILES} ${DEPS_SOURCE_FILES})
target_link_libraries(netexec ${LIBEV_LDFLAGS} -lssl -lcrypto -lm
        )

