cmake_minimum_required(VERSION 3.20)

get_property(UIAPI_LOCATION GLOBAL PROPERTY uiapi_location)
message(CMAKE_BINARY_DIR="${UIAPI_LOCATION}")

add_executable(
	ui WIN32
	main.cpp
	plot_layer.cpp
	plot_layer.h
	fm_synth.cpp
	fm_synth.h
	fm_synth_win_resource.rc
	fm_synth_win_resource.h
	oscillator.cpp
	oscillator.h
	envelope.cpp
	envelope.h
	midi_manager.cpp
	midi_manager.h
	rasterizer.cpp
	rasterizer.h
	cube_render.cpp
	cube_render.h
	wavefront_loader.cpp
	wavefront_loader.h
	asset.cpp
	asset.h
	auxiliary.cpp
	auxiliary.h
	bitmap.cpp
	bitmap.h
	)
add_definitions(-DUNICODE)
set_property(TARGET ui PROPERTY CXX_STANDARD 23)

target_include_directories(ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(ui uiapi)
target_link_libraries(ui dx12api)
target_link_libraries(ui Winmm.lib)

add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/plot.hlsl ${CMAKE_CURRENT_BINARY_DIR}/main_src/plot.hlsl)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mono2d.hlsl ${CMAKE_CURRENT_BINARY_DIR}/main_src/mono2d.hlsl)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/cube.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/cube.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/cube_2.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/cube_2.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/cube_numbers_forward.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/cube_numbers_forward.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/cube_numbers.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/cube_numbers.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/sphere_2.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/sphere_2.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/sphere.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/sphere.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/test.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/test.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../mesh/crate.obj ${CMAKE_CURRENT_BINARY_DIR}/mesh/crate.obj)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../tex/crate_tex.bmp ${CMAKE_CURRENT_BINARY_DIR}/tex/crate_tex.bmp)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../tex/butterfly_tex.bmp ${CMAKE_CURRENT_BINARY_DIR}/tex/butterfly_tex.bmp)
add_custom_command(TARGET ui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../tex/depth_tex.bmp ${CMAKE_CURRENT_BINARY_DIR}/tex/depth_tex.bmp)

install(TARGETS ui DESTINATION ${CMAKE_SOURCE_DIR})