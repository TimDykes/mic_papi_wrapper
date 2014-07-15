#include "papi_wrapper.h"

void PapiWrapper::init()
{  
	if(setup_) {
		printf("Cannot call init when PAPI is already initialised\n");
		exit(1);
	}
    
    int papi_error;
    
    // Init library
    papi_error = PAPI_library_init(PAPI_VER_CURRENT);
    if (papi_error != PAPI_VER_CURRENT) {
        printf("PAPI library init error!\n");
        papiPrintError(papi_error);
        exit(1);
    }

#ifdef _OPENMP
    // Assume fixed thread affinity, otherwise this approach fails
    papi_error = PAPI_thread_init((long unsigned int (*)()) omp_get_thread_num);
    if ( papi_error != PAPI_OK ) {
        printf("Could not initialize the library with openmp.\n");
        papiPrintError(papi_error);
        exit(1);
    }
    numThreads_ = omp_get_max_threads();
#endif

    // Determine the number of hardware counters
    int num_hwcntrs, i;
    papi_error = num_hwcntrs = PAPI_num_counters();
    if (papi_error <= PAPI_OK) {
        printf("Unable to determine number of hardware counters\n");
        papiPrintError(papi_error);
        exit(1);
    }


    // Get user defined events
    char* papi_counters = getenv("PAPI_EVENTS");
    if(papi_counters == NULL) {
        printf("PAPI_EVENTS environment variable not set, PAPI not initialised, running in time only mode\n");
        timeOnly_ = true;
        fflush(0);
        return;        
    }
    numEvents_ = 0;
    char* result = NULL;
    char  delim[] = "|";
    char* temp = strdup(papi_counters);
    result = strtok( temp, delim );

    while( result != NULL ) {
        numEvents_++;
        result = strtok( NULL, delim);
    }

    // Allocate memory and store event names/IDs
    if(numEvents_ < 1) {
        printf("No PAPI events set, PAPI not initialised, running in time only mode\n");
        fflush(0);
        timeOnly_ = true;
        return;
    }

    eventNames_.resize(numEvents_);
    eventIds_.resize(numEvents_);
    result = NULL;
    result = strtok( papi_counters, delim );
    int ctr = 0;
    
    while( result != NULL ) {
        int eventID;
        papi_error = PAPI_event_name_to_code(result, &eventID);

        if(papi_error==PAPI_OK) {

           eventNames_[ctr] = result;
           eventIds_[ctr] = eventID;
           ctr++;
        }
        else {
           printf("Unable to obtain code for event name: %s\n", result);
           papiPrintError(papi_error);
           exit(1);
        }

        result = strtok( NULL, delim );
    }

    if(debug_) {
        printf("Hardware Counters: %d\nThreads: %d\nEvents: %d\n", num_hwcntrs, numThreads_, numEvents_);
        for(unsigned i = 0; i < numEvents_; i++)
            printf("Event %d out of %d: %s\n",i,numEvents_,eventNames_[i]);
        fflush(0);
    }

    if(numEvents_ > num_hwcntrs)
    {
        printf("%d events requested but only %d hardware counters available.\nExiting.", numEvents_, num_hwcntrs);
        exit(1);
    } 

    // Create PAPI event set
    eventSet_ = PAPI_NULL;
    papi_error = PAPI_create_eventset(&eventSet_);

    if(papi_error != PAPI_OK) {
        printf("Could not create event set\n");
        papiPrintError(papi_error);
        exit(-1);
    }

    // Allocate memory for counters, nthread elements each containing nevent elements
    counters_.resize(numThreads_);
    for(unsigned i = 0; i < counters_.size(); i++)
        counters_[i].resize(numEvents_);


	setup_ = true;
}

void PapiWrapper::setDebug(bool onoff)
{
    debug_ = onoff;
}

void PapiWrapper::setVerboseDebug(bool onoff)
{
    verbose_debug_ = onoff;
}

void PapiWrapper::startRecording(unsigned key)
{
    bool unique = true;
    for(int i = 0; i < uniqueKeys_.size(); i++){
        if(key == uniqueKeys_[i])
            unique = false;
    }
    if(unique)
        uniqueKeys_.push_back(key);
  
    Record newRecord __attribute__((aligned(64)));
    newRecord.Init(key, numThreads_, numEvents_);
    records_.push_back(newRecord);
    sTime_.resize(numThreads_);
    currentRecord_++;

#ifdef _OPENMP
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            //records_[currentRecord_].time()[tid] = -(omp_get_wtime());
            sTime_[tid]= -(omp_get_wtime());
        }
