macro(linkGLibC targ)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_libraries(${targ} PUBLIC -static-libgcc -static-libstdc++)
	endif()
endmacro()

