#include "dsp.h"
#include<hthread_device.h>
extern volatile int dsp_comm_flag[NThreads][2];
inline void spinor_vec_zero(spinor_vec d){
    int i,j;
    for(i=0;i<3;++i)
        for(j=0;j<4;++j){
            d[i][j][0] = 0.0;
            d[i][j][1] = 0.0;
        }
}
inline void spinor_vec_copy(spinor_vec d, spinor_vec s){
    int i,j;
    for(i=0;i<3;++i)
        for(j=0;j<4;++j){
            d[i][j][0]=s[i][j][0];
            d[i][j][1]=s[i][j][1];
        }
}
inline void spinor_vec_acc(spinor_vec d, spinor_vec s){
    int i,j;
    for(i=0;i<3;++i)
        for(j=0;j<4;++j){
            d[i][j][0]+=s[i][j][0];
            d[i][j][1]+=s[i][j][1];
        }
}
void spinor_local_zero(spinor_local d){
    int i,j;
    for(i=0;i<NY;++i) 
        for(j=0;j<NX/16;++j)
            spinor_vec_zero(d[i][j]);
}

void spinor_local_copy(spinor_local d, spinor_local s){
    int i,j;
    for(i=0;i<NY;++i) 
        for(j=0;j<NX/16;++j)
            spinor_vec_copy(d[i][j],s[i][j]);
}

void spinor_local_acc(spinor_local d, spinor_local s){
    int i,j;
    for(i=0;i<NY;++i) 
        for(j=0;j<NX/16;++j)
            spinor_vec_acc(d[i][j],s[i][j]);
}

void spinor_local_shift(spinor_local d, spinor_local s, int dim, t_dir dir){
    int i,j,k,l;
    int thread_id = get_thread_id();
    if(dim==1){ //z
        if(dir==e_forward){
            if(thread_id!=0){
                while(!dsp_comm_flag[thread_id][e_forward]);//若缓冲区不为空
                dma_push(s,dsp_comm[thread_id][e_forward].data,sizeof(spinor_local),1,0);//发送
                dsp_comm_flag[thread_id][e_forward] = 0;
            }
            if(thread_id==NThreads-1){
                spinor_local_zero(d);
            }else{
                while(dsp_comm_flag[thread_id+1][e_forward]);//若缓冲区为空
                dma_pull(d,dsp_comm[thread_id+1][e_forward].data,sizeof(spinor_local),1,0);//接收
                dsp_comm_flag[thread_id+1][e_forward] = 1;
            }
        }else {
            if(thread_id!=NThreads-1){
                while(!dsp_comm_flag[thread_id][e_backward]);
                dma_push(s,dsp_comm[thread_id][e_backward].data,sizeof(spinor_local),1,0);
                dsp_comm_flag[thread_id][e_backward] = 0;
            }
            if(thread_id==0){
                spinor_local_zero(d);
            }else{
                while(dsp_comm_flag[thread_id-1][e_backward]);
                dma_pull(d,dsp_comm[thread_id-1][e_backward].data,sizeof(spinor_local),1,0);
                dsp_comm_flag[thread_id-1][e_backward] = 1;
            }    
        }
    }else if(dim==3){ //x  
        if(dir==e_forward){
            for(i=0;i<NY;++i){
                for(j=0;j<NX/16-1;++j)
                    for(k=0;k<3;++k)
                        for(l=0;l<4;++l){
                            d[i][j][k][l][0] = vec_ld(1,&s[i][j][k][l][0]);
                            d[i][j][k][l][1] = vec_ld(1,&s[i][j][k][l][1]);
                            mov_to_vlr(0x8000);//关闭VPE0-VPE14，故接下来的操作只影响向量中的第15个数
                            d[i][j][k][l][0] = vec_ld(1,&s[i][j+1][k][l][0]-1);
                            d[i][j][k][l][1] = vec_ld(1,&s[i][j+1][k][l][1]-1);
                            mov_to_vlr(0xFFFF);//打开所有的VPE
                        }
                for(k=0;k<3;++k)
                    for(l=0;l<4;++l){
                        d[i][j][k][l][0] = vec_ld(1,&s[i][j][k][l][0]);
                        d[i][j][k][l][1] = vec_ld(1,&s[i][j][k][l][1]);
                        mov_to_vlr(0x8000);//关闭VPE0-VPE14，故接下来的操作只影响向量中的第15个数
                        vec_st(vec_movi(0.0),0,&d[i][j][k][l][0]);//    d[i][j][k][0] = vec_ld(1,&s[i][j+1][k][l][0]-1);
                        vec_st(vec_movi(0.0),0,&d[i][j][k][l][1]);//    d[i][j][k][1] = vec_ld(1,&s[i][j+1][k][l][1]-1);
                        mov_to_vlr(0xFFFF);//打开所有的VPE
                    }
            }

        }else{
            for(i=0;i<NY;++i){
                for(j=NX/16-1;j>0;--j){
                    for(k=0;k<3;++k)
                        for(l=0;l<4;++l){
                            d[i][j][k][l][0] = vec_ld(15,&s[i][j][k][l][0]-1);
                            d[i][j][k][l][1] = vec_ld(15,&s[i][j][k][l][1]-1);
                            //if(thread_id==0&&i==0&&j==1&&k==0&&l==0) print_lvector_double(d[i][j][k][l][0]);
                            mov_to_vlr(0x0001);//关闭VPE1-VPE15，故接下来的操作只影响向量中的第0个数
                            d[i][j][k][l][0] = vec_ld(15,&s[i][j-1][k][l][0]);
                            d[i][j][k][l][1] = vec_ld(15,&s[i][j-1][k][l][1]);
                            mov_to_vlr(0xFFFF);//打开所有的VPE
                            //if(thread_id==0&&i==0&&j==1&&k==0&&l==0) print_lvector_double(d[i][j][k][l][0]);
                        }
                }
                for(k=0;k<3;++k)
                    for(l=0;l<4;++l){
                        d[i][j][k][l][0] = vec_ld(15,&s[i][j][k][l][0]-1);
                        d[i][j][k][l][1] = vec_ld(15,&s[i][j][k][l][1]-1);
                        //if(thread_id==0&&i==0&&j==0&&k==0&&l==3) print_lvector_double(d[i][j][k][l][0]);
                        mov_to_vlr(0x0001);//关闭VPE1-VPE15，故接下来的操作只影响向量中的第0个数
                        vec_st(vec_movi(0.0),0,&d[i][j][k][l][0]);                            
                        vec_st(vec_movi(0.0),0,&d[i][j][k][l][1]);
                        mov_to_vlr(0xFFFF);//打开所有的VPE
                        //if(thread_id==0&&i==0&&j==0&&k==0&&l==3) print_lvector_double(d[i][j][k][l][0]);
                    }
            }
        }
    }
}
