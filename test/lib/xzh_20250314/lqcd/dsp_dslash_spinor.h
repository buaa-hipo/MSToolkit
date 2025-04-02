#ifdef FLAG_DIR
#define GAMMA_OP -
#define DAG _
#else
#define GAMMA_OP +
#define DAG _dag_
#endif

//宏定义函数，根据不同的FLAG_DIR和FLAG_GAMMA产生不同的函数名
#define MACRO_FUN_NAME__(dag, gam) spinor_acc_gamma ## gam ## dag ## mul
#define MACRO_FUN_NAME_(dag, gam) MACRO_FUN_NAME__(dag,gam)
#define MACRO_FUN_NAME MACRO_FUN_NAME_(DAG, FLAG_GAMMA)
//求link的第三行
//dst_re = src1_re*src2_re-src1_im*src2_im-src3_re*src4_re+src3_im*src4_im
//dst_im = src3_im*src4_re+src3_re*src4_im-src1_im*src2_re-src1_re*src2_im
#define CAL_LINK(dst, src1, src2, src3, src4){\
    link ## dst ## _re = vec_mula(link ## src1 ## _im,link ## src2 ## _im,link ## dst ## _re);\
    link ## dst ## _re = vec_mula(link ## src3 ## _re,link ## src4 ## _re,link ## dst ## _re);\
    link ## dst ## _re = vec_mulb(link ## src1 ## _re,link ## src2 ## _re,link ## dst ## _re);\
    link ## dst ## _re = vec_mula(link ## src3 ## _im,link ## src4 ## _im,link ## dst ## _re);\
    link ## dst ## _im = vec_mula(link ## src1 ## _im,link ## src2 ## _re,link ## dst ## _im);\
    link ## dst ## _im = vec_mula(link ## src1 ## _re,link ## src2 ## _im,link ## dst ## _im);\
    link ## dst ## _im = vec_mulb(link ## src3 ## _im,link ## src4 ## _re,link ## dst ## _im);\
    link ## dst ## _im = vec_mula(link ## src3 ## _re,link ## src4 ## _im,link ## dst ## _im);\
}

#ifdef FLAG_DIR
    #define CAL_DST__(dst_idx,link_idx){\
        dst ## dst_idx ## _re = vec_mulb(link ## link_idx ## _im,src_im,dst ## dst_idx ## _re);\
        dst ## dst_idx ## _re = vec_mulb(link ## link_idx ## _re,src_re,dst ## dst_idx ## _re);\
        dst ## dst_idx ## _im = vec_mula(link ## link_idx ## _re,src_im,dst ## dst_idx ## _im);\
        dst ## dst_idx ## _im = vec_mula(link ## link_idx ## _im,src_re,dst ## dst_idx ## _im);\
    }
#else
    #define CAL_DST__(dst_idx,link_idx){\
        dst ## dst_idx ## _re = vec_mula(link ## link_idx ## _re,src_re,dst ## dst_idx ## _re);\
        dst ## dst_idx ## _re = vec_mula(link ## link_idx ## _im,src_im,dst ## dst_idx ## _re);\
        dst ## dst_idx ## _im = vec_mulb(link ## link_idx ## _im,src_re,dst ## dst_idx ## _im);\
        dst ## dst_idx ## _im = vec_mulb(link ## link_idx ## _re,src_im,dst ## dst_idx ## _im);\
    }
#endif

#define CAL_DST_(dst_idx1,dst_idx2,dst_idx3,link_idx1,link_idx2,link_idx3){\
    src_re = vec_ld(0,psrc++);\
    src_im = vec_ld(0,psrc++);\
    CAL_DST__(dst_idx1,link_idx1);\
    CAL_DST__(dst_idx2,link_idx2);\
    CAL_DST__(dst_idx3,link_idx3);\
}

