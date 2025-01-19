#include <string>
#include "record/record_type.h"
#include <time.h>
#include <cstdlib>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#include "instrument/stg.h"
#include "utils/configuration.h"

//#define SEED_VALUE 10

class ifSampFlag {
    int samp_option, samp_ratio, samp_rankall, samp_step, samp_myrank, samp_ifRandom, samp_time;
    double samp_doublestep, samp_index;
    bool samp_dynscale;

    public:
    // samp_ifSamp is true only if this rank is sampled
    // samp_extendFlag is true only if this rank is not sampled  
    bool samp_ifSamp, samp_iftime, samp_extendFlag;
    std::vector<bool> sampList;
    unsigned seed;

    // temporal sampling variables
    typedef enum {
        FUNCTION_NAME=0,
        CALL_SITE,
        CALL_PATH,
    } TemporalSampleMode;
    bool samp_temp;
    TemporalSampleMode samp_temp_mode;
    int window_enable, window_disable;
    bool print_stg;
    std::string stg_fn;

    void init( int size, int rank );
    int genRatio();
    void dynSamp(int rank_dest);
    int getSampTime();
    bool getSampIf();
    bool getSampEx();
    bool getDynscale();

    ifSampFlag()  { samp_ifSamp=false; samp_extendFlag=false; }
};

bool ifSampFlag::getDynscale()
{
    return samp_dynscale;
}

int ifSampFlag::getSampTime(){
    return samp_time;
}

bool ifSampFlag::getSampIf(){
    return samp_ifSamp;
}

bool ifSampFlag::getSampEx(){
    return samp_extendFlag;
}

