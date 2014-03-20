#!/bin/sh
echo "building cuda opcodes ..."
nvcc -O3 -shared -o libcudaop1.dylib adsyn.cu -DUSE_DOUBLE -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop2.dylib pvsops.cu -DUSE_DOUBLE -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
nvcc -O3 -shared -o libcudaop3.dylib slidingm.cu -DUSE_DOUBLE -I../../include -arch=sm_30 -I/usr/local/cuda/include -L/usr/local/cuda/lib -lcufft
echo "...done"