#define CAL_DST(link_idx1,link_idx2,link_idx3){\
    CAL_DST_(0,4,8,link_idx1,link_idx2,link_idx3);\
    CAL_DST_(1,5,9,link_idx1,link_idx2,link_idx3);\
    CAL_DST_(2,6,10,link_idx1,link_idx2,link_idx3);\
    CAL_DST_(3,7,11,link_idx1,link_idx2,link_idx3);\
}
#define MACRO_GAMMA0(idx0,idx1,idx2,idx3){\
  dst ## idx0 ## _re= dst ## idx0 ## _re - (GAMMA_OP dst ## idx3 ## _im); \
  dst ## idx0 ## _im = dst ## idx0 ## _im + (GAMMA_OP dst ## idx3 ## _re); \
  dst ## idx3 ## _re = (GAMMA_OP dst ## idx0 ## _im);                             \
  dst ## idx3 ## _im = -(GAMMA_OP dst ## idx0 ## _re);                              \
  dst ## idx1 ## _re = dst ## idx1 ## _re - (GAMMA_OP dst ## idx2 ## _im); \
  dst ## idx1 ## _im = dst ## idx1 ## _im + (GAMMA_OP dst ## idx2 ## _re); \
  dst ## idx2 ## _re = (GAMMA_OP dst ## idx1 ## _im);                             \
  dst ## idx2 ## _im = -(GAMMA_OP dst ## idx1 ## _re);                              }


#define MACRO_GAMMA1(idx0,idx1,idx2,idx3){\
  dst ## idx0 ## _re = dst ## idx0 ## _re - (GAMMA_OP dst ## idx3 ## _re); \
  dst ## idx0 ## _im = dst ## idx0 ## _im - (GAMMA_OP dst ## idx3 ## _im); \
  dst ## idx3 ## _re = -(GAMMA_OP dst ## idx0 ## _re);                             \
  dst ## idx3 ## _im = -(GAMMA_OP dst ## idx0 ## _im);                             \
  dst ## idx1 ## _re = dst ## idx1 ## _re + (GAMMA_OP dst ## idx2 ## _re); \
  dst ## idx1 ## _im = dst ## idx1 ## _im + (GAMMA_OP dst ## idx2 ## _im); \
  dst ## idx2 ## _re = GAMMA_OP dst ## idx1 ## _re;                              \
  dst ## idx2 ## _im = GAMMA_OP dst ## idx1 ## _im;}

#define MACRO_GAMMA2(idx0,idx1,idx2,idx3){\
  dst ## idx0 ## _re = dst ## idx0 ## _re - (GAMMA_OP dst ## idx2 ## _im); \
  dst ## idx0 ## _im = dst ## idx0 ## _im + (GAMMA_OP dst ## idx2 ## _re); \
  dst ## idx2 ## _re = (GAMMA_OP dst ## idx0 ## _im);                             \
  dst ## idx2 ## _im = -(GAMMA_OP dst ## idx0 ## _re);                              \
  dst ## idx1 ## _re = dst ## idx1 ## _re + (GAMMA_OP dst ## idx3 ## _im); \
  dst ## idx1 ## _im = dst ## idx1 ## _im - (GAMMA_OP dst ## idx3 ## _re); \
  dst ## idx3 ## _re = -(GAMMA_OP dst ## idx1 ## _im);                              \
  dst ## idx3 ## _im = GAMMA_OP dst ## idx1 ## _re;}


#define MACRO_GAMMA3(idx0,idx1,idx2,idx3){\
  dst ## idx0 ## _re = dst ## idx0 ## _re + (GAMMA_OP dst ## idx2 ## _re); \
  dst ## idx0 ## _im = dst ## idx0 ## _im + (GAMMA_OP dst ## idx2 ## _im); \
  dst ## idx2 ## _re = GAMMA_OP dst ## idx0 ## _re;                              \
  dst ## idx2 ## _im = GAMMA_OP dst ## idx0 ## _im;                              \
  dst ## idx1 ## _re = dst ## idx1 ## _re + (GAMMA_OP dst ## idx3 ## _re); \
  dst ## idx1 ## _im = dst ## idx1 ## _im + (GAMMA_OP dst ## idx3 ## _im); \
  dst ## idx3 ## _re = GAMMA_OP dst ## idx1 ## _re;                              \
  dst ## idx3 ## _im = GAMMA_OP dst ## idx1 ## _im;}

