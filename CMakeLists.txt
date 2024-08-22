cmake_minimum_required(VERSION 3.14)
project(vulkan VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED) 
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

file(GLOB vulkanSource CONFIGURE_DEPENDS "*.cpp" "*.h" "*.hpp" "header/*.h" "source/*.cpp" "header/*.hpp")

add_executable(vulkan ${vulkanSource})
target_include_directories(vulkan PRIVATE source header)
target_link_libraries(vulkan PRIVATE imgui::imgui)
target_link_libraries(vulkan PRIVATE glfw)
target_link_libraries(vulkan PRIVATE Vulkan::Vulkan)
target_link_libraries(vulkan PRIVATE glm::glm)
target_link_libraries(vulkan PRIVATE tinyobjloader::tinyobjloader)
target_include_directories(vulkan PRIVATE ${Stb_INCLUDE_DIR})