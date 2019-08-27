/**
 * @file
 * @brief     CIS 565 Version Checker
 * @details   A simple CUDA hello-world-style program for Patrick Cozzi's
 *            CIS 565: GPU Programming, at the University of Pennsylvania.
 * @authors   Starter code: Yining Karl Li, Liam Boone, Harmony Li, Kai Ninomiya
 * @copyright University of Pennsylvania
 */

#include <cstdio>
#include <cuda_runtime_api.h>
#include <device_launch_parameters.h>
#include "kernel.h"

void checkCUDAError(const char *msg) {
    cudaError_t err = cudaGetLastError();
    if (cudaSuccess != err) {
        fprintf(stderr, "Cuda error: %s: %s.\n", msg, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
}

// Kernel that writes the image to the OpenGL PBO directly.
__global__ void createVersionVisualization(uchar4* PBOpos, int width, int height, int major,
        int minor) {
    int x = (blockIdx.x * blockDim.x) + threadIdx.x;
    int y = (blockIdx.y * blockDim.y) + threadIdx.y;
    int index = x + (y * width);

    if (x <= width && y <= height) {
        // Each thread writes one pixel location in the texture (textel)
        PBOpos[index].w = 0;
        PBOpos[index].x = 0;
        PBOpos[index].y = 0;
        PBOpos[index].z = 0;

        int ver = y < height / 2 ? major : minor;
        if (ver == 0) {
            PBOpos[index].x = 255;
        } else if (ver == 1) {
            PBOpos[index].y = 255;
        } else if (ver == 2) {
            PBOpos[index].z = 255;
        } else if (ver == 3) {
            PBOpos[index].x = 255;
            PBOpos[index].y = 255;
        } else if (ver == 5) {
            PBOpos[index].z = 255;
            PBOpos[index].y = 255;
        }
    }
}

// Wrapper for the __global__ call that sets up the kernel calls
void kernelVersionVis(uchar4* PBOpos, int width, int height, int major, int minor) {
    // set up crucial magic
    unsigned int blockSize = 16;
    dim3 threadsPerBlock(blockSize, blockSize);

    unsigned int blocksX = (width + blockSize - 1) / blockSize;
    unsigned int blocksY = (height + blockSize - 1) / blockSize;
    dim3 fullBlocksPerGrid(blocksX, blocksY);

    //kernel launches
    createVersionVisualization <<< fullBlocksPerGrid, threadsPerBlock>>>(PBOpos, width, height, major, minor);

    // make certain the kernel has completed
    cudaThreadSynchronize();

    checkCUDAError("Kernel failed!");
}
