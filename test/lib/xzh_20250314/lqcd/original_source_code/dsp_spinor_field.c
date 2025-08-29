#include<hthread_device.h>
#include<compiler/m3000.h>
#include<compiler/vsip.h>
#include"dsp.h"

extern double residual_square, coef_g[2];
extern int flag_residual,flag_coef;

void spinor_field_zero(spinor_field dst,Para* local_para){
    int i,j,k,l;

    for(i=0;i<NY;++i)
        for(j=0;j<NX/16;++j)
            for(k=0;k<3;++k)
                for(l=0;l<4;++l){
                    local_para->local_ferm[0][i][j][k][l][0] = vec_movi(0.0);
                    local_para->local_ferm[0][i][j][k][l][1] = vec_movi(0.0);
                }
    
    for(i=0;i<NT;++i)
            dma_push(local_para->local_ferm[0],dst[i][local_para->thread_id],sizeof(spinor_local),1,0);
}

void pre_cal_sum(Para* local_para){
    if(local_para->flag_coef){
        local_para->coef_g[0] = 0;
        local_para->coef_g[1] = 0;
    }
    if(local_para->flag_residual) local_para->residual_square = 0;
}

double vsumval_double(lvector double lvd){
    double sum = 0.0;
    mov_to_svr_v16df(lvd);
    sum += mov_from_svr0_df();
    sum += mov_from_svr1_df();
    sum += mov_from_svr2_df();
    sum += mov_from_svr3_df();
    sum += mov_from_svr4_df();
    sum += mov_from_svr5_df();
    sum += mov_from_svr6_df();
    sum += mov_from_svr7_df();
    sum += mov_from_svr8_df();
    sum += mov_from_svr9_df();
    sum += mov_from_svr10_df();
    sum += mov_from_svr11_df();
    sum += mov_from_svr12_df();
    sum += mov_from_svr13_df();
    sum += mov_from_svr14_df();
    sum += mov_from_svr15_df();
    return sum;
}


void cal_sum(Para* local_para){
    lvector FLOAT *rebuf,*imbuf;
    rebuf = &(local_para->local_ferm[2][0][0][0][0][0]);
    imbuf = &(local_para->local_ferm[2][0][0][0][0][1]);
    int i,j,k,l;
    if(local_para->flag_coef){
        *rebuf = 0;
        *imbuf = 0;
        for(i=0;i<NY;++i)
            for(j=0;j<NX/16;++j)
                for(k=0;k<3;++k)
                    for(l=0;l<4;++l){
                        *rebuf = vec_mula(local_para->local_ferm[0][i][j][k][l][0],local_para->local_ferm[1][i][j][k][l][0],*rebuf);
                        *rebuf = vec_mula(local_para->local_ferm[0][i][j][k][l][1],local_para->local_ferm[1][i][j][k][l][1],*rebuf);
                        *imbuf = vec_mulb(local_para->local_ferm[0][i][j][k][l][0],local_para->local_ferm[1][i][j][k][l][1],*imbuf);
                        *imbuf = vec_mulb(local_para->local_ferm[0][i][j][k][l][1],local_para->local_ferm[1][i][j][k][l][0],*imbuf);
                    }
        local_para->coef_g[1] += vsip_vsumval_d_v(imbuf,16);
        local_para->coef_g[0] += vsip_vsumval_d_v(rebuf,16);
        //local_para->coef_g[1] += vsumval_double(*imbuf);
    }
    

    if(local_para->flag_residual){
        *rebuf = 0;
        for(i=0;i<NY;++i)
            for(j=0;j<NX/16;++j)
                for(k=0;k<3;++k)
                    for(l=0;l<4;++l){
                        *rebuf = vec_mula(local_para->local_ferm[0][i][j][k][l][0],local_para->local_ferm[0][i][j][k][l][0],*rebuf);
                        *rebuf = vec_mula(local_para->local_ferm[0][i][j][k][l][1],local_para->local_ferm[0][i][j][k][l][1],*rebuf);
                }
        local_para->residual_square += vsip_vsumval_d_v(rebuf,NX);
    }
}