#else
        struct timeval t;
        gettimeofday(&t, NULL);
        //records_[currentRecord_].time()[0] =  -(t.tv_sec + 1e-6*t.tv_usec);
        sTime_[0] =  -(t.tv_sec + 1e-6*t.tv_usec);
#endif

    if(!timeOnly_)
        startCounters();
    else
        counting_ = true;
}

void PapiWrapper::stopRecording()
{
    if(!counting_) {
        printf("Cannot stop counter/timer when not counting/timing...\n");
        fflush(0);
        exit(1);
    }

    if(!timeOnly_)
        stopCounters();

#ifdef _OPENMP
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            //records_[currentRecord_].time()[tid] += omp_get_wtime();
            sTime_[tid] += omp_get_wtime();
        }
#else
        struct timeval t;
        gettimeofday(&t, NULL);
       // records_[currentRecord_].time()[0] =  (t.tv_sec + 1e-6*t.tv_usec);
        sTime_[0] =  (t.tv_sec + 1e-6*t.tv_usec);
#endif

    for(unsigned i = 0; i < numThreads_; i++){
        if(!timeOnly_){
            for(unsigned j = 0; j < numEvents_; j++)
               records_[currentRecord_][i][j] = counters_[i][j];  
        } 
        records_[currentRecord_].time()[i] = sTime_[i];
    }
}



void PapiWrapper::printRecord(unsigned key)
{
    if(records_.size() == 0){
        printf("Cannot print record when no recordings have been made");
        fflush(0);
        exit(1);
    }

    bool found = false;
    for(unsigned i = 0; i < records_.size(); i++) {
        if(records_[i].rID() == key){
            printf("------------------------\nFor KEY ID %d\n------------------------\n", key);
            for(unsigned j = 0; j < numEvents_; j++) {
                // Print event name
                // unsigned strSize = eventNames_[i].length();
                // std::string format;
                // for(unsigned k = 0; k < strSize+4; k++)
                //     format += "-";
                printf("Event: %s: \n",eventNames_[j]);

                // Print thread IDs
                for(unsigned k = 0; k < numThreads_; k++)
                    printf("Thread ID: %10d | ",k);
                printf("\n");

                // Print counts   
                for(unsigned k = 0; k < numThreads_; k++)
                    printf("Count: %14lld | ", records_[i][k][j]);
                printf("\n");

                // For multiple openmp threads print cumulative total
                if(numThreads_>1){
                    long long total = 0;
                    for(unsigned k = 0; k < numThreads_; k++)
                        total += records_[i][k][j];
                    printf("Accumulative total from %d threads: %lld\n", numThreads_, total);
                }
            }
        // Print time
        for(unsigned k = 0; k < numThreads_; k++)           
            printf("------------------------");
        printf("\n");
        for(unsigned k = 0; k < numThreads_; k++)
            printf("Time: %15f | ", records_[i].time()[k]);
        printf("\n");    

        // For multiple openmp threads print average time
        if(numThreads_>1){
            double total = 0;
            for(unsigned k = 0; k < numThreads_; k++)
                total += records_[i].time()[k];
            printf("Average time from %d threads: %f\n", numThreads_, total/(double)numThreads_);
        }   
        
        found = true;
        }
    }

    if(!found){
        printf("Record with key: %u not found in records list.\n", key);
        fflush(0);
    }
}

void PapiWrapper::printAllRecords()
{
    if(records_.size() == 0){
        printf("printAllRecords(): No records made \n");
        fflush(0);
    }

    for(unsigned i = 0; i < records_.size(); i++) {
        printf("------------------------\nFor KEY ID %d\n------------------------\n", records_[i].rID());
        for(unsigned j = 0; j < numEvents_; j++) {
            // Print event name
            // unsigned strSize = eventNames_[i].length();
            // std::string format;
            // for(unsigned k = 0; k < strSize+4; k++)
            //     format += "-";
            printf("Event: %s: \n",eventNames_[j]);

            // Print thread IDs
            for(unsigned k = 0; k < numThreads_; k++)
                printf("Thread ID: %10d | ",k);
            printf("\n");

            // Print counts   
            for(unsigned k = 0; k < numThreads_; k++)
                printf("Count: %14lld | ", records_[i][k][j]);
            printf("\n");

            // For multiple openmp threads print cumulative total
            if(numThreads_>1){
                long long total = 0;
                for(unsigned k = 0; k < numThreads_; k++)
                    total += records_[i][k][j];
                printf("Accumulative total from %d threads: %lld\n", numThreads_, total);
            }
        }
        // Print time
        for(unsigned k = 0; k < numThreads_; k++)           
            printf("------------------------");
        printf("\n");
        for(unsigned k = 0; k < numThreads_; k++)
            printf("Time: %15f | ", records_[i].time()[k]);
        printf("\n");    

        // For multiple openmp threads print average time
        if(numThreads_>1){
            double total = 0;
            for(unsigned k = 0; k < numThreads_; k++)
                total += records_[i].time()[k];
            printf("Average time from %d threads: %f\n", numThreads_, total/(double)numThreads_);
        }   
    }
}

