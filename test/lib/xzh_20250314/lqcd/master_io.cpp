#include "master.h"

MPI_Datatype	MPI_SPINOR_FIELD;
MPI_Datatype	MPI_GAUGE_FIELD;

extern int mpi_rank;
extern MPI_Comm cart_comm;
void mpiio_init()
{
	MPI_Type_contiguous(VOLUME*3*2*4,MPI_DOUBLE,&MPI_SPINOR_FIELD);
	MPI_Type_commit(&MPI_SPINOR_FIELD);

	MPI_Type_contiguous(VOLUME*4*3*2*2,MPI_DOUBLE,&MPI_GAUGE_FIELD);
	MPI_Type_commit(&MPI_GAUGE_FIELD);
}

void mpi_read_conf(char* filename,su3_field data)
{

	MPI_File fh;
	MPI_Status status;

	MPI_File_open(cart_comm,filename,MPI_MODE_RDONLY,MPI_INFO_NULL,&fh);
	MPI_File_read(fh,data,1,MPI_GAUGE_FIELD,&status);
	//MPI_File_read_at_all(fh,0,data,1,MPI_GAUGE_FIELD,&status);
	MPI_File_close(&fh);
}