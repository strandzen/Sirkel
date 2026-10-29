# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

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
# Translation domains: "sirkel" for the effect and the shared code,
# the plugin id for the settings (KCMUtils looks up the settings QML's
# strings under the plugin id). Messages.sh extracts one .pot per domain.
target_compile_definitions(sirkel_core PRIVATE TRANSLATION_DOMAIN="sirkel")

kcoreaddons_add_plugin(kwin_effect_sirkel
    SOURCES
        sirkeleffect.cpp
        hyperkeytrigger.cpp
        globalshortcuts.cpp
        overlaycontroller.cpp
        colorresolver.cpp
        ${sirkel_kcfg_SRCS}
    INSTALL_NAMESPACE "kwin/effects/plugins"
)

target_include_directories(kwin_effect_sirkel PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(kwin_effect_sirkel PRIVATE TRANSLATION_DOMAIN="sirkel")
# KWIN_EFFECT_FACTORY declares the plugin's QObject; without this AutoMoc
# warns that sirkeleffect.cpp includes a moc file it sees no reason for.
set_property(TARGET kwin_effect_sirkel APPEND PROPERTY AUTOMOC_MACRO_NAMES KWIN_EFFECT_FACTORY)

# Logging categories, switchable in KDebugSettings (kdebugsettings) or with
# QT_LOGGING_RULES="sirkel*.debug=true". Warnings are on by default.
ecm_qt_declare_logging_category(kwin_effect_sirkel
    HEADER sirkel_debug.h
    IDENTIFIER SIRKEL
    CATEGORY_NAME sirkel
    DESCRIPTION "Sirkel window snapping (KWin effect)"
    DEFAULT_SEVERITY Warning
    EXPORT SIRKEL
)

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
    Qt6::DBus
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

add_subdirectory(rebuild)
add_subdirectory(kcm)

ecm_qt_install_logging_categories(
    EXPORT SIRKEL
    FILE sirkel.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

# For software centers (Discover). Validated by appstreamtest after an install.
install(FILES io.github.strandzen.sirkel.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# Settings migrations, run once per user by kconf_update at Plasma login.
install(FILES update/sirkel.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(PROGRAMS update/sirkel-2026-10-cleanup.sh update/sirkel-0.5-cleanup.sh update/sirkel-2026-10-26-cleanup.sh DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
