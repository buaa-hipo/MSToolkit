#include "master.h"
#include "cstdio"
#include "cstdlib"

void read_conf(char filename[],su3_field* u){
    FILE* fptr = fopen(filename,"rb");
    if(fptr==NULL){
        printf("Error in read su3 file.\n");
        exit(1);
    }
    fread(u,sizeof(su3_field),1,fptr);
    fclose(fptr);
}
void write_ferm_field(spinor_field s,char* filename){
	FILE* ptr = fopen(filename, "wb");
        if(ptr == NULL){
                printf("Error in write\n");
                exit(1);
        }
        fwrite(s, sizeof(spinor_field), 1, ptr);
        fclose(ptr);


}

