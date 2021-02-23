cmake_minimum_required(VERSION 3.5)

project(opt-methods-lab1 LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXCompilerFlag)
# unset(supports_fast_math CACHE)
check_cxx_compiler_flag(-ffast-math supports_fast_math)
if (supports_fast_math)
	add_compile_options(-ffast-math)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")

include_directories(${CMAKE_SOURCE_DIR}/include)

macro(appBuilder name)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

	file(GLOB_RECURSE SRC "*.cpp")

	add_executable(${name}
		${SRC}
		mainwindow.ui
	)

	target_link_libraries(${name} PRIVATE opt-methods)
	target_link_libraries(${name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
endmacro()

add_subdirectory(src)

