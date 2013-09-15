#define ALLOC alloc_if(1) free_if(0)
#define FREE alloc_if(0) free_if(1)
#define REUSE alloc_if(0) free_if(0)

#ifdef __INTEL_OFFLOAD
#define EVT_TARGET_MIC __attribute__((target(mic))) 
#else
#define EVT_TARGET_MIC
#endif

