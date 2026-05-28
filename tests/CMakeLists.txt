# Tests link against kiptv_core (UI-free logic). Each phase adds cases here.

add_executable(test_parse test_parse.cpp)
target_link_libraries(test_parse PRIVATE kiptv_core Qt6::Test)
add_test(NAME parse COMMAND test_parse)
