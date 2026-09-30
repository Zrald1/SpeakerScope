cmake_minimum_required(VERSION 3.26)
project(SpeakerScope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SPEAKERSCOPE_BUILD_TESTS "Build tests" ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /MP)
endif()

find_package(IXWebSocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
# miniaudio is a single-header lib installed by vcpkg into include/ (no CMake target needed)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stt/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/diar/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/align/*.cpp"
)
# SessionController has no UI deps — the headless sim tool links it too.
list(APPEND CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/app/SessionController.cpp")

add_library(speakerscope_core STATIC ${CORE_SOURCES})
target_include_directories(speakerscope_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(speakerscope_core
    PUBLIC
        ixwebsocket::ixwebsocket
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)
if(WIN32)
    target_link_libraries(speakerscope_core PUBLIC bcrypt) # mbedtls entropy
endif()

file(GLOB_RECURSE UI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp"
)
list(APPEND UI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/app/Application.cpp")

add_executable(speakerscope ${UI_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(speakerscope
    PRIVATE
        speakerscope_core
        imgui::imgui
        glfw
        OpenGL::GL
)

if(WIN32)
    target_compile_definitions(speakerscope PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(speakerscope_core PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

# Headless pipeline simulator: streams a wav through AssemblyAI for real,
# optionally replaying gold diarization labels.
add_executable(speakerscope_sim "${CMAKE_CURRENT_SOURCE_DIR}/src/sim/sim_main.cpp")
target_link_libraries(speakerscope_sim PRIVATE speakerscope_core)

if(SPEAKERSCOPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
