# Material Point Method for snow simulation 2D

Explanation of MPM in 500~ lines of code.\
This repo is 2D Implementation of [Material Point Method for snow simulation.](https://www.math.ucla.edu/~jteran/papers/SSCTS13.pdf)
\
This code is for Educational purpose not parallelized and optimized!!
### Build instruction
```
mkdir build
cd build
cmake ../
make
```
(or give -G option depending on your preference.)
### Record
- Make directory `mpm2d/build/out/`;
- Make sure that your system has installed [ffmpeg](https://ffmpeg.org/download.html#build-windows)





### Result

Simulation result for Various parameter.

![1](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/1.gif)
![2](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/2.gif)
![3](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/3.gif)
![4](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/4.gif)
![5](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/5.gif)
![6](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/6.gif)
![7](https://github.com/LEE-JAE-HYUN179/mpm2d/blob/master/out/7.gif)


### Dependencies
 - [GLFW](https://github.com/glfw/glfw)
 - [GLM](https://github.com/g-truc/glm)
 - [Eigen](https://eigen.tuxfamily.org/index.php?title=Main_Page)
 - [ffmpeg](https://ffmpeg.org/download.html#build-windows)
