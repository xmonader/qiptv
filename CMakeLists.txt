cmake_minimum_required(VERSION 3.20)

project(qiptv VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Test OpenGLWidgets)

# libmpv
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

# Strict string handling (previously provided by KDECompilerSettings).
add_compile_definitions(QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_BYTEARRAY)

enable_testing()

# Source lists grow per phase. qiptv_core holds testable, UI-free logic.
set(QIPTV_CORE_SOURCES
    src/core/parse.cpp
    src/core/imageloader.cpp
    src/core/playlist.cpp
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

add_library(qiptv_core STATIC ${QIPTV_CORE_SOURCES})
target_include_directories(qiptv_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(qiptv_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
)

set(QIPTV_APP_SOURCES
    src/main.cpp
    src/app/mainwindow.cpp
    src/app/settingsdialog.cpp
    src/auth/loginwidget.cpp
    src/browse/browsewidget.cpp
    src/browse/seriesdetail.cpp
    src/player/mpvwidget.cpp
    src/player/playerwidget.cpp
)

add_executable(qiptv ${QIPTV_APP_SOURCES})
target_link_libraries(qiptv PRIVATE
    qiptv_core
    Qt6::Widgets
    Qt6::OpenGLWidgets
    PkgConfig::MPV
)

install(TARGETS qiptv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/io.github.xmonader.qiptv.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/io.github.xmonader.qiptv.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES data/icons/io.github.xmonader.qiptv.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

add_subdirectory(tests)

