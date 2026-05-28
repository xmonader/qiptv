cmake_minimum_required(VERSION 3.20)

project(kiptv VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# extra-cmake-modules
find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Test)
find_package(KF6 6.0 REQUIRED COMPONENTS
    CoreAddons
    XmlGui
    Config
    ConfigWidgets
    I18n
    Notifications
    GlobalAccel
    KIO
    WidgetsAddons
    IconThemes
)

# libmpv
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

enable_testing()

# Source lists grow per phase. kiptv_core holds testable, UI-free logic.
set(KIPTV_CORE_SOURCES
    src/core/parse.cpp
    src/core/imageloader.cpp
    src/api/urlbuilder.cpp
    src/api/xtreamclient.cpp
    src/models/category.cpp
    src/models/serverinfo.cpp
    src/models/livestream.cpp
    src/models/vodstream.cpp
    src/models/series.cpp
    src/config/profilestore.cpp
    src/config/settings.cpp
)

add_library(kiptv_core STATIC ${KIPTV_CORE_SOURCES})
target_include_directories(kiptv_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(kiptv_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    KF6::I18n
    KF6::ConfigCore
)

set(KIPTV_APP_SOURCES
    src/main.cpp
    src/app/mainwindow.cpp
    src/auth/loginwidget.cpp
    src/browse/browsewidget.cpp
    src/browse/seriesdetail.cpp
    src/player/mpvwidget.cpp
    src/player/playerwidget.cpp
    src/kiptv.qrc
)

add_executable(kiptv ${KIPTV_APP_SOURCES})
target_link_libraries(kiptv PRIVATE
    kiptv_core
    Qt6::Widgets
    KF6::CoreAddons
    KF6::XmlGui
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ConfigWidgets
    KF6::I18n
    KF6::Notifications
    KF6::GlobalAccel
    KF6::KIOCore
    KF6::WidgetsAddons
    KF6::IconThemes
    PkgConfig::MPV
)

install(TARGETS kiptv ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES src/kiptvui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kiptv)
install(FILES src/kiptv.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

add_subdirectory(tests)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
