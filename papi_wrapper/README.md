This wrapper allows to use PAPI in offload mode on Intel Xeon Phi, with a demo usage in an offloaded implementation of the STREAM benchmark. The wrapper uses code from Ben Cummings papi-wrap: https://github.com/bcumming/papi-wrap

The wrapper is built natively for the MIC, then linked to only for the MIC objects when compiling mic/host code. The limitation with this is that you can only use the wrapper within __MIC__ preprocessor definition checks. There also appears to be some side effect of this that causes linker problems with STL containers (string/vector etc) hence use of a custom templated array class until I figure out the issue.

First modify the Makefile for your system, particularly to insert path to the papi install directory.
Then modify prepenv.sh to insert the path to the papi lib, and modify thread settings if you want.

For running:

source prepenv.sh
source preppapi.sh
./mic_demo

The first script sets the LD_LIBRARY_PATH and some thread settings.
The second sets which events to measure. Currrently set to VPU_INSTRUCTIONS_EXECUTED and VPU_ELEMENTS_ACTIVE, which when dividing the former by the latter provides the Vectorization Intensity (i.e. how many elements of the vector registers were active on average per instruction, 8 is target for double precision, and 16 for single).

The file runscript.sh provides a script to run a set of times collecting a different event/eventset each time, there are two hw counters per hw thread context (for most events), although you some events are limited to one particular counter and so you cannot collect two such events at once.