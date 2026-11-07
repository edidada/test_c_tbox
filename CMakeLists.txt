# ---------------------------------------------------------------------------
# tbox feature samples: one executable per tbox feature
#
# The samples are built with CMake, but tbox itself is NOT built here.
# tbox is built with xmake first, then installed with `xmake install -o <dir>`
# and consumed through TBOX_ROOT, e.g. on CI:
#
#   git clone https://github.com/tboox/tbox.git tbox && cd tbox
#   xmake f -c -y -m release --small=false --demo=false \
#       --mysql=n --zlib=n --pcre=n --pcre2=n --polarssl=n --mbedtls=n --openssl=n
#   xmake -y
#   xmake install -o ../tbox-install tbox
#   cd ..
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTBOX_ROOT=$PWD/tbox-install
#   cmake --build build -j && ctest -C Release --output-on-failure
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(tbox_samples C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# xmake builds tbox on Windows with the static CRT (/MT for release).
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(TBOX_ROOT "Install root of tbox (created by: xmake install -o <dir> tbox)" "")
option(TBOX_ENABLE_DATABASE "Link sqlite3 (requires tbox built with --database=y and sqlite3)" ON)

# ---------------------------------------------------------------------------
# find the installed tbox
# ---------------------------------------------------------------------------
find_path(TBOX_INCLUDE_DIR tbox/tbox.h
    HINTS ${TBOX_ROOT}/include
    NO_DEFAULT_PATH)
find_library(TBOX_LIBRARY
    NAMES tbox
    HINTS ${TBOX_ROOT}/lib ${TBOX_ROOT}/lib64 ${TBOX_ROOT}/lib32
    NO_DEFAULT_PATH)

if(NOT TBOX_INCLUDE_DIR OR NOT TBOX_LIBRARY)
    message(FATAL_ERROR
        "tbox not found under TBOX_ROOT '${TBOX_ROOT}'. Build and install tbox first.\n"
        "  git clone https://github.com/tboox/tbox.git tbox && cd tbox\n"
        "  xmake f -c -y -m release --small=false --demo=false --mysql=n --zlib=n --pcre=n --pcre2=n --polarssl=n --mbedtls=n --openssl=n\n"
        "  xmake -y && xmake install -o ../tbox-install tbox\n"
        "  then configure: cmake -B build -DTBOX_ROOT=../tbox-install")
endif()

message(STATUS "tbox include : ${TBOX_INCLUDE_DIR}")
message(STATUS "tbox library : ${TBOX_LIBRARY}")

add_library(tbox UNKNOWN IMPORTED)
set_target_properties(tbox PROPERTIES
    IMPORTED_LOCATION             ${TBOX_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${TBOX_INCLUDE_DIR})

# ---------------------------------------------------------------------------
# system libraries required by static libtbox
# ---------------------------------------------------------------------------
if(WIN32)
    set(TBOX_PLATFORM_LIBS ws2_32 user32)
elseif(APPLE)
    find_library(COREFOUNDATION_FW CoreFoundation)
    find_library(CORESERVICES_FW CoreServices)
    set(TBOX_PLATFORM_LIBS ${COREFOUNDATION_FW} ${CORESERVICES_FW})
else()
    set(TBOX_PLATFORM_LIBS pthread dl m)
endif()

# ---------------------------------------------------------------------------
# sqlite3 needed by the tbox database module (static libtbox has unresolved
# sqlite3 symbols until we link them)
# ---------------------------------------------------------------------------
set(TBOX_DATABASE_LIBS "")
if(TBOX_ENABLE_DATABASE)
    find_library(TBOX_SQLITE3_LIBRARY
        NAMES sqlite3 sqlite3_static
        HINTS ${TBOX_ROOT}/lib ${TBOX_ROOT}/lib64 ${TBOX_ROOT}/lib32
        NO_DEFAULT_PATH)
    if(NOT TBOX_SQLITE3_LIBRARY)
        find_package(SQLite3 QUIET)
        if(SQLite3_FOUND)
            set(TBOX_SQLITE3_LIBRARY ${SQLite3_LIBRARIES})
        endif()
    endif()
    if(NOT TBOX_SQLITE3_LIBRARY)
        message(WARNING "sqlite3 not found; sample_database will report 'not supported'")
    else()
        set(TBOX_DATABASE_LIBS ${TBOX_SQLITE3_LIBRARY})
        message(STATUS "sqlite3      : ${TBOX_SQLITE3_LIBRARY}")
    endif()
endif()

enable_testing()

foreach(sample stream coroutine parser container database memory)
    add_executable(sample_${sample} samples/${sample}/${sample}.c)
    target_link_libraries(sample_${sample} PRIVATE
        tbox
        ${TBOX_DATABASE_LIBS}
        ${TBOX_PLATFORM_LIBS})
    if(MSVC)
        target_compile_options(sample_${sample} PRIVATE /W1)
    else()
        target_compile_options(sample_${sample} PRIVATE -Wall)
    endif()
    add_test(NAME ${sample} COMMAND sample_${sample}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${sample} PROPERTIES TIMEOUT 120)
endforeach()
