#define ALLOC alloc_if(1) free_if(0)
#define FREE alloc_if(0) free_if(1)
#define REUSE alloc_if(0) free_if(0)

#ifdef __INTEL_OFFLOAD
#define EVT_TARGET_MIC __attribute__((target(mic))) 
#else
#define EVT_TARGET_MIC
#endif

#ifndef MIC_PSIM
#define MIC_PSIM

#pragma offload_attribute(push, target(mic))

#include <omp.h>

#pragma offload_attribute(pop)


#endif