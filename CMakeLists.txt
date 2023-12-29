cmake_minimum_required(VERSION 3.0.0)
project(demo)

set(CMAKE_CXX_STANDARD 11)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories (${PROJECT_SOURCE_DIR}/include 
                    ${PROJECT_SOURCE_DIR}/include/taskSchedule
                    )

aux_source_directory (src SRC_LIST)

aux_source_directory (src/taskSchedule SRC_TASK_SCHEDULE_LIST)

link_directories(/usr/local/lib
                )

add_executable (main ${SRC_LIST} ${SRC_TASK_SCHEDULE_LIST})

target_link_libraries(main -lpthread -ljsoncpp)

