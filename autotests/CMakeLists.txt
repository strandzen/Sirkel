# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

ecm_add_tests(
    placementtest.cpp
    directioninputtest.cpp
    shortcutactionstest.cpp
    LINK_LIBRARIES sirkel_core Qt6::Test
)

ecm_add_test(rebuildtest.cpp
    TEST_NAME rebuildtest
    LINK_LIBRARIES sirkel_rebuild_core Qt6::Test
)

# The settings module, loaded as KDE loads it (Apply/Defaults states).
ecm_add_test(kcmapplytest.cpp
    TEST_NAME kcmapplytest
    LINK_LIBRARIES Qt6::Test Qt6::Gui KF6::CoreAddons KF6::KCMUtilsQuick
)
target_compile_definitions(kcmapplytest PRIVATE SIRKEL_KCM_PLUGIN="$<TARGET_FILE:kwin_effect_sirkel_config>")
add_dependencies(kcmapplytest kwin_effect_sirkel_config)
set_tests_properties(kcmapplytest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
