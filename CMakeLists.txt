cmake_minimum_required(VERSION 3.20)
project(sirkel VERSION 0.5.0)

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

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml Quick DBus Test Widgets)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Config ColorScheme CoreAddons GlobalAccel I18n KCMUtils WindowSystem)
find_package(KWin ${KWIN_MIN_VERSION} REQUIRED)

add_definitions(-DTRANSLATION_DOMAIN=\"sirkel\")

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
