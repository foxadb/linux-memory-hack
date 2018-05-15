CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project (proc-memhack)

# Generate examples process
add_executable (hack src/hack.c)
add_executable (stringloop src/stringloop.c)