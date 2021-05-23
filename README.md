# Updating the RCAS Index

This is the code for the following paper:

- **Inserting Keys into the Robust Content-and-Structure (RCAS) Index**
- Kevin Wellenzohn, Luka Popovic, Michael H. Böhlen, Sven Helmer
- ADBIS 2021


## Reproducibility

To reproduce the findings in this paper please follow the instructions in the
[reproducibility package](REPRODUCIBILITY.md).



## Compilation

The code is written in C++11. You need
- A C++11 compliant compiler
- CMake


### Compiling in RELEASE Mode:

Compiling in RELEASE mode turns on optimizations:

```
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```


### Compiling in DEBUG Mode:

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```
