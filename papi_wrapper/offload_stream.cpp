/*
 * Copyright (c) 2004-2014
 *              Tim Dykes University of Portsmouth
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//---------------------------------------------------------------
// MIC offload Demo
//---------------------------------------------------------------
#include <cstdio>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <sys/time.h>
#include "mic_utils.h"


#ifdef USE_PAPI_WRAP
    #pragma offload_attribute(push, target(mic))
    #include "papi_wrapper.h"
    #pragma offload_attribute(pop)
#endif


#define MULTIRUN

#ifdef MULTIRUN
    #define NTIMES          10
#else
    #define NTIMES          1
#endif

#define SIZE            400000000
#define STREAM_TYPE     float

#ifdef USE_PAPI_WRAP
    #define STR_COPY        0
    #define STR_SCALE       1
    #define STR_ADD         2
    #define STR_TRIAD       3
#endif

double getTime();
void reportTime(std::string);
std::vector<double> timer;
 
int main(int argc, char* argv[])
{

    // Init host timer
    timer.push_back(getTime());
    int nBytes = sizeof(STREAM_TYPE);
    double memReqPerArray = (((double)(nBytes*SIZE) / 1024) / 1024);
    double totalMemReq = memReqPerArray*3;
    // Inital report
    printf("-----------Offload stream benchmark-----------\n");
    printf("Using %d byte array elements.\n",nBytes);
    printf("Number of elements: %d\n",SIZE);
    printf("Memory required per array: %f MB (%f GB).\n",memReqPerArray,memReqPerArray/1024);
    printf("Total memory require: %f MB (%f GB).\n",totalMemReq,totalMemReq/1024);
    printf("Running bench %d times, using average (discarding first run).\n", NTIMES);
 
    // Alloc memory on host and fill with some data
    STREAM_TYPE* x = (STREAM_TYPE*)_mm_malloc(SIZE*sizeof(STREAM_TYPE), 64);
    STREAM_TYPE* y = (STREAM_TYPE*)_mm_malloc(SIZE*sizeof(STREAM_TYPE), 64);
    STREAM_TYPE* z = (STREAM_TYPE*)_mm_malloc(SIZE*sizeof(STREAM_TYPE), 64);
    for(unsigned i = 0; i < SIZE; i++)
    {
        x[i] = 1.0;
        y[i] = 2.0;
        z[i] = 0.0;
    }
    reportTime("Data generation & initial report");
 
    // Allocate memory on device
 	#pragma offload_transfer target(mic:0)  nocopy(x : length(SIZE) ALLOC) \
                                            nocopy(y : length(SIZE) ALLOC) \
                                            nocopy(z : length(SIZE) ALLOC)

    reportTime("Device memory alloc");

    // Copy data from host to device
	#pragma offload_transfer target(mic:0)  in(x : length(SIZE) REUSE) \
                                            in(y : length(SIZE) REUSE) \
                                            in(z : length(SIZE) REUSE) 
    
    reportTime("Data transfer");
    fflush(0);

    // Offload stream bench
    #pragma offload target(mic:0) nocopy(x) \
                                  nocopy(y) \
                                  nocopy(z)
    {
        // Init omp and check threads/procs
        #pragma omp parallel
        #pragma omp master
        {
            printf("omp_num_procs(): %d\n",omp_get_num_procs());
            printf("omp_num_threads(): %d\n",omp_get_num_threads());
            fflush(0);
        }

        // Init PAPI
        #ifdef __MIC__
            #ifdef USE_PAPI_WRAP
                PapiWrapper pw;
                pw.setDebug(true);
                pw.init();
            #endif
        #endif

        STREAM_TYPE scalar = 3.0;

        // Run bench (if(i) is used to skip first runthrough)
        for(int i = 0; i < NTIMES; i++)
        {
            #ifdef __MIC__
                #ifdef USE_PAPI_WRAP
                    if(i) pw.startRecording(STR_COPY);
                #endif
            #endif

            // Stream copy 
            #pragma omp parallel for
            #pragma ivdep
            for (int j = 0; j < SIZE; j++)
            {
                __assume_aligned(x, 64);
                __assume_aligned(z, 64);
                z[j] = x[j];
            }  

            #ifdef __MIC__
                #ifdef USE_PAPI_WRAP
                    if(i) pw.stopRecording();
                    if(i) pw.startRecording(STR_SCALE);
                #endif                   
            #endif

            // Stream Scale
            #pragma omp parallel for 
            #pragma ivdep
            for (int j = 0; j < SIZE; j++)
            {
                __assume_aligned(y, 64);
                __assume_aligned(z, 64);
                y[j] = scalar*z[j];
            }   

            #ifdef __MIC__
                #ifdef USE_PAPI_WRAP
                    if(i) pw.stopRecording();
                    if(i) pw.startRecording(STR_ADD);    
                #endif               
            #endif

            // Stream add
            #pragma omp parallel for
            #pragma ivdep
            for (int j = 0; j < SIZE; j++)
            {
                __assume_aligned(x, 64);
                __assume_aligned(y, 64);
                __assume_aligned(z, 64);    
                z[j] = x[j]+y[j];
            }

            #ifdef __MIC__
                #ifdef USE_PAPI_WRAP
                    if(i) pw.stopRecording();
                    if(i) pw.startRecording(STR_TRIAD);    
                #endif               
            #endif

            // Stream triad      
            #pragma omp parallel for
            #pragma ivdep
            for (int j = 0; j < SIZE; j++)
            {
                __assume_aligned(x, 64);
                __assume_aligned(y, 64);
                __assume_aligned(z, 64);   
                x[j] = y[j]+scalar*z[j];
            }

            #ifdef __MIC__
                #ifdef USE_PAPI_WRAP
                    if(i) pw.stopRecording();
                #endif
            #endif   
        }

        #ifdef __MIC__
            #ifdef USE_PAPI_WRAP
                #ifdef MULTIRUN
                    pw.multiRunPrintAverageRecords();
                #else
                    pw.printAllRecords();
                #endif
            #endif
        #endif

        printf("Finished bench on device\n");
        fflush(0);
    }

    double t = getTime() - timer.front();
    printf("Overall time: %f\n", t);
    return 0;
}

void reportTime(std::string s)
{
    timer.push_back(getTime());
    for(unsigned i = 0; i < timer.size()-1; i++)
        timer.back() -= timer[i];
    printf("%s: %f\n",s.c_str(),timer.back());
}

double getTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + 1e-6*t.tv_usec;
}
