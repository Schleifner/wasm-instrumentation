find_package(GTest REQUIRED)
include(FetchContent)

file(GLOB test_sources CONFIGURE_DEPENDS ./lit.cpp) # innclude lit test by default
set(JSONCPP_WITH_TESTS false)
set(CMAKE_CXX_FLAGS "-fno-rtti")
if(ENABLE_SANITIZE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined,pointer-compare")
endif()
if(ENABLE_FUZZ)
  file(GLOB test_sources CONFIGURE_DEPENDS ${test_sources} ./fuzz.cpp)
endif()
if(ENABLE_E2E)
  file(GLOB test_sources CONFIGURE_DEPENDS ${test_sources} ./e2e.cpp)
endif()
add_executable(
  wasm-instrumentation-test
  ${test_sources}
)
target_link_libraries(
  wasm-instrumentation-test
  GTest::gtest_main
  binaryen
  jsoncpp_static
)
file(GLOB sources CONFIGURE_DEPENDS ../src/*.cpp ../src/*.hpp)
target_sources(wasm-instrumentation-test PUBLIC ${sources})
target_include_directories(wasm-instrumentation-test PUBLIC ../thirdparty/binaryen/src ../thirdparty/jsoncpp/include)
include(GoogleTest)
gtest_discover_tests(wasm-instrumentation-test)
if(ENABLE_E2E)
  add_custom_target(npm-install ALL
      COMMAND npm install
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test-asc
      )
  add_custom_target(compile-asc ALL
      DEPENDS npm-install
      COMMAND npm run ascbuild
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test-asc
      )
  add_dependencies(wasm-instrumentation-test copy-files)
  add_custom_target(copy-files ALL
      DEPENDS compile-asc
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/test-asc/build
      ${CMAKE_CURRENT_BINARY_DIR}/test-asc/build
      )
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
  LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()
  setup_target_for_coverage_gcovr_xml(NAME cov
                            EXECUTABLE wasm-instrumentation-test
                            DEPENDENCIES wasm-instrumentation-test
                            BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src")

  setup_target_for_coverage_gcovr_html(NAME cov_html 
                            EXECUTABLE wasm-instrumentation-test
                            DEPENDENCIES wasm-instrumentation-test
                            BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src")                              
endif()
