macro(helperBuilder name)
	file(GLOB_RECURSE SRC "*.cpp")

	add_executable(${name}
		${SRC}
	)
	linkGLibC(${name})
	target_link_libraries(${name} PRIVATE opt-methods)
endmacro()

