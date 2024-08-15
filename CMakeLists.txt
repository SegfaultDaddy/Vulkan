cmake_minimum_required(VERSION 3.14)
project(app VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB app_source CONFIGURE_DEPENDS "header/*.h" "source/*.cpp" "header/*.hpp")

add_executable(app ${app_source})
target_link_libraries(app PRIVATE imgui::imgui)
target_link_libraries(app PRIVATE glfw)
target_link_libraries(app PRIVATE Vulkan::Vulkan)
target_link_libraries(app PRIVATE glm::glm)
target_include_directories(app PRIVATE source header)