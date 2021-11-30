
configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_subdirectory(addon)
add_subdirectory(inputmethod)
add_executable(testunikey testunikey.cpp)
target_link_libraries(testunikey Fcitx5::Core Fcitx5::Module::TestFrontend)
add_dependencies(testunikey unikey copy-addon copy-im)
add_test(testunikey testunikey)
