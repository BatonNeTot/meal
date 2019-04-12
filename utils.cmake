macro(create_meal_library)
	file(GLOB_RECURSE PROJECT_SOURCES
	        CONFIGURE_DEPENDS
	        ${PROJECT_SOURCE_DIR}/src/*.c)


	if (NOT "${PROJECT_SOURCES}" STREQUAL "")
		if (NOT TARGET ${PROJECT_NAME})
			add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
			add_library(meal::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
		endif()

		target_include_directories(${PROJECT_NAME} 
								PRIVATE ${PROJECT_SOURCE_DIR}/src
								PUBLIC ${PROJECT_SOURCE_DIR}/include)
	else()
		if (NOT TARGET ${PROJECT_NAME})
			add_library(${PROJECT_NAME} INTERFACE)
			add_library(meal::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
		endif()

		target_include_directories(${PROJECT_NAME} 
						        INTERFACE ${PROJECT_SOURCE_DIR}/include)
	endif()

	foreach(LIBRARY IN ITEMS ${ARGN})
		if (NOT (
			"${LIBRARY}" STREQUAL "PUBLIC" OR
			"${LIBRARY}" STREQUAL "PRIVATE" OR
			"${LIBRARY}" STREQUAL "INTERFACE"))
			list(APPEND ${PROJECT_NAME}_PROJECT_LIBRARIES ${LIBRARY})
			add_subdirectory(${PROJECT_SOURCE_DIR}/../${LIBRARY} sub-build/${LIBRARY})
		endif()
	endforeach()

	if (NOT "${${PROJECT_NAME}_PROJECT_LIBRARIES}" STREQUAL "")
		add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_PROJECT_LIBRARIES})
		target_link_libraries(${PROJECT_NAME} ${ARGN})
	endif()

endmacro()

macro(create_meal_executable)
	file(GLOB_RECURSE PROJECT_SOURCES
	        CONFIGURE_DEPENDS
	        ${PROJECT_SOURCE_DIR}/src/*.c)


	add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

	target_include_directories(${PROJECT_NAME} 
							PRIVATE ${PROJECT_SOURCE_DIR}/src
							PUBLIC ${PROJECT_SOURCE_DIR}/include)

	foreach(LIBRARY IN ITEMS ${ARGN})
		if (NOT (
			"${LIBRARY}" STREQUAL "PUBLIC" OR
			"${LIBRARY}" STREQUAL "PRIVATE" OR
			"${LIBRARY}" STREQUAL "INTERFACE"))
			list(APPEND ${PROJECT_NAME}_PROJECT_LIBRARIES ${LIBRARY})
			add_subdirectory(${PROJECT_SOURCE_DIR}/../${LIBRARY} sub-build/${LIBRARY})
		endif()
	endforeach()

	if (NOT "${${PROJECT_NAME}_PROJECT_LIBRARIES}" STREQUAL "")
		add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_PROJECT_LIBRARIES})
		target_link_libraries(${PROJECT_NAME} ${ARGN})
	endif()

endmacro()