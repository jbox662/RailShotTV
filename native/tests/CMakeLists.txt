cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Test Core Gui Widgets)

function(railshot_add_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE
        railshot_core
        railshot_scoreboard
        railshot_schedule
        Qt6::Test
        Qt6::Core
    )
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

railshot_add_test(test_project unit/test_project.cpp)
railshot_add_test(test_json_schema unit/test_json_schema.cpp)
railshot_add_test(test_scoreboard unit/test_scoreboard.cpp)
railshot_add_test(test_schedule unit/test_schedule.cpp)
railshot_add_test(test_audio_meter unit/test_audio_meter.cpp)

# Audio meter test needs audio lib
target_link_libraries(test_audio_meter PRIVATE railshot_audio)

# Integration-style tests (engine lifecycle without GUI)
add_executable(test_engine_lifecycle integration/test_engine_lifecycle.cpp)
target_link_libraries(test_engine_lifecycle PRIVATE
    railshot_engine
    Qt6::Test
)
target_include_directories(test_engine_lifecycle PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_engine_lifecycle COMMAND test_engine_lifecycle)

add_executable(test_mkv_recording integration/test_mkv_recording.cpp)
target_link_libraries(test_mkv_recording PRIVATE
    railshot_encoding
    railshot_recording
    Qt6::Test
    Qt6::Gui
)
target_include_directories(test_mkv_recording PRIVATE ${CMAKE_SOURCE_DIR}/src)
railshot_link_ffmpeg(test_mkv_recording)
add_test(NAME test_mkv_recording COMMAND test_mkv_recording)

add_executable(test_rtmp_flv integration/test_rtmp_flv.cpp)
target_link_libraries(test_rtmp_flv PRIVATE
    railshot_encoding
    railshot_streaming
    railshot_output
    Qt6::Test
    Qt6::Gui
)
target_include_directories(test_rtmp_flv PRIVATE ${CMAKE_SOURCE_DIR}/src)
railshot_link_ffmpeg(test_rtmp_flv)
add_test(NAME test_rtmp_flv COMMAND test_rtmp_flv)

add_executable(test_mf_encoder integration/test_mf_encoder.cpp)
target_link_libraries(test_mf_encoder PRIVATE
    railshot_encoding
    Qt6::Test
    Qt6::Gui
)
target_include_directories(test_mf_encoder PRIVATE ${CMAKE_SOURCE_DIR}/src)
railshot_link_ffmpeg(test_mf_encoder)
add_test(NAME test_mf_encoder COMMAND test_mf_encoder)

add_executable(test_creator_features integration/test_creator_features.cpp)
target_link_libraries(test_creator_features PRIVATE
    railshot_capture
    railshot_overlays
    railshot_encoding
    Qt6::Test
    Qt6::Gui
)
target_include_directories(test_creator_features PRIVATE ${CMAKE_SOURCE_DIR}/src)
railshot_link_ffmpeg(test_creator_features)
add_test(NAME test_creator_features COMMAND test_creator_features)

add_executable(test_phase5 integration/test_phase5.cpp)
target_link_libraries(test_phase5 PRIVATE
    railshot_chat
    Qt6::Test
    Qt6::Core
)
target_include_directories(test_phase5 PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_phase5 COMMAND test_phase5)
