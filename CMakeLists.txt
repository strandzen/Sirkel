# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.20)
project(sirkel VERSION 0.5.3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_MIN_VERSION "6.7.0")
set(KF_MIN_VERSION "6.10.0")
# The effect API is internal and changes between Plasma releases; 6.7 is the
# oldest version this has been built against.
set(KWIN_MIN_VERSION "6.7.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(KDEClangFormat)
include(KDEGitCommitHooks)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml Quick DBus Test Widgets)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Config ColorScheme CoreAddons GlobalAccel I18n KCMUtils WindowSystem)
find_package(KWin ${KWIN_MIN_VERSION} REQUIRED)

add_subdirectory(src)

# Compiled translations from po/<language>/*.po (see po/README.md).
ki18n_install(po)

# KDE's code style. `cmake --build build --target clang-format` formats
# everything; ECM writes the .clang-format file itself (not versioned), and
# the pre-commit hook refuses commits whose changes aren't formatted.
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h autotests/*.cpp)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
