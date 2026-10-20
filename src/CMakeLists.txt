kconfig_add_kcfg_files(sirkel_kcfg_SRCS GENERATE_MOC sirkelconfig.kcfgc)

# The plugin version comes from project() above.
configure_file(metadata.json.in ${CMAKE_CURRENT_BINARY_DIR}/metadata.json @ONLY)

# Pure logic shared by the effect, the KCM and the tests.
add_library(sirkel_core STATIC
    placement.cpp
    directioninput.cpp
    shortcutactions.cpp
)
set_target_properties(sirkel_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(sirkel_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sirkel_core PUBLIC Qt6::Core Qt6::Gui KF6::I18n)

kcoreaddons_add_plugin(kwin_effect_sirkel
    SOURCES
        sirkeleffect.cpp
        hyperkeytrigger.cpp
        overlaycontroller.cpp
        colorresolver.cpp
        ${sirkel_kcfg_SRCS}
    INSTALL_NAMESPACE "kwin/effects/plugins"
)

target_include_directories(kwin_effect_sirkel PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt6_add_resources(kwin_effect_sirkel sirkel_qml
    PREFIX "/qml"
    BASE "qml"
    FILES
        qml/Outline.qml
        qml/Indicator.qml
        qml/Overlay.qml
)

target_link_libraries(kwin_effect_sirkel PRIVATE
    sirkel_core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ColorScheme
    KF6::CoreAddons
    KF6::GlobalAccel
    KF6::I18n
    KF6::WindowSystem
    KWin::kwin
)

add_subdirectory(kcm)

# Settings migrations, run once per user by kconf_update at Plasma login.
install(FILES update/sirkel.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(PROGRAMS update/sirkel-2026-10-cleanup.sh update/sirkel-0.5-cleanup.sh DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
