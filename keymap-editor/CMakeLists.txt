

set(KEYMAP_EDITOR_SRCS
    main.cpp
    editor.cpp
    model.cpp
    actions.cpp
    )

add_library(fcitx5-unikey-keymap-editor
  MODULE ${KEYMAP_EDITOR_SRCS})
set_target_properties(fcitx5-unikey-keymap-editor PROPERTIES
                      AUTOMOC TRUE
                      AUTOUIC TRUE
                      AUTOUIC_OPTIONS "-tr=fcitx::tr2fcitx;--include=fcitxqti18nhelper.h"
)
target_link_libraries(fcitx5-unikey-keymap-editor
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Widgets
    Fcitx5Qt${QT_MAJOR_VERSION}::WidgetsAddons
    unikey-lib
    )

install(TARGETS fcitx5-unikey-keymap-editor DESTINATION ${CMAKE_INSTALL_LIBDIR}/fcitx5/qt${QT_MAJOR_VERSION})

