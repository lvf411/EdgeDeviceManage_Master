cmake_minimum_required(VERSION 3.0.0)
project(demo)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

aux_source_directory (src SRC_LIST)

aux_source_directory (src/taskSchedule SRC_TASK_SCHEDULE)

include_directories (include
                    )

link_directories(usr/local/lib
                )

add_executable (main ${SRC_LIST} ${SRC_TASK_SCHEDULE})

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

target_link_libraries(main -lpthread -ljsoncpp)
