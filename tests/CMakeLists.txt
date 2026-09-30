find_package(GTest CONFIG REQUIRED)

add_executable(speakerscope_tests
    test_speaker_timeline.cpp
    test_attributor.cpp
    test_transcript_buffer.cpp
    test_resolver.cpp
    test_turn_path.cpp
    test_turn_battery.cpp
)

target_link_libraries(speakerscope_tests
    PRIVATE
        speakerscope_core
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(speakerscope_tests)
