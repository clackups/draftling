# Fails the build when the co-processor image is larger than the
# smallest OTA slot it may be installed into (see CMakeLists.txt).
if(NOT EXISTS "${BIN}")
    return()
endif()
file(SIZE "${BIN}" size)
math(EXPR max "${MAX}")
if(size GREATER max)
    message(FATAL_ERROR "Co-processor image is ${size} bytes, more than "
        "the ${max} bytes of the smallest OTA slot it may be installed "
        "into (CP_MAX_APP_SIZE). Reduce the image size.")
endif()
math(EXPR free "${max} - ${size}")
message(STATUS "Co-processor image: ${size} bytes, ${free} bytes below CP_MAX_APP_SIZE")
