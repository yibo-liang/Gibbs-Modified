cmake_minimum_required(VERSION 3.5)
project(PLDA)

##BOOST

set(Boost_INCLUDE_DIR /usr/include/)
FIND_PACKAGE(Boost COMPONENTS program_options serialization REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

#MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES
    corpus.cpp
    corpus.h
    document.cpp
    document.h
    job.cpp
    job.h
    job_config.cpp
    job_config.h
    json.hpp
    main.cpp
    model.cpp
    model.h
    mpi_helper.h
    shared_header.h
    task.cpp
    task.h
    task_executor.cpp
    task_executor.h)

add_executable(PLDA ${SOURCE_FILES})


target_link_libraries(PLDA ${MPI_LIBRARIES})
target_link_libraries(PLDA ${Boost_LIBRARIES})


if(MPI_COMPILE_FLAGS)
    set_target_properties(PLDA PROPERTIES
            COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(PLDA PROPERTIES
            LINK_FLAGS "${MPI_LINK_FLAGS}")

endif()

