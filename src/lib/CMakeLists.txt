cmake_minimum_required(VERSION 3.5)

file(GLOB_RECURSE SRC "*.cpp")

add_library(opt-methods STATIC ${SRC})

find_library(PThread pthread)
if (PThread)
	target_link_libraries(opt-methods PUBLIC  ${PThread})
endif()

target_link_libraries(opt-methods PUBLIC Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)
target_link_libraries(opt-methods PUBLIC -static-libgcc -static-libstdc++)

