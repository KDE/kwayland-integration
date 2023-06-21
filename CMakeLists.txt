project(kwayland-integration)

set(CMAKE_C_STANDARD 99)

set(PROJECT_VERSION "5.27.80")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 3.16)
set(QT_MIN_VERSION "5.15.2")
set(KF_MIN_VERSION "5.102.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} )

include(FeatureSummary)

include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)

set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)

option(BUILD_QT5 "Build for Qt5" ON)
option(BUILD_QT6 "Build for Qt6" ON)


#CI is stupid and doesn't allow us to set CMAKE options per build variant
if ($ENV{CI_JOB_NAME_SLUG} MATCHES "qt5")
    set(BUILD_QT5 ON)
    set(BUILD_QT6 OFF)
elseif($ENV{CI_JOB_NAME_SLUG} MATCHES "qt6")
    set(BUILD_QT5 OFF)
    set(BUILD_QT6 ON)
endif()

find_package(WaylandProtocols 1.21 REQUIRED)
find_package(Wayland 1.15 COMPONENTS Client)
find_package(PkgConfig REQUIRED)
find_package(PlasmaWaylandProtocols REQUIRED)

ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.101
)

set(KDE_SKIP_TEST_SETTINGS ON)

function(buildsrc QT_MAJOR_VERSION)
    include(KDEInstallDirs${QT_MAJOR_VERSION})
    include(KDECMakeSettings)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
        Core
        Widgets
        WaylandClient
    )
    # https://bugreports.qt.io/browse/QTBUG-114706
    add_library(Qt::Core ALIAS Qt${QT_MAJOR_VERSION}::Core)
    add_library(Qt::CorePrivate ALIAS Qt${QT_MAJOR_VERSION}::CorePrivate)
    add_library(Qt::Gui ALIAS Qt${QT_MAJOR_VERSION}::Gui)
    add_library(Qt::GuiPrivate ALIAS Qt${QT_MAJOR_VERSION}::GuiPrivate)

    # required frameworks
    find_package(KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS
        WindowSystem
        Wayland
    )

    if (QT_MAJOR_VERSION EQUAL "5")
        find_package(QtWaylandScanner ${QT_MIN_VERSION} REQUIRED)
        find_package(Qt5XkbCommonSupport REQUIRED) # Needed by Qt5::WaylandClientPrivate
    endif()
    add_subdirectory(src src${QT_MAJOR_VERSION})
endfunction()

if (${BUILD_QT5})
    unset(QUERY_EXECUTABLE CACHE)
    buildsrc(5)
endif()
if (${BUILD_QT6})
    unset(QUERY_EXECUTABLE CACHE)
    buildsrc(6)
endif()


# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
