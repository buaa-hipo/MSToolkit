#include "dsp.h"

void dslash_spinor_local(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir, spinor_local tmp){
    spinor_local_zero(dst);
    dslash_spinor_local_acc(dst,link,src,dim,dir,tmp);
}

void dslash_spinor_local_acc(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir, spinor_local tmp){
    int i,j,k;
    if(dir==e_forward){
        switch (dim)
        {
        case 0:
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma3_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        case 1:{
            spinor_local_shift(tmp,src,1,e_forward);
	        //if(get_thread_id()==14) print_spinor_vec(tmp[31][1]);
            for(i=0;i<NY;++i)
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma2_mul(dst[i][j],link[i][j],tmp[i][j]);
            break;
        }
        case 2:
            for(i=0;i<NY-1;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma1_mul(dst[i][j],link[i][j],src[i+1][j]);
            break;
        case 3:{
            spinor_local_shift(tmp,src,3,e_forward);
            for(i=0;i<NY;++i)
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma0_mul(dst[i][j],link[i][j],tmp[i][j]);
            break;
        }
        default:
            break;
        }
    } else{
        switch (dim)
        {
        case 0:
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma3_dag_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        case 1:{
            spinor_local_zero(tmp);
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma2_dag_mul(tmp[i][j],link[i][j],src[i][j]);
            spinor_local_shift(tmp,tmp,1,e_backward);
            spinor_local_acc(dst,tmp);
            break;
        }
        case 2:
            for(i=1;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma1_dag_mul(dst[i][j],link[i-1][j],src[i-1][j]);
	    break;
        case 3:{
            spinor_local_zero(tmp);
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma0_dag_mul(tmp[i][j],link[i][j],src[i][j]);
            spinor_local_shift(tmp,tmp,3,e_backward);
            spinor_local_acc(dst,tmp);
            break;
        }
        default:
            break;
        }
    }
}

void dslash_spinor_local_noshift(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir){
    spinor_local_zero(dst);
    dslash_spinor_local_acc_noshift(dst,link,src,dim,dir);
}

void dslash_spinor_local_acc_noshift(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir){
    int i,j,k;
    if(dir==e_forward){
        switch (dim)
        {
        case 0:
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma3_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        case 1:{
            for(i=0;i<NY;++i)
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma2_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        }
        case 2:
            for(i=0;i<NY/2;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma1_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        case 3:{
            for(i=0;i<NY/2;++i)
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma0_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        }
        default:
            break;
        }
    } else{
        switch (dim)
        {
        case 0:
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma3_dag_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        case 1:{
            for(i=0;i<NY;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma2_dag_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        }
        case 2:
            for(i=0;i<NY/2;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma1_dag_mul(dst[i][j],link[i][j],src[i][j]);
	    break;
        case 3:{
            for(i=0;i<NY/2;++i) 
                for(j=0;j<NX/16;++j)
                    spinor_acc_gamma0_dag_mul(dst[i][j],link[i][j],src[i][j]);
            break;
        }
        default:
            break;
        }
    }
}
