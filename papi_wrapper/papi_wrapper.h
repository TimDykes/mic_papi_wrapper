/*
    Simple wrapper for PAPI, based on Ben Cummings' papi-wrap 
    Updated and modified for easy offloading to Intel MIC by Tim Dykes
	timothy.dykes@myport.ac.uk
*/

#ifndef MIC_PAPI_WRAP_H
#define MIC_PAPI_WRAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#ifdef _OPENMP
	#include <omp.h>
#endif
#include <papi.h>

#include "array_t.h"

class Record{
public:
	Record() {}
	void Init(unsigned rID, unsigned nThreads, unsigned nEvents)
	{
		rID_ = rID;
		recordedCounts_.resize(nThreads);
		for(unsigned i = 0; i < nThreads; i++)
			recordedCounts_[i].resize(nEvents);
		time_.resize(nThreads);
	}
	unsigned rID() const{ return rID_; }
	Array_T<double>& time() { return time_; }
	Array_T<double> const& time() const { return time_; }
	Array_T< Array_T<long long> > const& recordedCounts_ref() const{ return recordedCounts_; }

	Array_T< long long >& operator[] (unsigned loc) { return recordedCounts_[loc]; };

	void operator= (Record const& input) {
		rID_ = input.rID();
		recordedCounts_ = input.recordedCounts_ref();
		time_ = input.time();
	}

	void operator= (Record& input) {
		rID_ = input.rID();
		recordedCounts_ = input.recordedCounts_ref();
		time_ = input.time();
	}

private:
	unsigned rID_;
	Array_T< Array_T<long long> > recordedCounts_;
	Array_T<double> time_;
};

class PapiWrapper{
public:
	PapiWrapper() { setup_ = false; numEvents_ = 0; numThreads_ = 1; debug_ = false; verbose_debug_ = false; counting_=false; timeOnly_ = false; currentRecord_ = -1;}
	~PapiWrapper() {}

	void init();
	void setDebug(bool);
	void setVerboseDebug(bool);
	void startRecording(unsigned);
	void stopRecording();
	void printRecord(unsigned);
	void printAllRecords();
	void multiRunPrintAverageRecords();

private:
	void startCounters();
	void stopCounters();
	void papiPrintError(int);

	Array_T<Record> records_;
	Array_T<double> times_;
	Array_T<char*> eventNames_;
	Array_T<int> eventIds_;
	Array_T< Array_T<long long> > counters_;
    Array_T<double> sTime_;
    Array_T<unsigned> uniqueKeys_;
	int currentRecord_;
	bool setup_;
	bool debug_;
	bool verbose_debug_;
	bool counting_;
	bool timeOnly_;
	int numThreads_;
	int numEvents_;
    int eventSet_;
};

#endif
