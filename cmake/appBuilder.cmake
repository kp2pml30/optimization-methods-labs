macro(appBuilder name)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)

	file(GLOB_RECURSE SRC "*.cpp")
	file(GLOB_RECURSE UI "*.ui")
	file(GLOB_RECURSE RES "*.qrc")

	add_executable(${name}
		${SRC}
		${UI}
		${RES}
	)

	# target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/include/opt-methods/widgets")

	linkGLibC(${name})
	target_link_libraries(${name} PRIVATE opt-methods)
	# target_link_libraries(${name} PRIVATE opt-methods-widgets)
	target_link_libraries(${name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)
	install(TARGETS ${name} DESTINATION ./bin)
endmacro()

