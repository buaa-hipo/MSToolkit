#include "master.h"
#include <cstdlib>
#include <cstring>
Data_send Dsend[2][4];
Data_recv Drecv[2][4];
int mpi_size, mpi_rank;
MPI_Comm cart_comm;
master_transfer* mtransfer;
extern int cluster_id;
//初始化笛卡尔拓扑
void torus_comm_init(){
    int i;
    int id,p;
    int cart_p;
    int period[4];
    int cart_dims[4];
    MPI_Comm_rank(comm,&id);
    MPI_Comm_size(comm,&p);
    cart_dims[0] = N0/NT;
    cart_dims[1] = N1/NZ;
    cart_dims[2] = N2/NY;
    cart_dims[3] = N3/NX;

    cart_p = 1;
    for(i=0;i<4;++i) cart_p*=cart_dims[i];
    if (cart_p != p){
		printf("Error! The number of MPI nodes should be %ld, but found %ld.\n", cart_p, p);
		exit(-1);
	}

    //设置维度的周期性
	for(i=0;i<4;i++)
		period[i]=1;

	//生成torus通信域
	MPI_Cart_create(comm,4,cart_dims,period,0,&cart_comm);

	MPI_Comm_rank(cart_comm, &mpi_rank);
	MPI_Comm_size(cart_comm, &mpi_size);
	cluster_id = mpi_rank%4;
}

void transfer_init(){
    int i,j,k;
    int coords[4];
	int data_len[4] = {NZ*NY*NX*24,NT*NY*NX*24,NT*NZ*NX*24,NT*NZ*NY*24};
	MPI_Cart_coords(cart_comm,mpi_rank,4,coords);
	
    for(i=0;i<2;++i)
        for(j=0;j<4;++j){
            Dsend[i][j].cart_id=mpi_rank;
			Drecv[i][j].cart_id=mpi_rank;

			Dsend[i][j].coords=coords;
			Drecv[i][j].coords=coords;

			Dsend[i][j].buf=(FLOAT*)hthread_malloc(cluster_id,data_len[j]*sizeof(FLOAT)+N_align,HT_MEM_RW);
			Dsend[i][j].buf = (FLOAT*)align_f4((char*)Dsend[i][j].buf);
			mtransfer->data_send_recv[0][i][j].ptr = Dsend[i][j].buf;
			Drecv[i][j].buf=(FLOAT*)hthread_malloc(cluster_id,data_len[j]*sizeof(FLOAT)+N_align,HT_MEM_RW);
			Drecv[i][j].buf = (FLOAT*)align_f4((char*)Drecv[i][j].buf);
			mtransfer->data_send_recv[1][i][j].ptr = Drecv[i][j].buf;

			Dsend[i][j].num=data_len[j];
			Drecv[i][j].num=data_len[j];

			Dsend[i][j].dtype=MPI_DOUBLE;
			Drecv[i][j].dtype=MPI_DOUBLE;

			Dsend[i][j].tag=i*4+j;
			Drecv[i][j].tag=i*4+j;

			Dsend[i][j].req=(MPI_Request*)malloc(sizeof(MPI_Request));
			Drecv[i][j].req=(MPI_Request*)malloc(sizeof(MPI_Request));
			Drecv[i][j].sta=(MPI_Status*)malloc(sizeof(MPI_Status));
        }
    for(i=0;i<2;i++)
		for(j=0;j<4;j++)
			for(k=0;k<4;k++)
			{
				Dsend[i][j].dst_coords[k]=Dsend[i][j].coords[k];
				Drecv[i][j].src_coords[k]=Drecv[i][j].coords[k];
			}
	for(j=0;j<4;j++){
		Dsend[e_backward][j].dst_coords[j]++;
		Dsend[e_forward][j].dst_coords[j]--;

		Drecv[e_backward][j].src_coords[j]--;
		Drecv[e_forward][j].src_coords[j]++;
	}
    for(i=0;i<2;i++)
		for(j=0;j<4;j++){
			MPI_Cart_rank(cart_comm,Dsend[i][j].dst_coords,&(Dsend[i][j].dst_cart_id));
			MPI_Cart_rank(cart_comm,Drecv[i][j].src_coords,&(Drecv[i][j].src_cart_id));
		}
}

void mpi_init(){
	torus_comm_init();
	if(hthread_dev_open(cluster_id)!=HT_SUCCESS){ //未启动设备无法申请内存
        char name[256];
		int len = 256;
		MPI_Get_processor_name(name,&len);
		printf("%s failed to open dev!\n",name);
		MPI_Abort(comm, -1);
        return;
    }
	mtransfer = (master_transfer*)hthread_malloc(cluster_id,sizeof(master_transfer),HT_MEM_RW);
	memset(mtransfer,0,sizeof(master_transfer));
	transfer_init();
}

void isend(Data_send* isender){
	MPI_Isend(isender->buf,isender->num,isender->dtype,isender->dst_cart_id,isender->tag,cart_comm,isender->req);
}

void irecv(Data_recv* irecver){
	MPI_Irecv(irecver->buf,irecver->num,irecver->dtype,irecver->src_cart_id,irecver->tag,cart_comm,irecver->req);
}
int mpi_test(MPI_Request* req){
	int flag;
	MPI_Test(req,&flag,MPI_STATUS_IGNORE);
	return flag;
}

void send_data(t_dir dir){
    for(int i = 0; i < 4; ++i) isend(&Dsend[dir][i]);
}

void recv_data(t_dir dir){
    for(int i = 0; i < 4; ++i) irecv(&Drecv[dir][i]);
}

void mpi_wait(t_dir dir){
        for(int i = 0; i < 4; ++i)
			MPI_Wait(Drecv[dir][i].req,MPI_STATUS_IGNORE);
            //while(!mpi_test(Drecv[dir][i].req));
}

