cmake_minimum_required(VERSION 3.16)
project(editor_menu_tests LANGUAGES CXX)

enable_testing()

function(add_editor_menu_test name)
    add_executable(${name} ../editor_menu.cpp editor_menu_test.cpp)
    target_include_directories(${name} PRIVATE ../include)
    target_compile_features(${name} PRIVATE cxx_std_11)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif()
    foreach(feature IN LISTS ARGN)
        target_compile_definitions(${name} PRIVATE ${feature}=1)
    endforeach()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_editor_menu_test(editor_menu_base)
add_editor_menu_test(editor_menu_usb CONFIG_DRAFTLING_HAS_USB_MSC)
add_editor_menu_test(editor_menu_kermit CONFIG_DRAFTLING_HAS_KERMIT)
add_editor_menu_test(editor_menu_usb_kermit
    CONFIG_DRAFTLING_HAS_USB_MSC
    CONFIG_DRAFTLING_HAS_KERMIT)
