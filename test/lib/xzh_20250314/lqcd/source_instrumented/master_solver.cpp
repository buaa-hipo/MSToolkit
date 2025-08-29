#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "master.h"

#include "instrument/instrumented_func_host.h"
#include "record/mt_callback_defs.h"

#include <unistd.h>


extern int              cluster_id, group_id;
extern int              mpi_rank;
extern master_transfer* mtransfer;
extern double*          reduce_buffer;
double                  residual_square, coef_g[2];
extern spinor_sliceX *  dslash_backward_spinor, *dslash_forward_spinor;

void print_spinor(spinor_field s) {
    int i, j, k;
    printf("\n");
    for (int it = 0; it < NT; it++)
        for (int iz = 0; iz < NZ; iz++)
            for (int iy = 0; iy < NY; iy++)
                for (int ix = 0; ix < NX; ix++) {
                    for (int a = 0; a < 3; ++a) {
                        for (int b = 0; b < 4; ++b) {
                            printf("%lf ", s[it][iz][iy][ix / 16][a][b][0][ix % 16]);
                        }
                        printf("\n");
                    }
                    printf("\n");
                    for (int a = 0; a < 3; ++a) {
                        for (int b = 0; b < 4; ++b) {
                            printf("%lf ", s[it][iz][iy][ix / 16][a][b][1][ix % 16]);
                        }
                        printf("\n");
                    }
                    return;
                }
}

double cal(spinor_field s) {
    double ret = 0.0;
    for (int it = 0; it < 1; ++it)
        for (int iz = 0; iz < 1; ++iz)
            for (int iy = 0; iy < 4; ++iy)
                for (int ix = 0; ix < NX; ++ix)
                    for (int a = 0; a < 3; ++a)
                        for (int b = 0; b < 4; ++b)
                            for (int r = 0; r < 2; ++r) {
                                ret +=
                                    s[it][iz][iy][ix >> 4][a][b][r][ix & 15] * s[it][iz][iy][ix >> 4][a][b][r][ix & 15];
                            }

    return ret;
}

void print_su3(su3_field s, int dim) {
    int i, j, k;
    printf("\n");
    for (int it = 0; it < NT; it++)
        for (int iz = 15; iz < NZ; iz++)
            for (int iy = 31; iy < NY; iy++)
                for (int ix = 1; ix < NX; ix++) {
                    for (int a = 0; a < 2; ++a) {
                        for (int b = 0; b < 3; ++b) {
                            printf("%lf ", s[dim][it][iz][iy][ix / 16][a][b][0][ix % 16]);
                        }
                        printf("\n");
                    }
                    printf("\n");
                    for (int a = 0; a < 2; ++a) {
                        for (int b = 0; b < 3; ++b) {
                            printf("%lf ", s[dim][it][iz][iy][ix / 16][a][b][1][ix % 16]);
                        }
                        printf("\n");
                    }
                    return;
                }
}

void m_wilson(spinor_field dst, su3_field link, spinor_field src) {
    dslash(dst, link, src);

    reduce1(dst, src);
    // print_spinor(dst);
    //    printf("%lf   %lf\n",residual_square,cal(dst));
}

void init_spinor(spinor_field src) {
    for (int it = 0; it < NT; ++it)
        for (int iz = 0; iz < NZ; ++iz)
            for (int a = 0; a < 3; ++a)
                for (int b = 0; b < 4; ++b)
                    for (int r = 0; r < 2; ++r)
                        for (int iy = 0; iy < NY; ++iy) {
                            dslash_forward_spinor[0][it][iz][iy >> 4][a][b][r][iy & 15] =
                                src[it][iz][iy][0][a][b][r][0];
                            dslash_backward_spinor[0][it][iz][iy >> 4][a][b][r][iy & 15] =
                                src[it][iz][iy][1][a][b][r][15];
                        }
    uint64_t args[2];
    args[0] = (uint64_t)dslash_backward_spinor;
    args[1] = (uint64_t)dslash_forward_spinor;
    instrumented_hthread_group_exec(group_id, "init_gsm", 0, 2, args);
    instrumented_hthread_group_wait(group_id);
}

void mr_solver(spinor_field dst, su3_field link, spinor_field src, spinor_field aux, spinor_field src0) {
    double *coef = (double*)instrumented_hthread_malloc(cluster_id, 2 * sizeof(double), HT_MEM_RW), residual;
    memset(dst, 0, sizeof(spinor_field));
    memset(aux, 0, sizeof(spinor_field));
    residual = 1.0;
    init_spinor(src);
    for (int count = 0; (count < max_iter) && (residual > tolerance); ++count) {
        // if (count == 204) sleep(100);
        m_wilson(aux, link, src);
        coef[0] = coef_g[0] * omega / residual_square;
        coef[1] = coef_g[1] * omega / residual_square;
        reduce2(coef, dst, src, aux);

        residual = sqrt(residual_square);

        if (mpi_rank == 0) printf("iteration: %d, residual: %.16e \n", count, residual);
        residual = 1.0;
        // print_spinor(src);
        // if(count==3) return;
    }
    instrumented_hthread_free(coef);
}
