#include <stdio.h>
#include <random>

#define NT        16
#define NZ        16
#define NY        32
#define NX        32

typedef double FLOAT;
typedef FLOAT su3_vec[2][3][2][16];
typedef su3_vec    su3_field[4][NT][NZ][NY][NX / 16];

// 生成示例数据并写入文件
void generate_and_write_file(const char *filename) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 10.0);
    su3_field u;
    // 初始化示例数据
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < NT; j++) {
            for (int k = 0; k < NZ; k++) {
                for (int l = 0; l < NY; l++) {
                    for (int m = 0; m < NX / 16; m++) {
                        for (int n = 0; n < 2; n++) {
                            for (int p = 0; p < 3; p++) {
                                for (int q = 0; q < 2; q++) {
                                    for (int r = 0; r < 16; r++) {
                                        u[i][j][k][l][m][n][p][q][r] = dis (gen);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 打开文件以二进制写入模式
    FILE *fptr = fopen(filename, "wb");
    if (fptr == NULL) {
        printf("Error opening file for writing.\n");
        exit(1);
    }

    // 写入数据
    fwrite(&u, sizeof(su3_field), 1, fptr);

    // 关闭文件
    fclose(fptr);
}

int main() {
    const char *filename = "test16x16x32x32.cfg";
    generate_and_write_file(filename);
    printf("File generated successfully: %s\n", filename);
    return 0;
}