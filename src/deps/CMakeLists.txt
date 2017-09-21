set(B64_SOURCE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/b64/b64.h
        ${CMAKE_CURRENT_LIST_DIR}/b64/decode.c
        ${CMAKE_CURRENT_LIST_DIR}/b64/encode.c
        )

set(FILE_SOURCE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/file/file.c
        )

set(ASPRINTF_SOURCE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/asprintf/asprintf.c
        )

set(DEPS_SOURCE_FILES
        ${B64_SOURCE_FILES}
        ${FILE_SOURCE_FILES}
        ${ASPRINTF_SOURCE_FILES}
        )
