cmake_minimum_required(VERSION 3.6)
project(netexec)

set(CMAKE_C_STANDARD 11)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

    SET (CMAKE_C_COMPILER             "/usr/bin/clang-5.0")
    SET (CMAKE_C_FLAGS                "-Wall -std=c99")
    SET (CMAKE_C_FLAGS_DEBUG          "-g")
    SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
    SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

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
target_link_libraries(netexec ${CMAKE_THREAD_LIBS_INIT} ${LIBEV_LDFLAGS} -lssl -lcrypto -lm)

# list
set(LIST_SOURCE_FILES src/main.list.c src/log.h src/log.c src/list.h)
add_executable(list ${LIST_SOURCE_FILES})
target_link_libraries(list ${CMAKE_THREAD_LIBS_INIT} ${LIBEV_LDFLAGS})

# bench
set(BENCH_SOURCE_FILES src/bench.c src/log.h src/log.c src/list.h src/array-heap.c src/array-heap.h)
add_executable(bench ${BENCH_SOURCE_FILES})
target_link_libraries(bench ${CMAKE_THREAD_LIBS_INIT} ${LIBEV_LDFLAGS})

# ssl-example
set(SSL_SOURCE_FILES src/ssl-example.c)
add_executable(ssl-example ${SSL_SOURCE_FILES})
target_link_libraries(ssl-example ${CMAKE_THREAD_LIBS_INIT} ${LIBEV_LDFLAGS} -lssl -lcrypto)

# ssl-server
set(SSLS_SOURCE_FILES
        src/ssl-server.c
        src/ssl.c src/ssl.h)
add_executable(ssl-server ${SSLS_SOURCE_FILES})
target_link_libraries(ssl-server ${CMAKE_THREAD_LIBS_INIT} ${LIBEV_LDFLAGS} -lssl -lcrypto)

# list-dir
add_executable(listdir src/listdir.c)
target_link_libraries(listdir)

# small-test
add_executable(small-test src/small-test.c)

