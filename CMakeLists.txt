cmake_minimum_required(VERSION 3.5)

project(opt-methods-lab LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if (MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /permissive-")
else()
	# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wall -Wextra")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Charts REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Charts REQUIRED)

macro(appBuilder name)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)

	file(GLOB_RECURSE SRC "*.cpp")

	add_executable(${name}
		${SRC}
		mainwindow.ui
	)

	target_link_libraries(${name} PRIVATE opt-methods)
	target_link_libraries(${name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)
	target_link_libraries(${name} PUBLIC -static-libgcc -static-libstdc++)
	install(TARGETS ${name})
endmacro()

macro(helperBuilder name)
	file(GLOB_RECURSE SRC "*.cpp")

	add_executable(${name}
		${SRC}
	)
	target_link_libraries(${name} PRIVATE opt-methods)
endmacro()

add_subdirectory(src)

### cpack
set(CPACK_PACKAGE_NAME "OptimizationMethods")
set(CPACK_PACKAGE_VENDOR "kp2dp5nh5")
set(CPACK_VERBATIM_VARIABLES true)
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt5-default, libqt5charts5")
include(CPack)
