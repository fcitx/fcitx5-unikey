

set(MACRO_EDITOR_SRCS
    model.cpp
    main.cpp
    editor.cpp
    dialog.cpp
    )

if (NOT ENABLE_QT)
    return()
endif (NOT ENABLE_QT)

add_library(fcitx5-unikey-macro-editor
  MODULE ${MACRO_EDITOR_SRCS})
set_target_properties(fcitx5-unikey-macro-editor PROPERTIES
                      AUTOMOC TRUE
                      AUTOUIC TRUE
                      AUTOUIC_OPTIONS "-tr=fcitx::tr2fcitx;--include=fcitxqti18nhelper.h"
)
target_link_libraries(fcitx5-unikey-macro-editor
    Qt5::Core
    Qt5::Widgets
    Fcitx5Qt5::WidgetsAddons
    unikey-lib
    )

install(TARGETS fcitx5-unikey-macro-editor DESTINATION ${CMAKE_INSTALL_LIBDIR}/fcitx5/qt5)
