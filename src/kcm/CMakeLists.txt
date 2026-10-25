# Generated here as well: a custom command's outputs are only usable by
# targets in the directory that defines it, so reusing src/'s copy breaks
# clean (and parallel) builds.
kconfig_add_kcfg_files(kcm_kcfg_SRCS GENERATE_MOC ../sirkelconfig.kcfgc)

# The plugin version comes from project().
configure_file(kwin_effect_sirkel_config.json.in ${CMAKE_CURRENT_BINARY_DIR}/kwin_effect_sirkel_config.json @ONLY)

kcmutils_add_qml_kcm(kwin_effect_sirkel_config
    SOURCES
        sirkelconfigmodule.cpp
        ../colorresolver.cpp
        ${kcm_kcfg_SRCS}
    INSTALL_NAMESPACE "kwin/effects/configs"
    DISABLE_DESKTOP_FILE_GENERATION
)

# The effect's overlay QML, bundled next to the KCM's own files so the live
# preview uses the exact same Outline/Indicator as the real thing.
qt6_add_resources(kwin_effect_sirkel_config sirkel_preview_qml
    PREFIX "/kcm/kwin_effect_sirkel_config"
    BASE "../qml"
    FILES
        ../qml/Outline.qml
        ../qml/Indicator.qml
        ../qml/Overlay.qml
)

# KWin loads an effect only if it was built for its exact version; the About
# page compares this against the installed KWin.
target_compile_definitions(kwin_effect_sirkel_config PRIVATE SIRKEL_KWIN_VERSION="${KWin_VERSION}")

target_include_directories(kwin_effect_sirkel_config PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(kwin_effect_sirkel_config PRIVATE
    sirkel_core
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Widgets
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ColorScheme
    KF6::CoreAddons
    KF6::GlobalAccel
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
)

# Launcher entry: opens the same settings in their own window.
install(FILES org.kde.sirkel.settings.desktop DESTINATION ${KDE_INSTALL_APPDIR})