void spinor_field_acc_sub(spinor_field dst,spinor_field src, Para* local_para){
    int i,j,k;
    lvector FLOAT *p,*q;
    pre_cal_sum(local_para);
    for(i=0;i<NT;++i){
        p = (lvector FLOAT*) (local_para->local_ferm[0]);
        q = (lvector FLOAT*) (local_para->local_ferm[1]);
        dma_pull(local_para->local_ferm[0],dst[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        dma_pull(local_para->local_ferm[1],src[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        for(j=0;j<3*4*2*NY*NX/16;++j){
            p[j] = p[j] - q[j];
        }
        int dma_handle = dma_push_async(local_para->local_ferm[0],dst[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        cal_sum(local_para);
        dma_wait(dma_handle);
    }
}


void gather_backward_forward(double backward_spinor_sm[4][2][3][4][2][16], double forward_spinor_sm[4][2][3][4][2][16],spinor_local local_ferm, int idx){
    for(int a = 0; a < 3; ++a)
        for(int b = 0; b < 4; ++b)
            for(int r = 0; r < 2; ++r)
                for(int iy = 0; iy < NY; ++iy){
                    mov_to_svr_v16df(local_ferm[iy][0][a][b][r]);
                    forward_spinor_sm[idx][iy>>4][a][b][r][iy&15] = mov_from_svr0_df();
                    mov_to_svr_v16df(local_ferm[iy][1][a][b][r]);
                    backward_spinor_sm[idx][iy>>4][a][b][r][iy&15] = mov_from_svr15_df();
                }


}
void spinor_field_acc_complex_mul(spinor_field dst, const double coef[2], spinor_field src, Para* local_para,spinor_sliceX backward_spinor, spinor_sliceX forward_spinor){
    int i,j;
    lvector FLOAT *p,*q;
    lvector FLOAT *coef_buf0 = &(local_para->local_link[0][0][0][0][0][0]),*coef_buf1 =  &(local_para->local_link[0][0][0][0][0][1]);
    coef_buf0[0] = vec_svbcast(coef[0]);
    coef_buf1[0] = vec_svbcast(coef[1]);
    double backward_spinor_sm[4][2][3][4][2][16],forward_spinor_sm[4][2][3][4][2][16];
    int dma_handle_sm0[2],dma_handle_sm1[2];
    spinor_sliceX* gsm_mem = (spinor_sliceX*)dsp_comm;
    int handle_idx = 0;
    pre_cal_sum(local_para);
    
    for(i=0;i<NT;++i){
        p = (lvector FLOAT*) (local_para->local_ferm[0]);
        q = (lvector FLOAT*) (local_para->local_ferm[1]);
        dma_pull(local_para->local_ferm[0],dst[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        dma_pull(local_para->local_ferm[1],src[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        for(j=0;j<3*4*NY*2*NX/16;j+=2){
            p[j] = vec_mulb(q[j+1],*coef_buf1,p[j]);
            p[j] = vec_mulb(q[j],*coef_buf0,p[j]);
            p[j+1] = vec_mula(q[j+1],*coef_buf0,p[j+1]);
            p[j+1] = vec_mula(q[j],*coef_buf1,p[j+1]);
        }
        int dma_handle = dma_push_async(local_para->local_ferm[0],dst[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        if(backward_spinor!=NULL){
            if(i>3&&!(i&1)) {
                dma_wait(dma_handle_sm0[handle_idx]);
                dma_wait(dma_handle_sm1[handle_idx]);
            }
            gather_backward_forward(backward_spinor_sm,forward_spinor_sm,local_para->local_ferm[0],i&3);
            if(i&1) {
                dma_handle_sm0[handle_idx] = dma_push_async(backward_spinor_sm[i&3-1],gsm_mem[0][i-1][local_para->thread_id],NY/16*sizeof(spinor_vec),2,(NZ-1)*NY/16*sizeof(spinor_vec));
                dma_handle_sm1[handle_idx] = dma_push_async(forward_spinor_sm[i&3-1],gsm_mem[1][i-1][local_para->thread_id],NY/16*sizeof(spinor_vec),2,(NZ-1)*NY/16*sizeof(spinor_vec));
                handle_idx = 1 - handle_idx;
            }
        }
        cal_sum(local_para);
        dma_wait(dma_handle);
    }
    if(backward_spinor!=NULL){
        dma_wait(dma_handle_sm0[handle_idx]);
        dma_wait(dma_handle_sm1[handle_idx]);
        dma_wait(dma_handle_sm0[1-handle_idx]);
        dma_wait(dma_handle_sm1[1-handle_idx]);
    }
}
void spinor_field_acc_linear_combine_real(spinor_field dst, spinor_field src, double coef_dst, double coef_s, Para* local_para){
    int i,j;
    lvector FLOAT *p,*q;
    pre_cal_sum(local_para);
    spinor_sliceX* gsm_mem = (spinor_sliceX*)dsp_comm;
    for(i=0;i<NT;++i){
        p = (lvector FLOAT*) (local_para->local_ferm[0]);
        q = (lvector FLOAT*) (local_para->local_ferm[1]);
        dma_pull(local_para->local_ferm[0],dst[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        dma_pull(local_para->local_ferm[1],src[i][local_para->thread_id][0],sizeof(spinor_local),1,0);


        {
            for(int iy = 0; iy < NY; ++iy){
                for(int ia = 0; ia < 3; ++ia)
                    for(int ib = 0; ib < 4; ++ib)
                        for(int ir = 0; ir < 2; ++ir){
                            lvector FLOAT tmp = vec_svbcast(gsm_mem[0][i][local_para->thread_id][iy>>4][ia][ib][ir][iy&15]);
                            mov_to_vlr(0x0001);
                            local_para->local_ferm[0][iy][0][ia][ib][ir] += tmp;
                            mov_to_vlr(0xFFFF);
                            local_para->local_ferm[0][iy][0][ia][ib][ir] = (local_para->local_ferm[0][iy][0][ia][ib][ir])*coef_dst + (local_para->local_ferm[1][iy][0][ia][ib][ir])*coef_s;

                            //tmp = vec_svbcast(gsm_mem[NT*NZ*NY*24+i*NZ*NY*24+(local_para->thread_id)*NY*24+(iy>>4)*24*16+ia*8*16+ib*2*16+ir*16+(iy&15)]);
                            tmp = vec_svbcast(gsm_mem[1][i][local_para->thread_id][iy>>4][ia][ib][ir][iy&15]);
                            mov_to_vlr(0x8000);
                            local_para->local_ferm[0][iy][1][ia][ib][ir] += tmp;
                            mov_to_vlr(0xFFFF);

                            local_para->local_ferm[0][iy][1][ia][ib][ir] = (local_para->local_ferm[0][iy][1][ia][ib][ir])*coef_dst + (local_para->local_ferm[1][iy][1][ia][ib][ir])*coef_s;
                        }
            }
                
        }

        /*for(j=0;j<3*4*NY*2*NX/16;++j){
            p[j] = p[j]*coef_dst+q[j]*coef_s;
        }*/
        int dma_handle = dma_push_async(local_para->local_ferm[0],dst[i][local_para->thread_id][0],sizeof(spinor_local),1,0);
        cal_sum(local_para);
        dma_wait(dma_handle);
    }
}

void print_lvector_double(lvector double lv){
    mov_to_svr_v16df(lv);
    mov_from_svr0_df();
}
void print_su3_vec(su3_vec sv){
    int a,b;
    for(a=0;a<2;++a){
        for(b=0;b<3;++b)
            print_lvector_double(sv[a][b][0]);
    }
    for(a=0;a<2;++a){
        for(b=0;b<3;++b)
            print_lvector_double(sv[a][b][1]);
        hthread_printf("\n");
    }
    hthread_printf("\n");

}
void print_spinor_vec(spinor_vec sv){
    int a,b;
    for(a=0;a<3;++a){
        for(b=0;b<4;++b)
            print_lvector_double(sv[a][b][0]);
        hthread_printf("\n");
    }
    hthread_printf("\n");
    for(a=0;a<3;++a){
        for(b=0;b<4;++b)
            print_lvector_double(sv[a][b][1]);
        hthread_printf("\n");
    }
    hthread_printf("\n");
}
void print_spinor(spinor_field s, Para* local_para){
    dma_pull(local_para->local_ferm[0],s[0][local_para->thread_id][0],sizeof(spinor_local),1,0);
    int it,iz,iy,ix;
    print_spinor_vec(local_para->local_ferm[0][0]);
}
