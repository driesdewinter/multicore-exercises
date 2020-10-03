cmake_minimum_required (VERSION 2.6)

project (multicore-exercises)

enable_testing()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -Wall -Wextra -g -pthread")

include_directories(${PROJECT_SOURCE_DIR})

function(add_example name)
	add_executable(${name} ${name}.cpp)
endfunction()

add_example(example1)
add_example(example2)
add_example(example3)
add_example(example4)
add_example(example-impl1-step2)
add_example(false-sharing)
add_example(false-sharing-1)
add_example(false-sharing-2)
add_example(atomic)
add_example(acq-rel-example)
add_example(seq-cst-example)
add_example(tsfifo)
add_example(singleton)

function(add_impl name)
	add_executable(${name} ${name}.cpp)
	add_test(test-${name} sh -c "cat ${CMAKE_CURRENT_SOURCE_DIR}/test-file.json | ${CMAKE_CURRENT_BINARY_DIR}/${name} | diff -y ${CMAKE_CURRENT_SOURCE_DIR}/test-file.xml -")
endfunction()

add_impl(impl1)
add_impl(impl1-orig)
add_impl(impl2)
add_impl(impl2-orig)
add_impl(impl3)
add_impl(impl3-orig)

function(add_large_file name r)
	add_custom_command(
		OUTPUT ${name} 
		COMMAND sh -c "${CMAKE_CURRENT_SOURCE_DIR}/make-large-file.sh ${CMAKE_CURRENT_SOURCE_DIR}/large-file.json ${CMAKE_CURRENT_BINARY_DIR}/${name} ${r}"
		DEPENDS make-large-file.sh large-file.json)
	add_custom_target(make-${name} ALL DEPENDS ${name})
endfunction()

add_large_file(large-file.json 1)
add_large_file(xlarge-file.json 5)
add_large_file(xxlarge-file.json 25)

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

