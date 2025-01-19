#include <thread>
#include <chrono>
#include <cstring>

volatile bool terminate = false;
// 1 GB
#define NELEMENTS (1<<30)

// The function we want to execute on the new thread.
void streambench()
{
    char* A = new char[NELEMENTS];
    char* B = new char[NELEMENTS];
    char* C = new char[NELEMENTS];
    // undefined value from memory
    char* x = new char[2];
    while(!terminate) {
        memset(A, x[0], NELEMENTS);
        memset(B, x[1], NELEMENTS);
        for(int i=0; i<NELEMENTS; ++i) {
            C[i] = A[i]+B[i];
        }
    }
    delete[] A;
    delete[] B;
    delete[] C;
}

int main(int argc, char* argv[])
{
    int tnum = atoi(argv[1]);
    int delay = atoi(argv[2]);
    int last = atoi(argv[3]);
	printf("^^^ will start stream after delaying %d seconds\n", delay);
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    // Constructs the new thread and runs it. Does not block execution.
	printf("^^^ Start Stream with %d threads, last for %d seconds\n", tnum, last);
    std::thread** ts = new std::thread*[tnum];
    for (int i=0; i<tnum; ++i) {
       ts[i] = new std::thread(streambench); 
    }
    // Do other things...
    std::this_thread::sleep_for(std::chrono::seconds(last));
	printf("^^^ Terminating stream...\n");
    terminate = true;
    for (int i=0; i<tnum; ++i) {
        ts[i]->join();
        delete ts[i];
    }
    delete[] ts;
	printf("^^^ Terminated\n");
}
