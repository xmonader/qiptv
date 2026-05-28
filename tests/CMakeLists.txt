# Tests link against kiptv_core (UI-free logic). Each phase adds cases here.

add_executable(test_smoke test_smoke.cpp)
target_link_libraries(test_smoke PRIVATE kiptv_core Qt6::Test)
add_test(NAME smoke COMMAND test_smoke)
