## Build
* install gtest: `sudo apt-get install libgtest-dev && cd /usr/src/gtest && sudo cmake CMakeLists.txt && sudo make && cd ./lib && sudo cp *.a /usr/lib && sudo ln -s /usr/lib/libgtest.a /usr/local/lib/libgtest.a && sudo ln -s /usr/lib/libgtest_main.a /usr/local/lib/libgtest_main.a`  
* build: `cmake -B build -G Ninja && cmake --build build --parallel 2`  
* run test: `cd build && ninja test`

## build WASM Variant
* install emsdk: [GUIDE](https://emscripten.org/docs/getting_started/downloads.html)
* `emsdk install tot` `emsdk install latest`
* `emsdk activate tot` `emsdk activate latest`
* `source [emsdk dir]/emsdk_env.sh`
* `mkdir dist && cd dist`
* `emcmake cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DGTEST_LIBRARY=/usr/src/gtest/lib/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/lib/libgtest_main.a -DGTEST_INCLUDE_DIR=/usr/src/gtest/include/ -DBUILD_TESTS=0 -DEMSCRIPTEN=1`
* `emmake make -j$(nproc) wasm-instrumentation`

## run lit test
`./build/test/wasm-instrumentation-test --gtest_filter=asc_cov_instru_test.lit`