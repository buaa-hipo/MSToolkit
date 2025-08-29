#pragma once
#include <stdio.h>
class Recorder {

};
class SequentialRecorder {
    enum RecordType {None};
    FILE* f;
    void extract(RecordType type);
    template<RecordType rt>
    class Iterator {
        
    };


};