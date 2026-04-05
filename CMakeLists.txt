cmake_minimum_required(VERSION 3.20)
project(discordc VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

find_package(cJSON CONFIG QUIET)
if(NOT cJSON_FOUND)
    pkg_check_modules(PC_CJSON REQUIRED libcjson)
    add_library(dc_cjson INTERFACE)
    target_include_directories(dc_cjson INTERFACE ${PC_CJSON_INCLUDE_DIRS})
    target_link_libraries(dc_cjson INTERFACE ${PC_CJSON_LINK_LIBRARIES})
    set(DC_CJSON_TARGET dc_cjson)
else()
    if(TARGET cJSON::cJSON)
        set(DC_CJSON_TARGET cJSON::cJSON)
    elseif(TARGET cjson)
        set(DC_CJSON_TARGET cjson)
    else()
        message(FATAL_ERROR "cJSON found but no known CMake target (cJSON::cJSON or cjson)")
    endif()
endif()

find_package(libwebsockets CONFIG QUIET)
if(NOT libwebsockets_FOUND)
    pkg_check_modules(PC_LWS REQUIRED libwebsockets)
    add_library(dc_lws INTERFACE)
    target_include_directories(dc_lws INTERFACE ${PC_LWS_INCLUDE_DIRS})
    target_link_libraries(dc_lws INTERFACE ${PC_LWS_LINK_LIBRARIES})
    set(DC_LWS_TARGET dc_lws)
else()
    if(TARGET websockets_shared)
        set(DC_LWS_TARGET websockets_shared)
    elseif(TARGET websockets)
        set(DC_LWS_TARGET websockets)
    else()
        message(FATAL_ERROR "libwebsockets found but no known CMake target (websockets_shared or websockets)")
    endif()
endif()

add_library(discordc
    src/client.c
    src/models.c
    src/rest.c
    src/router.c
    src/gateway.c
)

target_include_directories(discordc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(discordc
    PUBLIC
        CURL::libcurl
        ${DC_CJSON_TARGET}
        ${DC_LWS_TARGET}
)

if(WIN32)
    target_link_libraries(discordc PUBLIC ws2_32 crypt32)
endif()

add_executable(discordc_example examples/basic_bot.c)
target_link_libraries(discordc_example PRIVATE discordc)
