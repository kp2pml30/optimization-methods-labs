cmake_minimum_required(VERSION 3.5)

add_subdirectory(lib)

file(GLOB APPS LIST_DIRECTORIES true "app*")
foreach(APP ${APPS})
	add_subdirectory("${APP}")
endforeach()

file(GLOB APPS LIST_DIRECTORIES true "helper*")
foreach(APP ${APPS})
	add_subdirectory("${APP}")
endforeach()

