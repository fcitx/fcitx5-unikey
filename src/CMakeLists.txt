
set( fcitx_unikey_sources
    unikey-im.cpp
    )

add_library(unikey MODULE ${fcitx_unikey_sources})
target_link_libraries(unikey Fcitx5::Core Fcitx5::Config unikey-lib)
target_include_directories(unikey PRIVATE ${PROJECT_BINARY_DIR})
if (ENABLE_QT)
target_compile_definitions(unikey PRIVATE "-DENABLE_QT")
endif()
install(TARGETS unikey DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
fcitx5_translate_desktop_file(unikey.conf.in unikey.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/unikey.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod" COMPONENT config)
configure_file(unikey-addon.conf.in.in unikey-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/unikey-addon.conf.in" unikey-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/unikey-addon.conf" RENAME unikey.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon" COMPONENT config)
