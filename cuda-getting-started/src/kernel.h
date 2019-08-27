#pragma once

#include <cuda.h>
#include <vector_types.h>

void kernelVersionVis(uchar4* pos, int width, int height, int major, int minor);
