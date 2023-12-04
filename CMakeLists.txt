project(una)

#Specify supported version
cmake_minimum_required(VERSION 2.8.12)

# Collect the sources
add_executable(una
    src/main.c
    src/utils.c
    src/logger.c
)
