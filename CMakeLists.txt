project(kwayland-integration)
set(PROJECT_VERSION "5.13.4")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.24.0")

find_package(ECM 0.0.11 REQUIRED NO_MODULE)

include(FeatureSummary)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})


find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

# required frameworks
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    IdleTime
    WindowSystem
)
find_package(KF5Wayland CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(autotests)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
