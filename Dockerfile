FROM ubuntu:20.04

RUN apt-get update && apt-get install -y g++ build-essential

WORKDIR /src
COPY . .

# Build the C++ binary
RUN mkdir -p bin && \
    g++ -std=c++14 -O2 run_simulation.cc cheby.cc simulate_system.cc params.cc params_common.cc ev.cc -o bin/sim
