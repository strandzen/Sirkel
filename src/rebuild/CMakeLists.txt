# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Whether Sirkel needs a rebuild for the installed KWin, shared by the
# settings' About page, sirkel-rebuild and the tests. Doesn't link KWin, so
# it keeps working after the KWin update it checks for.
add_library(sirkel_rebuild_core STATIC rebuildstatus.cpp)
set_target_properties(sirkel_rebuild_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(sirkel_rebuild_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sirkel_rebuild_core PUBLIC Qt6::Core KF6::ConfigCore KF6::CoreAddons KF6::I18n)
# KWin only loads an effect built for its exact version. The library link
# (libkwin.so.6) points at the installed KWin's version. The package folder
# is where both PKGBUILDs' package() install themselves.
target_compile_definitions(sirkel_rebuild_core PRIVATE
    SIRKEL_KWIN_VERSION="${KWin_VERSION}"
    SIRKEL_KWIN_LIBRARY_LINK="$<TARGET_SONAME_FILE:KWin::kwin>"
    SIRKEL_REBUILD_DATADIR="${KDE_INSTALL_FULL_DATADIR}/sirkel/rebuild"
    SIRKEL_HAS_REBUILD_HELPER=$<BOOL:${BUILD_REBUILD_HELPER}>
    TRANSLATION_DOMAIN="sirkel"
)

if(NOT BUILD_REBUILD_HELPER)
    return()
endif()

add_executable(sirkel-rebuild main.cpp)
target_compile_definitions(sirkel-rebuild PRIVATE TRANSLATION_DOMAIN="sirkel")

ecm_qt_declare_logging_category(sirkel-rebuild
    HEADER sirkel_rebuild_debug.h
    IDENTIFIER SIRKEL_REBUILD
    CATEGORY_NAME sirkel.rebuild
    DESCRIPTION "Sirkel rebuild check and helper"
    DEFAULT_SEVERITY Warning
    EXPORT SIRKEL
)

target_link_libraries(sirkel-rebuild PRIVATE
    sirkel_rebuild_core
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    KF6::KIOGui
    KF6::Notifications
)

install(TARGETS sirkel-rebuild ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
# The login check, and the notification's entry in System Settings ->
# Notifications (where it can be muted).
install(FILES io.github.strandzen.sirkel.rebuild-check.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install(FILES sirkel.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
