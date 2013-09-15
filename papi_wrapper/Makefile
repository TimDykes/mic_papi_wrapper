#-----------------------------------#
#			Demo Makefile			#
#-----------------------------------#

# Compiler
CXX = icpc

# Target
TARGET = mic_demo

# Use openmp
OMP = -fopenmp
#OMP = 

# General compiler flags
OPTS = -Wall -O2 $(OMP)

# Mic offload compiler flags
MIC_OFFLOAD_OPTS = -offload-option,mic,compiler," $(OMP) -Wall -O2 -lpwp -L. -z defs"

# Mic native compiler flags
MIC_NATIVE_OPTS = -mmic $(OMP) -fPIC -shared -lpapi -lpfm -L/users/dykest/Programs/papi

# Objects to be built
OBJS = main.o
#$(patsubst %.cpp,%.o,$(wildcard *.cpp))

# Rules
all: $(TARGET)

$(TARGET): $(OBJS) libpwp.so 
	$(CXX) $(OPTS) $(MIC_OFFLOAD_OPTS) $(LIBS) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) -c $(OPTS) $(MIC_OFFLOAD_OPTS) -o "$@" "$<"

libpwp.so: papi_wrapper.cpp
	$(CXX) $(MIC_NATIVE_OPTS) -o libpwp.so papi_wrapper.cpp

clean: 
	rm -f *.o *.so
	rm -f $(TARGET)
