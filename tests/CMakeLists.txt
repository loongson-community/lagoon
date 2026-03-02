add_executable(test_smoke test_smoke.c ../lagoon.c)
target_include_directories(test_smoke PRIVATE .. ../include)

add_test(
    NAME test_smoke
    COMMAND test_smoke
)

add_executable(test_is_store test_is_store.c ../lagoon.c)
target_include_directories(test_is_store PRIVATE .. ../include)

add_test(
    NAME test_is_store
    COMMAND test_is_store
)

add_executable(test_load_immediate test_load_immediate.c ../lagoon.c)
target_include_directories(test_load_immediate PRIVATE .. ../include)

add_test(
    NAME test_load_immediate
    COMMAND test_load_immediate
)

add_executable(test_label test_label.c ../lagoon.c)
target_include_directories(test_label PRIVATE .. ../include)

add_test(
    NAME test_label
    COMMAND test_label
)
