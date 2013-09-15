/*
    Simple wrapper for PAPI, based on Ben Cummings' papi-wrap 
    Rewritten to allow running on Intel MIC, in offload or native modes
	timothy.dykes@myport.ac.uk
*/

#ifndef MIC_PAPI_WRAP_H
#define MIC_PAPI_WRAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include <omp.h>
#include </users/dykest/Programs/papi/papi.h>

// Rudimentary array class as cannot use std::vector in offload code
template<typename T>
class Array_T{
public:
	// TODO: mm_malloc to 64 instead of new
	Array_T() { arr_ = NULL; size_ = 0; }
	Array_T(int i) { arr_ = new T[i]; size_ = i; } 
	~Array_T() { if(arr_ != NULL) delete[] arr_; }

	T& operator[] (int loc) {
		if(loc >= size_){
			printf("Error: array element access out of range\n");
			exit(1);
		}
		else
			return arr_[loc];
	};

	void operator= (Array_T<T>& copyarr) {
		if(size_ != copyarr.size())
			resize(copyarr.size());
		for(unsigned i = 0; i < size_; i++)
			arr_[i] = copyarr[i];
	}

	void resize(int newSize) {
		if(arr_ != NULL) {
			T* temp = new T[newSize];
			memcpy(temp, arr_, (newSize > size_ ? size_ : newSize)*sizeof(T) );
			arr_ = temp;
			size_ = newSize;
		}
		else {
			arr_ = new T[newSize];
			size_ = newSize;
		}
	}

	void push_back(T& element) {
		resize(size_+1);
		arr_[size_-1] = element;
	}

	unsigned size() { return size_; }

private:
	T* arr_;
	unsigned size_;
};


// Record class to store recording of counters for section of code
class Record{
public:
	Record() {}
	Record(std::string name, unsigned nThreads, unsigned nEvents, Array_T<double>& t)
	{
		name_ = name;
		recordedCounts_.resize(nThreads);
		for(unsigned i = 0; i < nThreads; i++)
			recordedCounts_[i].resize(nEvents);
		time_ = t;
	}
	std::string name() { return name_; }
	Array_T<double>& time() { return time_; }

	Array_T< long long >& operator[] (unsigned loc) { return recordedCounts_[loc]; };

private:
	std::string name_;
	Array_T< Array_T<long long> > recordedCounts_;
	Array_T<double> time_;
};

// Wrapper for PAPI
class PapiWrapper{
public:
	PapiWrapper() { setup_ = false; numEvents_ = 0; numThreads_ = 1; debug_ = false;  counting_=false;}
	~PapiWrapper() {}

	void init();
	void setDebug(bool);
	void startRecording(std::string);
	void stopRecording();
	void printRecord(std::string);

private:
	void startCounters();
	void stopCounters();
	void papiPrintError(int);

	Array_T<Record> records_;
	Array_T<double> times_;
	Array_T<std::string> eventNames_;
	Array_T<int> eventIds_;
	Array_T< Array_T<long long> > counters_;
	Record* currentRecord_;
	bool setup_;
	bool debug_;
	bool counting_;
	int numThreads_;
	int numEvents_;
    int eventSet_;
};

#endif
