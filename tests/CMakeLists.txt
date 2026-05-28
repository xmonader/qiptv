# Tests link against kiptv_core (UI-free logic). Each phase adds cases here.

# Relax the strict implicit-cast guards for test code so fixtures can use bare
# string/byte-array literals. Production code keeps the guards (set globally by
# KDECompilerSettings).
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

add_executable(test_parse test_parse.cpp)
target_link_libraries(test_parse PRIVATE kiptv_core Qt6::Test)
add_test(NAME parse COMMAND test_parse)

add_executable(test_models test_models.cpp)
target_link_libraries(test_models PRIVATE kiptv_core Qt6::Test)
add_test(NAME models COMMAND test_models)

add_executable(test_urlbuilder test_urlbuilder.cpp)
target_link_libraries(test_urlbuilder PRIVATE kiptv_core Qt6::Test)
add_test(NAME urlbuilder COMMAND test_urlbuilder)

add_executable(test_xtreamclient test_xtreamclient.cpp)
target_link_libraries(test_xtreamclient PRIVATE kiptv_core Qt6::Test Qt6::Network)
add_test(NAME xtreamclient COMMAND test_xtreamclient)

add_executable(test_profilestore test_profilestore.cpp)
target_link_libraries(test_profilestore PRIVATE kiptv_core Qt6::Test)
add_test(NAME profilestore COMMAND test_profilestore)
