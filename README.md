# 3D Splatting Rendering
## [Read my dissertation here](https://thomas-chernaik.github.io/project.pdf)
![Render of a bike resting on a bench in a park](https://github.com/uol-feps-soc-comp3931-2324-classroom/final-year-project-thomas-chernaik/assets/60945086/b41d9935-25ab-4bb7-9f2a-3528657a8503)

## About
This is an interactive renderer for scenes stored as 3D Gaussian splats, made for my third year project at the University of Leeds. It is mostly based on the implementation described in the [original paper](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/). It is implemented in C++ with OpenGL compute shaders. There are also a couple of python files I used to generate some test files. This project contains an OpenGL GPU radix sort implementation, used for the depth sorting, which could also be used for other purposes. The project was developed on AMD integrated graphics in Ubuntu, and is not guaranteed to work on other hardware or operating systems, but may do. Warp sizes may not be valid on other hardware, and endianness and libraries may not work exactly as intended on other platforms.
## Acquire a model
This renderer is designed to work with `.ply` files, although could be adapted to work with other files if you wish. An example can be found [here on hugging face](https://huggingface.co/datasets/dylanebert/3dgs/blob/main/bicycle/point_cloud/iteration_30000/point_cloud.ply)

Set the first parameter of the Splats constuctor on line 47 of main.cpp to be the path to this file.

`Splats splats(<file-path>, camera.getWidth(), camera.getHeight());`
## Prerequisites
The project requires OpenGL 4.6 or newer to work. OpenGL libraries are not included with the project.
## Build instructions
create build folder `mkdir build-release`

navigate to build folder `cd ./build-release`

run cmake `cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release`

build with ninja `ninja`

navigate to base directory `cd ..`

run binary "diss" `./build-release/diss`
