cmake_minimum_required(VERSION 3.20)
project(MOD_ELITE VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/utf-8 /O2)
endif()

option(MODELITE_BUILD_APP "Construir o aplicativo (baixa GLFW e Dear ImGui via FetchContent)" ON)

# ---- Núcleo + rede (sem GPU; testado em Linux) ----
add_library(me_core STATIC
  src/core/settings.cpp src/core/stats.cpp src/core/session.cpp src/core/autorun.cpp
  src/core/protocol.cpp src/core/skin.cpp src/core/server.cpp src/net/ws_client.cpp)
target_include_directories(me_core PUBLIC src)
find_package(Threads REQUIRED)
target_link_libraries(me_core PUBLIC Threads::Threads)
if(WIN32)
  target_link_libraries(me_core PUBLIC ws2_32)
endif()

enable_testing()
add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests me_core)
add_test(NAME core_tests COMMAND core_tests)
add_executable(ws_probe tests/ws_probe.cpp)
target_link_libraries(ws_probe me_core)
add_test(NAME ws_rfc_vectors COMMAND ws_probe)
add_executable(bench_core tools/bench_core.cpp)
target_link_libraries(bench_core me_core)

# ---- Aplicativo (janela + interface) ----
if(MODELITE_BUILD_APP)
  include(FetchContent)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.0)
  FetchContent_MakeAvailable(glfw imgui)
  find_package(OpenGL REQUIRED)
  add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

  add_executable(MOD_ELITE src/app/main.cpp)
  target_link_libraries(MOD_ELITE PRIVATE me_core imgui_lib)
  set_target_properties(MOD_ELITE PROPERTIES OUTPUT_NAME "MOD_ELITE_1.0" WIN32_EXECUTABLE ON)
  if(MSVC)
    target_link_options(MOD_ELITE PRIVATE /ENTRY:mainCRTStartup)
  elseif(MINGW)
    target_link_options(MOD_ELITE PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
endif()
