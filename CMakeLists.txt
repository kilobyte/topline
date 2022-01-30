cmake_minimum_required(VERSION 2.8.12)
project(topline C)

set (CMAKE_C_FLAGS "-Wall -Werror=format-security ${CMAKE_C_FLAGS}")

add_executable(topline
	topline.c
	cpu.c
	disk.c
	signals.c
)

install(TARGETS topline RUNTIME DESTINATION bin)
install(FILES topline.1 DESTINATION share/man/man1)
