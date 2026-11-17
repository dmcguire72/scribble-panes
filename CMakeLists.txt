cmake_minimum_required(VERSION 3.24)
project(scribble-panes CXX)

find_package(panes REQUIRED)   # -DCMAKE_PREFIX_PATH=<the unzipped panes folder>
panes_add_app(scribble main.cpp scribble.cpp)
panes_target_assets(scribble assets)