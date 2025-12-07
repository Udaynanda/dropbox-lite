find_package(GTest)

if(NOT GTest_FOUND)
    message(WARNING "GTest not found. Skipping tests.")
    return()
endif()

add_executable(test_hash
    test_hash.cpp
)

target_link_libraries(test_hash
    dropbox_common
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME test_hash COMMAND test_hash)

add_executable(test_chunker
    test_chunker.cpp
)

target_link_libraries(test_chunker
    dropbox_common
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME test_chunker COMMAND test_chunker)
