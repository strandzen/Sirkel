# Generated here as well: a custom command's outputs are only usable by
# targets in the directory that defines it, so reusing src/'s copy breaks
# clean (and parallel) builds.
kconfig_add_kcfg_files(kcm_kcfg_SRCS GENERATE_MOC ../sirkelconfig.kcfgc)

kcmutils_add_qml_kcm(kwin_effect_sirkel_config
    SOURCES
        sirkelconfigmodule.cpp
        ../colorresolver.cpp
        ${kcm_kcfg_SRCS}
    INSTALL_NAMESPACE "kwin/effects/configs"
    DISABLE_DESKTOP_FILE_GENERATION
)

target_include_directories(kwin_effect_sirkel_config PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(kwin_effect_sirkel_config PRIVATE
    sirkel_core
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ColorScheme
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
)
