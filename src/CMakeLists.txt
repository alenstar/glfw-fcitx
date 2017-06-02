# Build steps
#file(GLOB_RECURSE APP_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c)
set(APP_SRC_FILES 
	main.c
	SDL_dbus.c
	SDL_fcitx.c
	SDL_ime.c
	)

include_directories(${PROJECT_SOURCE_DIR}/ext)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/ext/glfw/include)
include_directories(/usr/lib/x86_64-linux-gnu/dbus-1.0/include)
include_directories(/usr/include/dbus-1.0)
#add_dependencies(${APP_NAME} ${APP_LIB_NAME})
#add_dependencies(${APP_NAME} ${ly_lib_name})
add_executable(${APP_NAME} ${APP_SRC_FILES})
add_dependencies(${APP_NAME} glfw)
#target_include_directories(${APP_NAME} PUBLIC ../src)
target_link_libraries(${APP_NAME} LINK_PUBLIC glfw m c pthread dbus-1 dl GL GLU)
#install(TARGETS ${APP_NAME} RUNTIME DESTINATION bin)
