cmake_minimum_required(VERSION 3.6)

project(fcitx5-unikey VERSION 5.0.2)

find_package(ECM 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

option(ENABLE_QT "Enable Qt based macro editor" On)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS TestFrontend)
find_package(Gettext REQUIRED)
find_package(Pthread REQUIRED)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-unikey\")
fcitx5_add_i18n_definition()

if (ENABLE_QT)
  find_package(Qt5 5.7 REQUIRED COMPONENTS Core Gui Widgets)
  find_package(Fcitx5Qt5WidgetsAddons REQUIRED)
  add_subdirectory(macro-editor)
endif (ENABLE_QT)

add_subdirectory(po)
add_subdirectory(unikey)
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(test)

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Unikey.metainfo.xml.in
    org.fcitx.Fcitx5.Addon.Unikey.metainfo.xml XML)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Unikey.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

