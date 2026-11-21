add_library(raven_pe_inspect_cli STATIC cli.cpp)
target_include_directories(raven_pe_inspect_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(raven_pe_inspect_cli PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

add_executable(pe_inspect_cli_test cli_test.cpp)
target_link_libraries(pe_inspect_cli_test PRIVATE raven_pe_inspect_cli)
target_compile_options(pe_inspect_cli_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
add_test(NAME pe_inspector_cli_contracts COMMAND pe_inspect_cli_test)
set_tests_properties(pe_inspector_cli_contracts PROPERTIES TIMEOUT 10)
