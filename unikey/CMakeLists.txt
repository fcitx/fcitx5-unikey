
set(UNIKEY_SRCS
    byteio.cpp
    charset.cpp
    convert.cpp
    data.cpp
    inputproc.cpp
    mactab.cpp
    pattern.cpp
    ukengine.cpp
    usrkeymap.cpp
    unikeyinputcontext.cpp
)


add_library(unikey-lib STATIC ${UNIKEY_SRCS})
target_link_libraries(unikey-lib Fcitx5::Utils)
set_target_properties(unikey-lib PROPERTIES COMPILE_FLAGS "-fPIC")
target_include_directories(unikey-lib PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