void ifSampFlag::init( int size, int rank ){

    sampList.resize(size, false);

    // samp_rankall: sum of the ranks
    samp_rankall = size;
    // samp_myrank: the number of the rank
    samp_myrank = rank;   
    // initial the samp_extendFlag.
    // true: this rank has been extended into sampling list
    // false: this rank has not been extended into sampling list
    samp_extendFlag = false;
    samp_dynscale = false;

    samp_temp = EnvConfigHelper::get_enabled("JSI_TEMPORAL_SAMPLING", false);
    if(samp_temp) {
        // FUNCTION_NAME, CALL_SITE, CALL_PATH (default)
        std::string jsi_temp_mode = EnvConfigHelper::get("JSI_TEMPORAL_SAMPLING_MODE", "CALL_PATH", 
                                                        {"FUNCTION_NAME", "CALL_SITE", "CALL_PATH"});
        if(jsi_temp_mode=="CALL_PATH") { 
            samp_temp_mode=TemporalSampleMode::CALL_PATH;
        } else if(jsi_temp_mode=="CALL_SITE") { 
            samp_temp_mode=TemporalSampleMode::CALL_SITE;
        } else if(jsi_temp_mode=="FUNCTION_NAME") {
            samp_temp_mode=TemporalSampleMode::FUNCTION_NAME;
        }
        // window size for sampled iterations (default=1)
        window_enable = EnvConfigHelper::get_int("JSI_TEMPORAL_SAMPLING_WINDOW_ENABLE", 1);
        // total window size for temporal sampling (default=5)
        // if both is default (enable=1, disable=5), the sampling rate is 1/5 = 20%
        window_disable = EnvConfigHelper::get_int("JSI_TEMPORAL_SAMPLING_WINDOW_DISABLE", 5);
        // debug info
        JSI_INFO("Enabling temporal sampling: mode=%s, window=%d/%d\n", jsi_temp_mode.c_str(), window_enable, window_disable);
        print_stg = EnvConfigHelper::get_enabled("JSI_PRINT_STG", false);
        stg_fn = EnvConfigHelper::get("JSI_MEASUREMENT_DIR", "./") + std::string("/stg.") + std::to_string(rank);
        if(print_stg) {
            JSI_INFO("STG will be printed to file: %s\n", stg_fn.c_str());
        }
    } else {
        JSI_INFO("Temporal sampling disabled\n");
    }

    char* jsi_samp_rt = getenv("JSI_SAMPLING_RATIO");
    char* jsi_samp_op = getenv("JSI_SAMPLING_MODE");
    char* jsi_samp_ifrand = getenv("JSI_SAMPLING_IFRANDOM");
    char* jsi_samp_ontime = getenv("JSI_SAMPLING_ONTIME");
    const char* dynscale_enable = getenv("JSI_DYNSCALE");
    if (dynscale_enable)
    {
        samp_dynscale = true;
    }

    if(!samp_rankall){
        JSI_ERROR("MPI message : No Mpi rank message \n");
    }

    if(samp_myrank<0){
        JSI_ERROR("MPI message : Wrong rank message \n");
    }

    if(!jsi_samp_op) {
        JSI_ERROR("PARAMETER : JSI_SAMPLING_MODE must be set!\n");
    }

    if(!jsi_samp_rt) {
        JSI_ERROR("PARAMETER : JSI_SAMPLING_RATIO must be set!\n");
    }

    /* todo:
    if(!jsi_samp_ifrand) {
        JSI_ERROR("PARAMETER : JSI_SAMPLING_IFRANDOM must be set!\n");
    }
    */

    /*
    // samp_option: 
    // 1: all sampling
    // 2: specific sampling ratio
    // 3: modified sampling ratio generate sampling ratio
    
    // samp_ratio: sampling ratio modified by command line parameters
    // example: 
        samp_ratio = 5
        Means  the toolkit will samples 5 processes every 100 processes

    // samp_ifRandom: a command line parameter to specify whether  the toolkit 
    // randomly samples the processes 
    */
    samp_option = atoi(jsi_samp_op);
    samp_ratio = atoi(jsi_samp_rt);
    samp_ifRandom = atoi(jsi_samp_ifrand);
    samp_time = atoi(jsi_samp_ontime);
    //std::cout << "samp_time value is " << samp_time << "\n";


    if (samp_ifRandom)
    {
        seed = (unsigned)samp_ifRandom;
    }
    else
    {
        seed = 0;
    }

    if ( samp_option == 3 ){
        // randomling generate samp_ratio adaptively
        samp_ratio = ifSampFlag::genRatio();
        std::cout << "samp_ratio = " << samp_ratio << "\n" ;
        // std::cout << "samp_ratio is " << samp_ratio << "\n";
    }

    /*
    if ( samp_option == 1 ){
        samp_ifSamp = true;
    }
    else {
        samp_ifSamp = ( samp_myrank / samp_step ) == 0 ? true : false;
    }
    */

    // if random sampling
    // generate the global original sampling list using same seed value 
    if (samp_ifRandom){

        //seed = samp_ifRandom;
        srand(seed);
        int num;
        int sampRank;

        // the num of sampled rank
        sampRank = samp_rankall * samp_ratio / 100 ;
        //std::cout << "sampRank = " << sampRank << "\n" ;

        // generate sampList
        for (int x = 0; x < sampRank; x++){ 
            num = rand()%samp_rankall;

            // no repeat
            if (sampList[num]){
                x--;
                continue;
            }

            // mark the sampled rank use bool vector
            sampList[num] = true;
            // test code:
            // std::cout << x << "." << num << "\n";
            // std::cout << x << "." << rand()%1920000 << "\n";
            //rand();
        }
        samp_extendFlag = sampList[rank];

        // todo : sample trace data with random rank in a specific ratio
        
    }
    else {

        if (samp_option==1){
            // all processes sampling
            for (std::vector<bool>::size_type x = 0; x != sampList.size(); x++){

                sampList[x] = true;
            }
            samp_extendFlag = true;
        }
        else if (samp_option==2 || samp_option==3){
            // user modified sampling_ratio
            /*
            std::cout << "test : branch op 2 \n";
            std::cout << "size is " << size << "\n";
            std::cout << "rank is " << rank << "\n";
            std::cout << "samp_ratio is " << samp_ratio << "\n";
            */
            samp_doublestep = (double)100 / (double)samp_ratio;
            // std::cout << "samp_doublestep : " << samp_doublestep << "\n";
            for ( samp_index=0 ; samp_index < size - 0.5 ; samp_index += samp_doublestep ){
                sampList[(int)(samp_index+0.5)] = true;
                // std::cout << "samp_index : " << (int)(samp_index+0.5) << " samplist value: " << sampList[(int)(samp_index+0.5)] << "\n";
            }
            samp_extendFlag = sampList[rank];
            // std::cout << "samp_step is " << samp_step << "\n";
            /*
            for (std::vector<bool>::size_type x = 0; x != sampList.size(); x++)
            {
                // std::cout << "test : branch op 2 for loop ";
                sampList[x] = ( ((int)x) % samp_step ) == 0 ? true : false;
                // std::cout << "test : op = 2 , num = "<< x  << " value is :" << sampList[x] << "\n";
            }
            */
            
        }

    }
    
    // init the sample flag of this mpi rank
    samp_ifSamp = sampList[samp_myrank];

}

// generate the samp_ratio adaptively
int ifSampFlag::genRatio(){
    if ( samp_rankall < 1000 ){
        samp_ratio = 100;
    }
    else if ( samp_rankall >= 1000 && samp_rankall < 5000 ){
        samp_ratio = 50;
    }
    else if ( samp_rankall >= 5000 && samp_rankall < 10000 ) {
        samp_ratio = 20;
    }
    else if ( samp_rankall >= 10000 && samp_rankall < 20000 ) {
        samp_ratio = 10;
    }
    else if ( samp_rankall >= 20000 && samp_rankall < 50000 ) {
        samp_ratio = 5;
    }
    else {
        samp_ratio = 1;
    }
    return samp_ratio;
}

void ifSampFlag::dynSamp ( int rank_dest ) {
    // todo : dynamic extend process
    if (rank_dest == -1)
    {
        samp_ifSamp = false;
        return;
    }
    // modify the samp_extendFlag
    if (sampList[rank_dest])
    {
        // std::cout << "rank " << samp_myrank << "jsi_ifsamp " << samp_ifSamp << "\n";
        samp_ifSamp = true;
        //samp_extendFlag = true;
    }
    // todo:
    // the 0 process of each communication
}
