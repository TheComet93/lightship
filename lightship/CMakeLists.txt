###############################################################################
# compiler flags for this project
###############################################################################

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    add_definitions (-W -Wall -Wextra -pedantic -Wno-unused-parameter)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    add_definitions (-W -Wall -Wextra -pedantic -Wno-unused-parameter)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "Intel")
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
endif ()

###############################################################################
# source files and runtime definition
###############################################################################

file (GLOB lightship_HEADERS "include/lightship/*.h")
file (GLOB lightship_SOURCES "src/*.c")

set (lightship_HEADERS ${lightship_HEADERS}
    "include/lightship/config.h.in"
    "${CMAKE_SOURCE_DIR}/util/include/common/export.h.in"
)

include_directories ("include")

add_executable (lightship
    ${lightship_HEADERS}
    ${lightship_SOURCES}
)

if(${PLATFORM} MATCHES "LINUX" OR ${PLATFORM} MATCHES "MACOSX")
    set (MATH_LIB m)
endif ()

target_link_libraries (lightship
    ${MATH_LIB}
    lightship_util
    framework
)

# generates the correct project files for visual studio, setting the correct
# debug working directory and other stuff
create_vcproj_userfile (lightship)

###############################################################################
# generate config.h and export.h
###############################################################################

set (PROJECT_NAME "LIGHTSHIP")

configure_file ("${EXPORT_H_TEMPLATE}"
                "include/lightship/export.h")
configure_file ("include/lightship/config.h.in"
                "include/lightship/config.h")

include_directories ("${CMAKE_CURRENT_BINARY_DIR}/include")

###############################################################################
# install targets
###############################################################################

install (
    TARGETS
        lightship
    DESTINATION
        "bin"
)
install (
    DIRECTORY
        "${CMAKE_CURRENT_LIST_DIR}/cfg"
    DESTINATION
        "bin"
)
