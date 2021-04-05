macro(linkGLibC targ)
	target_link_libraries(${targ} PUBLIC -static-libgcc -static-libstdc++)
endmacro()

