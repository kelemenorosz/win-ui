cmake_minimum_required(VERSION 3.20)

set(UIAPI_LOCATION "${CMAKE_BINARY_DIR}/src/Debug")
set_property(GLOBAL PROPERTY uiapi_location "${UIAPI_LOCATION}")

add_library(uiapi STATIC
	main.cpp
	main.h
	threaded_window.cpp
	threaded_window.h
	control.cpp
	control.h
	button.cpp
	button.h
	trackbar.cpp
	trackbar.h
	static_text.cpp
	static_text.h
	combobox.cpp
	combobox.h
	)
add_definitions(-DUNICODE)
set_property(TARGET uiapi PROPERTY CXX_STANDARD 23)
target_include_directories(uiapi SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
# install(TARGETS uiapi DESTINATION ${CMAKE_SOURCE_DIR})
