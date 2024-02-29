FROM public.ecr.aws/lts/ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive


RUN apt-get clean && apt-get update -y -qq && apt-get install wget libgtest-dev cmake build-essential python3 ninja-build gcovr clang-tidy clang-format doxygen nodejs npm git -y --no-install-recommends

RUN cd /usr/src/gtest && cmake CMakeLists.txt && make && cd ./lib && cp *.a /usr/lib && ln -s /usr/lib/libgtest.a /usr/local/lib/libgtest.a && ln -s /usr/lib/libgtest_main.a /usr/local/lib/libgtest_main.a
#update nodejs
RUN npm install n -g && n stable
RUN npm config set strict-ssl false

# install cspell
RUN npm install -g cspell 

# Set up Emsdk
RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git /opt/emsdk && /opt/emsdk/emsdk install latest && /opt/emsdk/emsdk activate latest

ENV PATH "/opt/emsdk:$PATH"

WORKDIR /home/wasm-instrumentation
