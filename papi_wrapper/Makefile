#-----------------------------------#
#			Demo Makefile			#
#-----------------------------------#

# Compiler
CXX = icpc

# Target
TARGET = mic_demo

# General compiler flags
CPPFLAGS = -fopenmp -Wall -O2 -I. -L.  

# Compiler flags for host offloading c++ files
OFFLOAD_MIC_FLAGS = -offload-option,mic,compiler," -fopenmp -Wall -ansi-alias -O3 -I. -L. -z defs -ffreestanding -opt-streaming-stores always -opt-streaming-cache-evict=0 -mP2OPT_hlo_use_const_pref_dist=64 -mP2OPT_hlo_use_const_second_pref_dist=8 -wd3218" -wd3218

# Compiler flags for native MIC c++ files
NATIVE_MIC_FLAGS = -mmic -fopenmp -fPIC -shared

# Additional libraries
LIBS = 

# Include directories for host build
INC =

# Include directories for native build
NATIVE_INC = 

# Use PAPI wrapper
OPT = -DUSE_PAPI_WRAP

ifeq (USE_PAPI_WRAP,$(findstring USE_PAPI_WRAP,$(OPT)))
	PAPI_PATH = /users/dykest/Programs/papi 
	NATIVE_MIC_FLAGS +=  -lpapi -lpfm -L$(PAPI_PATH)
	INC += -I$(PAPI_PATH)
	NATIVE_INC += -I$(PAPI_PATH)
	OFFLOAD_MIC_FLAGS += -offload-option,mic,compiler,"-L. -lpwp " 
endif

# In order to use PAPI in offload, we build the papi_wrapper (which links to a MIC 
# native version of PAPI) as a native library, and then link to it via the mic compiler 
# options in CPPFLAGS

# Rules
all: $(TARGET)

$(TARGET): offload_stream.o 
	$(CXX) $(CPPFLAGS) $(OFFLOAD_MIC_FLAGS) $(LIBS) offload_stream.o -o $(TARGET)

offload_stream.o: offload_stream.cpp libpwp.so
	$(CXX) -c offload_stream.cpp $(CPPFLAGS) $(INC) $(OPT) $(OFFLOAD_MIC_FLAGS) -o "$@" 


libpwp.so: papi_wrapper.cpp 
	$(CXX) $(NATIVE_MIC_FLAGS) $(NATIVE_INC) -o "$@" "$<"

clean: 
	rm -f *.o
	rm -f $(TARGET)

cleanlib:
	rm -f *.o
	rm -f *.so
	rm -f $(TARGET)
