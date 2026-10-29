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
