cmake_minimum_required(VERSION 3.6)

project(fcitx5-unikey VERSION 5.1.7)

set(REQUIRED_FCITX_VERSION 5.1.13)
find_package(ECM 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

option(ENABLE_QT "Enable Qt based macro editor" On)
option(ENABLE_TEST "Build Test" On)
option(ENABLE_COVERAGE "Build the project with gcov support (Need ENABLE_TEST=On)" Off)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core ${REQUIRED_FCITX_VERSION} REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS TestFrontend)
find_package(Gettext REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-unikey\")
add_definitions(-DQT_NO_KEYWORDS)
fcitx5_add_i18n_definition()

if (ENABLE_QT)
  set(QT_MAJOR_VERSION 6)
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
  find_package(Fcitx5Qt${QT_MAJOR_VERSION}WidgetsAddons 5.0.12 REQUIRED)
  add_subdirectory(macro-editor)
  add_subdirectory(keymap-editor)
endif (ENABLE_QT)

add_subdirectory(po)
add_subdirectory(unikey)
add_subdirectory(src)
add_subdirectory(data)

if (ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)

    if (ENABLE_COVERAGE)
        add_custom_target(coverage
                        COMMAND "${CMAKE_CTEST_COMMAND}"
                        COMMAND lcov --gcov-tool "${GCOV_TOOL}" --no-external --capture --directory ./ -b "${CMAKE_CURRENT_SOURCE_DIR}" --output-file coverage.info
                        COMMAND genhtml coverage.info --output-directory "coverage_pages"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif ()

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Unikey.metainfo.xml.in
    org.fcitx.Fcitx5.Addon.Unikey.metainfo.xml XML)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Unikey.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

