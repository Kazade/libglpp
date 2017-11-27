CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(libglpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

ADD_LIBRARY(
    GL
    src/gl.cpp
)

ADD_EXECUTABLE(
    nehe01
    test/apps/nehe01.cpp
)

INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/include)
TARGET_LINK_LIBRARIES(nehe01 GL)

SET(DC_EXECUTABLES
    nehe01
)

FOREACH(EXE ${DC_EXECUTABLES})
get_filename_component(EXE_FILENAME ${EXE} NAME)
ADD_CUSTOM_TARGET(
    ${EXE_FILENAME}.bin ALL
    COMMAND $ENV{KOS_OBJCOPY} -R .stack -O binary -S -g ${CMAKE_BINARY_DIR}/${EXE}.elf ${CMAKE_BINARY_DIR}/${EXE}.bin
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/${EXE}
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${EXE}
    COMMAND $ENV{KOS_BASE}/utils/scramble/scramble ${CMAKE_BINARY_DIR}/${EXE}.bin ${CMAKE_BINARY_DIR}/${EXE}/1ST_READ.BIN
    COMMAND IP_TEMPLATE_FILE=${CMAKE_SOURCE_DIR}/IP.TMPL $ENV{KOS_BASE}/utils/makeip/makeip ${CMAKE_SOURCE_DIR}/ip.txt ${CMAKE_BINARY_DIR}/${EXE}/IP.BIN
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${EXE}/resources
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/test/apps/resources/${EXE} ${CMAKE_BINARY_DIR}/${EXE}/resources
    COMMAND mkisofs -C 0,11702 -V ${EXE_FILENAME} -G ${CMAKE_BINARY_DIR}/${EXE}/IP.BIN -joliet -rock -l -o ${EXE}.iso ${CMAKE_BINARY_DIR}/${EXE}
    COMMAND $ENV{KOS_BASE}/utils/img4dc/build/cdi4dc/cdi4dc ${CMAKE_BINARY_DIR}/${EXE}.iso ${CMAKE_BINARY_DIR}/${EXE}.cdi
    DEPENDS ${EXE}
    COMMENT "Converting ${EXE}.elf to ${EXE}.bin"
)
ENDFOREACH()

