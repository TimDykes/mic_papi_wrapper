#!/bin/bash
export MIC_LD_LIBRARY_PATH=$MIC_LD_LIBRARY_PATH:.:/users/dykest/Programs/papi
export OFFLOAD_INIT=on_start
export MIC_USE_2MB_BUFFERS=64K