# Tests link against qiptv_core (UI-free logic). Each phase adds cases here.

# Relax the strict implicit-cast guards for test code so fixtures can use bare
# string/byte-array literals. Production code keeps the guards (set globally by
# the top-level CMakeLists.txt).
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

add_executable(test_parse test_parse.cpp)
target_link_libraries(test_parse PRIVATE qiptv_core Qt6::Test)
add_test(NAME parse COMMAND test_parse)

add_executable(test_playlist test_playlist.cpp)
target_link_libraries(test_playlist PRIVATE qiptv_core Qt6::Test)
add_test(NAME playlist COMMAND test_playlist)

add_executable(test_models test_models.cpp)
target_link_libraries(test_models PRIVATE qiptv_core Qt6::Test)
add_test(NAME models COMMAND test_models)

add_executable(test_urlbuilder test_urlbuilder.cpp)
target_link_libraries(test_urlbuilder PRIVATE qiptv_core Qt6::Test)
add_test(NAME urlbuilder COMMAND test_urlbuilder)

add_executable(test_xtreamclient test_xtreamclient.cpp)
target_link_libraries(test_xtreamclient PRIVATE qiptv_core Qt6::Test Qt6::Network)
add_test(NAME xtreamclient COMMAND test_xtreamclient)

add_executable(test_profilestore test_profilestore.cpp)
target_link_libraries(test_profilestore PRIVATE qiptv_core Qt6::Test)
add_test(NAME profilestore COMMAND test_profilestore)

add_executable(test_settings test_settings.cpp)
target_link_libraries(test_settings PRIVATE qiptv_core Qt6::Test)
add_test(NAME settings COMMAND test_settings)

add_executable(test_imageloader test_imageloader.cpp)
target_link_libraries(test_imageloader PRIVATE qiptv_core Qt6::Test Qt6::Network Qt6::Gui)
add_test(NAME imageloader COMMAND test_imageloader)

# GUI test: compiles the widget directly and runs under the offscreen platform.
add_executable(test_loginwidget test_loginwidget.cpp ${CMAKE_SOURCE_DIR}/src/auth/loginwidget.cpp)
target_link_libraries(test_loginwidget PRIVATE qiptv_core Qt6::Test Qt6::Widgets)
add_test(NAME loginwidget COMMAND test_loginwidget)

add_executable(test_playerwidget test_playerwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/player/playerwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/player/mpvwidget.cpp)
target_link_libraries(test_playerwidget PRIVATE qiptv_core Qt6::Test Qt6::Widgets
    Qt6::OpenGLWidgets PkgConfig::MPV)
add_test(NAME playerwidget COMMAND test_playerwidget)

# GUI-touching tests construct a QGuiApplication/QApplication; run every test
# under the offscreen platform so the suite is headless-safe (CI has no
# display). Logic-only tests are unaffected by the platform choice.
set_tests_properties(parse playlist models urlbuilder xtreamclient profilestore settings imageloader loginwidget playerwidget
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
