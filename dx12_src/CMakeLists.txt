cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
	DirectX-Headers
	GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers
	GIT_TAG 64c650b0c6642ab0f3cdf674e38401e02ccd2460 # version 1.618.1
	)
FetchContent_MakeAvailable(DirectX-Headers)

set(DX12API_LOCATION "${CMAKE_BINARY_DIR}/src/Debug")
set_property(GLOBAL PROPERTY dx12api_location "${DX12API_LOCATION}")

add_library(dx12api STATIC
	dx12_window.cpp
	dx12_window.h
	dx12_layer.h
	commandqueue.cpp
	commandqueue.h
	root_signature.cpp
	root_signature.h
	shader.cpp
	shader.h
	pipeline_state.cpp
	pipeline_state.h
	depth_stencil.cpp
	depth_stencil.h
	resource.cpp
	resource.h
	)
add_definitions(-DUNICODE)
set_property(TARGET dx12api PROPERTY CXX_STANDARD 23)
target_link_libraries(dx12api PUBLIC DirectX-Headers)
target_link_libraries(dx12api PUBLIC uiapi)
target_link_libraries(dx12api PUBLIC d3d12.lib)
target_link_libraries(dx12api PUBLIC dxgi.lib)
target_link_libraries(dx12api PUBLIC dxguid.lib)
target_link_libraries(dx12api PUBLIC d3dcompiler.lib)
target_include_directories(dx12api SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
# install(TARGETS dx12api DESTINATION ${CMAKE_SOURCE_DIR})
