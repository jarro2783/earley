add_library(test_main main.cpp)

function(add_test_binary test_name)
  set(test_binary ${test_name}_test)
  add_executable(${test_binary} ${ARGN})
  target_link_libraries(${test_binary} test_main libearley fast)
  add_test(${test_name} ${test_binary})
endfunction()

add_test_binary(hash hash.cpp)
add_test_binary(fast fast.cpp)
add_test_binary(stack stack.cpp)
add_test_binary(grammar grammar_util.cpp)
add_test_binary(timer timer.cpp)