void PapiWrapper::multiRunPrintAverageRecords()
{
    if(records_.size() == 0){
        printf("multiRunPrintFastestRecords(): No records made \n");
        fflush(0);
    }
    // Loop through each unique key, work out counts and totals as usual
    // but then provide averaged count and store in order to easily print
    // for excel
    Array_T< double > keyTimes;
    keyTimes.resize(uniqueKeys_.size());
    keyTimes.fill(0.0);

    Array_T< Array_T< long long > > keyEvents;
    keyEvents.resize(uniqueKeys_.size());

    for(unsigned i = 0; i < uniqueKeys_.size(); i++){
        keyEvents[i].resize(numEvents_);
        keyEvents[i].fill(0);
    }
    
    for(unsigned i = 0; i < uniqueKeys_.size(); i++){

        // Go through all records and extract times
        for(unsigned j = 0; j < records_.size(); j++) {
            // If record matches unique key
            if(records_[j].rID() == uniqueKeys_[i]){
                // Get its events
                for(unsigned k = 0; k < numEvents_; k++){
                    // Get cumulative total for all threads
                    for(unsigned l = 0; l < numThreads_; l++)
                         keyEvents[i][k] += records_[j][l][k];
                }
                for(unsigned k = 0; k < numThreads_; k++)
                    keyTimes[i] += records_[i].time()[k];
            }
        }

        // Average
        int nRuns = (records_.size()/uniqueKeys_.size());
        for(unsigned j = 0; j < numEvents_; j++)
            keyEvents[i][j] /= nRuns;

        printf("numThreads_ %d nRuns %d\n", numThreads_, nRuns);
        keyTimes[i] /= (numThreads_ * nRuns);
    }

    // Print results
    printf("-----------Events-----------\nTime\n");
    for(unsigned i = 0; i < numEvents_; i++)
        printf("%s\n",eventNames_[i]);
    printf("\n");

    printf("-----------Counts-----------\n");
    for(unsigned i = 0; i < uniqueKeys_.size(); i++){
            printf("%f\t",keyTimes[i]);
    }
    printf("\n");

    for(unsigned i = 0; i < numEvents_; i++){
        for(unsigned j = 0; j < uniqueKeys_.size(); j++){
            printf("%lld\t",keyEvents[j][i]);
        }
        printf("\n");
    }
    printf("\n");
    fflush(0);
}

void PapiWrapper::papiPrintError(int err)
{
	char* errName = PAPI_strerror(err);
	printf("PAPI error code: %s (%d)\n", errName, err);
	fflush(0);
}

void PapiWrapper::startCounters()
{
    if(!setup_){
        printf("Must initialise PAPI before starting counters.\n");
        fflush(0);
        exit(1);
    }

    if(counting_) {
        printf("Cannot start counters when already counting.\n");
        fflush(0);
        exit(1);
    }


    #pragma omp parallel
    {
        if(numEvents_){
                #ifdef _OPENMP
                        int tid = omp_get_thread_num();
                #else
                        int tid = 0;
                #endif
            int papi_error = PAPI_start_counters(&eventIds_[0], eventIds_.size());
            if (papi_error != PAPI_OK){
                printf("Thread %d: Could not start counters\n", tid);
                if(verbose_debug_){
                    for(unsigned i = 0; i < eventIds_.size(); i++)
                        printf("EventID %d out of %d: 0x%X\n",i, eventIds_.size(), eventIds_[i]);
                    fflush(0);
                }
                papiPrintError(papi_error);
                exit(-1);
            }
        }
    }

    counting_ = true;

}

void PapiWrapper::stopCounters()
{
    if(!counting_) {
        printf("Cannot stop counters when not counting.\n");
        fflush(0);
        exit(1);
    }

    #pragma omp parallel
    {
#ifdef _OPENMP
        int tid = omp_get_thread_num();
#else
        int tid = 0;
#endif
        if(numEvents_){
            int papi_error = PAPI_stop_counters(&counters_[tid][0], eventIds_.size());
            if (papi_error != PAPI_OK){
                printf("Could not stop counters\n");
                papiPrintError(papi_error);
                exit(-1);
            }
        }
    }

    counting_ = false;
}




