//---------------------------------------------------------------
// MIC papi wrapper demo
//
// Demonstrates use of papi wrapper library to measure l1 cache 
// data accesses and misses
// 
//---------------------------------------------------------------
#include <cstdio>
#include <unistd.h>
#include <stdlib.h>

#pragma offload_attribute(push, target(mic))
//#ifdef __MIC__
#include "papi_wrapper.h"
//#endif
#pragma offload_attribute(pop)

#include "mic_utils.h"

// Roughly 100MB 
#define ARR_SIZE 13000000
#define CHUNK_SIZE 4096

int main(int argc, char* argv[])
{

	int* test1 = (int*)_mm_malloc(ARR_SIZE*sizeof(int), 64);
	int* test2 = (int*)_mm_malloc(ARR_SIZE*sizeof(int), 64);

	for(int i = 0; i < ARR_SIZE; i++){
		test1[i] = i;
		test2[i] = ARR_SIZE-i;
	}

	#pragma offload target(mic:0) in(test1 : length(ARR_SIZE) ALLOC) in(test2 : length(ARR_SIZE) ALLOC)
	{
		// Shouldnt really use this macro within offload block
		#ifdef __MIC__
		
		printf("Array Size: %d\nChunk Size: %d\n", ARR_SIZE, CHUNK_SIZE);

		PapiWrapper pw;
		pw.setDebug(true);
		pw.init();
		pw.startRecording("L1_CACHE");

		int nchunks = (ARR_SIZE/CHUNK_SIZE)+1;

		#pragma omp parallel for
		for(int chunk = 0; chunk < nchunks; chunk++){
			__assume_aligned(test1, 64);
			__assume_aligned(test2, 64);
			int start = chunk*CHUNK_SIZE;
			int end = ((chunk+1)*CHUNK_SIZE) >= ARR_SIZE ? ARR_SIZE : (chunk+1)*CHUNK_SIZE;
			for(unsigned i = start; i < end; i++){
				test1[i] ^= test2[i];
				test2[i] ^= test1[i];
				test1[i] ^= test2[i];
			}
		}

		pw.stopRecording();

		pw.printRecord("L1_CACHE");

		#else
		printf("NOT ON MIC!\n");
		#endif
	}

	return 0;

}