#define MACRO_GAMMA__(gamma) {\
  MACRO_GAMMA ## gamma (0,1,2,3)\
  MACRO_GAMMA ## gamma (4,5,6,7)\
  MACRO_GAMMA ## gamma (8,9,10,11)}

#define MACRO_GAMMA_(gamma) MACRO_GAMMA__(gamma)

#define MACRO_GAMMA MACRO_GAMMA_(FLAG_GAMMA)

void MACRO_FUN_NAME(spinor_vec dst, su3_vec link, spinor_vec src){
    lvector FLOAT *psrc = (lvector FLOAT*)src;
    lvector FLOAT *plink = (lvector FLOAT*)link;
    register lvector FLOAT src_re,src_im;
    register lvector FLOAT dst0_re = 0, dst1_re = 0,dst2_re = 0,dst3_re = 0,
      dst4_re = 0,dst5_re = 0,dst6_re = 0,dst7_re = 0,dst8_re = 0,dst9_re = 0,
      dst10_re = 0,dst11_re = 0;
    register lvector FLOAT dst0_im = 0, dst1_im = 0,dst2_im = 0,dst3_im = 0,
      dst4_im = 0,dst5_im = 0,dst6_im = 0,dst7_im = 0,dst8_im = 0,dst9_im = 0,
      dst10_im = 0,dst11_im = 0;
    register lvector FLOAT link0_re,link1_re,link2_re,link3_re,link4_re,link5_re,link6_re,link7_re,link8_re;
    register lvector FLOAT link0_im,link1_im,link2_im,link3_im,link4_im,link5_im,link6_im,link7_im,link8_im;

    link0_re = vec_ld(0,plink);
    link0_im = vec_ld(16,plink);
    link1_re = vec_ld(32,plink);
    link1_im = vec_ld(48,plink);
    link2_re = vec_ld(64,plink);
    link2_im = vec_ld(80,plink);
    link3_re = vec_ld(96,plink);
    link3_im = vec_ld(112,plink);
    link4_re = vec_ld(128,plink);
    link4_im = vec_ld(144,plink);
    link5_re = vec_ld(160,plink);
    link5_im = vec_ld(176,plink);
    link6_re = 0;
    link6_im = 0;
    link7_re = 0;
    link7_im = 0;
    link8_re = 0;
    link8_im = 0;

    CAL_LINK(6,1,5,2,4);
    CAL_LINK(7,2,3,0,5);
    CAL_LINK(8,0,4,1,3);

#ifdef FLAG_DIR
    CAL_DST(0,3,6)
    CAL_DST(1,4,7)
    CAL_DST(2,5,8)
#else
    CAL_DST(0,1,2)
    CAL_DST(3,4,5)
    CAL_DST(6,7,8)
#endif
    MACRO_GAMMA;
    psrc = (lvector FLOAT*) dst;
    vec_st(dst0_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst0_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst1_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst1_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst2_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst2_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst3_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst3_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst4_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst4_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst5_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst5_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst6_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst6_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst7_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst7_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst8_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst8_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst9_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst9_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst10_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst10_im+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst11_re+vec_ld(0,psrc),0,psrc);psrc++;
    vec_st(dst11_im+vec_ld(0,psrc),0,psrc);
}

#undef GAMMA_OP
#undef DAG
#undef CAL_DST
#undef CAL_DST_
#undef CAL_DST__
#undef CAL_LINK
#undef MACRO_GAMMA
#undef MACRO_GAMMA_
#undef MACRO_GAMMA__
#undef MACRO_GAMMA0
#undef MACRO_GAMMA1
#undef MACRO_GAMMA2
#undef MACRO_GAMMA3
#undef MACRO_FUN_NAME
#undef MACRO_FUN_NAME_
#undef MACRO_FUN_NAME__
