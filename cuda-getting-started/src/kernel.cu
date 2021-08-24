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

/**
 * Ver  Color           HEX         RGB
 * 0  -> White          #ffffff     rgb(255, 255, 255)
 * 1  -> Red            #ff0000     rgb(255,   0,   0)
 * 2  -> Orange         #ff8000     rgb(255, 128,   0)
 * 3  -> Yellow         #ffff00     rgb(255, 255,   0)
 * 4  -> Lime           #80ff00     rgb(128, 255,   0)
 * 5  -> Green          #00ff00     rgb(  0, 255,   0)
 * 6  -> Spring Green   #00ff80     rgb(  0, 255, 128)
 * 7  -> Cyan           #00ffff     rgb(  0, 255, 255)
 * 8  -> Dodger Blue    #0080ff     rgb(  0, 128, 255)
 * 9  -> Blue           #0000ff     rgb(  0,   0, 255)
 * 10 -> Purple         #8000ff     rgb(128,   0, 255)
 * 11 -> Violet         #ff00ff     rgb(255,   0, 255)
 * 12 -> Magenta        #ff0080     rgb(255,   0, 128)
 * *  -> Black          #000000     rgb(  0,   0,   0)
 */
__host__ __device__ void versionToColor(uchar4* pixel, int version) {
    switch(version) {
        case 0:
            pixel->x = 255; pixel->y = 255; pixel->z = 255; break;
        case 1:
            pixel->x = 255; pixel->y =   0; pixel->z =   0; break;
        case 2:
            pixel->x = 255; pixel->y = 128; pixel->z =   0; break;
        case 3:
            pixel->x = 255; pixel->y = 255; pixel->z =   0; break;
        case 4:
            pixel->x = 128; pixel->y = 255; pixel->z =   0; break;
        case 5:
            pixel->x =   0; pixel->y = 255; pixel->z =   0; break;
        case 6:
            pixel->x =   0; pixel->y = 255; pixel->z = 128; break;
        case 7:
            pixel->x =   0; pixel->y = 255; pixel->z = 255; break;
        case 8:
            pixel->x =   0; pixel->y = 128; pixel->z = 255; break;
        case 9:
            pixel->x =   0; pixel->y =   0; pixel->z = 255; break;
        case 10:
            pixel->x = 128; pixel->y =   0; pixel->z = 255; break;
        case 11:
            pixel->x = 255; pixel->y =   0; pixel->z = 255; break;
        case 12:
            pixel->x = 255; pixel->y =   0; pixel->z = 128; break;
        default:
            pixel->x =   0; pixel->y =   0; pixel->z =   0;
    }
}

// Kernel that writes the image to the OpenGL PBO directly.
__global__ void createVersionVisualization(uchar4* PBOpos, int width, int height, int major, int minor) {
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
        versionToColor(&PBOpos[index], ver);
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
