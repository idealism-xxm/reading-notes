/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
 * 专门处理 32 * 32 矩阵的转置
 */
char trans_32_32_desc[] = "Transpose 32 x 32";
void trans_32_32(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, k, l;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            if (i == j) {
                // 如果 i == j ，那么 A 和 B 的块会在对角线重叠
                // 由于 A 和 B 对应位置映射到同一个缓存行，所以必定冲突
                // 为了解决这个冲突，我们先将 A 中的 64 个整型复制到 B 中
                // 保证每行复制时 A 和 B 各只有一次未命中
                for (k = i; k < (i + 8); k++) {
                    v0 = A[k][j];
                    v1 = A[k][j + 1];
                    v2 = A[k][j + 2];
                    v3 = A[k][j + 3];
                    v4 = A[k][j + 4];
                    v5 = A[k][j + 5];
                    v6 = A[k][j + 6];
                    v7 = A[k][j + 7];
                    B[k][j] = v0;
                    B[k][j + 1] = v1;
                    B[k][j + 2] = v2;
                    B[k][j + 3] = v3;
                    B[k][j + 4] = v4;
                    B[k][j + 5] = v5;
                    B[k][j + 6] = v6;
                    B[k][j + 7] = v7;
                }
                // 此时 B 中的 8 * 8 小块全部在缓存中，在 B 中实现原地转置
                for (k = i; k < (i + 8); k++) {
                    for (l = k + 1; l < (j + 8); l++) {
                        v0 = B[k][l];
                        B[k][l] = B[l][k];
                        B[l][k] = v0;
                    }
                }
            } else {
                for (k = i; k < (i + 8); k++) {
                    v0 = A[k][j];
                    v1 = A[k][j + 1];
                    v2 = A[k][j + 2];
                    v3 = A[k][j + 3];
                    v4 = A[k][j + 4];
                    v5 = A[k][j + 5];
                    v6 = A[k][j + 6];
                    v7 = A[k][j + 7];
                    B[j][k] = v0;
                    B[j + 1][k] = v1;
                    B[j + 2][k] = v2;
                    B[j + 3][k] = v3;
                    B[j + 4][k] = v4;
                    B[j + 5][k] = v5;
                    B[j + 6][k] = v6;
                    B[j + 7][k] = v7;
                }
            }
        }
    }
}

/*
 * 专门处理 64 * 64 矩阵的转置
 */
char trans_64_64_desc[] = "Transpose 64 x 64";

/*
 * 将 A[i][j] 的 8 * 8 分块中的前四行复制到 B[j][i] 分块中的前四行
 */
void copy_8_8_pre_4(int i, int j, int M, int N, int A[N][M], int B[M][N]) {
    int k;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    for (k = 0; k < 4; k++) {
        // 由于 A[i][j] 分块和 B[j][i] 分块的前四行都是第一次访问
        // 所以每次循环各有一次未命中
        v0 = A[i + k][j];
        v1 = A[i + k][j + 1];
        v2 = A[i + k][j + 2];
        v3 = A[i + k][j + 3];
        v4 = A[i + k][j + 4];
        v5 = A[i + k][j + 5];
        v6 = A[i + k][j + 6];
        v7 = A[i + k][j + 7];
        B[j + k][i] = v0;
        B[j + k][i + 1] = v1;
        B[j + k][i + 2] = v2;
        B[j + k][i + 3] = v3;
        B[j + k][i + 4] = v4;
        B[j + k][i + 5] = v5;
        B[j + k][i + 6] = v6;
        B[j + k][i + 7] = v7;
    }
    // 综上：这个函数每调用一次，产生 8 个未命中
}

/*
 * 将 B[i][j] 为左上角的 4 * 4 分块进行内部转置
 */
void trans_with_4_4(int i, int j, int M, int N, int B[M][N]) {
    int k, l, tmp;
    for(k = 0; k < 4; k++) {
        for(l = k + 1; l < 4; l++) {
            tmp = B[i + k][j + l];
            B[i + k][j + l] = B[i + l][j + k];
            B[i + l][j + k] = tmp;
        }
    }
    // 由于前面已经访问过 B 的 8 * 8 分块的前四行和 B 分块的右下角，
    // 所以这个函数每次调用都不会产生未命中
}

/*
 * 将 A[i][j] 的 8 * 8 分块中的左下角转置到 B[j][i] 分块中的右上角
 * 同时将 B[j][i] 分块中的右上角转置好的小分块复制到 B[j][i] 分块中的左下角
 */
void trans_8_8_bottom_left(int i, int j, int M, int N, int A[N][M], int B[M][N]) {
    int k;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    // 此时只要 i != j ，那么 A[i][j] 分块和 B[j][i] 分块使用的缓存不会冲突
    for (k = 0; k < 4; k++) {
        // i != j: 第一次产生 4 个未命中，后续三次在缓存中无未命中
        // i == j: 第一次产生 4 个未命中，后续三次由于冲突各产生 1 次未命中
        v0 = A[i + 4][j + k];
        v1 = A[i + 5][j + k];
        v2 = A[i + 6][j + k];
        v3 = A[i + 7][j + k];
        // i != j: 由于前面已经用过 B[j][i] 分块的右上角，所以此时在缓存中，无未命中
        // i == j: 由于冲突，每次循环会发生 1 个未命中
        v4 = B[j + k][i + 4];
        v5 = B[j + k][i + 5];
        v6 = B[j + k][i + 6];
        v7 = B[j + k][i + 7];
        B[j + k][i + 4] = v0;
        B[j + k][i + 5] = v1;
        B[j + k][i + 6] = v2;
        B[j + k][i + 7] = v3;
        // 这里 B[j][i] 分块的左下角第一次访问，每次循环只发生 1 个未命中
        B[j + 4 + k][i] = v4;
        B[j + 4 + k][i + 1] = v5;
        B[j + 4 + k][i + 2] = v6;
        B[j + 4 + k][i + 3] = v7;
    }
    // 综上：这个函数每调用一次，
    //      i != j: 产生 8 个未命中
    //      i == j: 产生 15 个未命中
}

/*
 * 将 A[i][j] 的 8 * 8 分块的右下角拷贝至 B[j][i] 的 8 * 8 分块的右下角
 */
void copy_8_8_bottom_right(int i, int j, int M, int N, int A[M][N], int B[M][N]) {
    int k;
    int v0, v1, v2, v3;
    // 此时只要 i != j ，那么 A[i][j] 分块和 B[j][i] 分块使用的缓存不会冲突
    for(k = 4; k < 8; k++) {
        // i != j: 前面已经访问过了，所以在缓存中，无未命中
        // i == j: 由于冲突，前三行已在缓存中，第四行的数据没在缓存中，所以总共只有 1 次未命中
        v0 = A[i + k][j + 4];
        v1 = A[i + k][j + 5];
        v2 = A[i + k][j + 6];
        v3 = A[i + k][j + 7];
        // i != j: 前面已经访问过了，所以在缓存中，无未命中
        // i == j: 由于冲突，每次循环 B 有 1 次未命中
        B[j + k][i + 4] = v0;
        B[j + k][i + 5] = v1;
        B[j + k][i + 6] = v2;
        B[j + k][i + 7] = v3;
    }
    // 综上：这个函数每调用一次，
    //      i != j: 无未命中
    //      i == j: 产生 5 个未命中
}

void trans_64_64(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;
    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            // 先将 A[i][j] 的 8 * 8 分块中的前四行复制到 B[j][i] 分块中的前四行
            copy_8_8_pre_4(i, j, M, N, A, B);
            // 此时 B[j][i] 中的 4 * 8 分块全部在缓存中，在 B 中实现 4 * 4 小分块原地转置
            trans_with_4_4(j, i, M, N, B);
            trans_with_4_4(j, i + 4, M, N, B);

            // 再将 A[i][j] 的 8 * 8 分块中的左下角转置到 B[j][i] 分块中的右上角
            // 【注意】这一步既对 A[i][j] 左下角分块内进行了转置，也把 A[i][j] 左下角分块放到 B[j][i] 右上角
            // 同时将刚刚 B[j][i] 分块中的右上角转置好的小分块复制到 B[j][i] 分块中的左下角
            trans_8_8_bottom_left(i, j, M, N, A, B);

            // 最后将 A[i][j] 的 8 * 8 分块的右下角拷贝至 B[j][i] 的 8 * 8 分块的右下角
            copy_8_8_bottom_right(i, j, M, N, A, B);
            // 此时 B[j][i] 的 8 * 8 分块右下角全部在缓存中，实现 4 * 4 小分块原地转置
            trans_with_4_4(j + 4, i + 4, M, N, B);
        }
    }
    // 综上：
    //  i != j: 总共 56 个分块，每个分块未命中次数为 16 ，A 和 B 各 8 次必须的未命中
    //  i == j: 总共 8 个分块，每个分块未命中次数为 28
    //
    //  总未命中次数 = 56 * 16 + 8 * 28 = 1120
}



/*
 * 专门处理 61 * 67 矩阵的转置
 */
char trans_61_67_desc[] = "Transpose 61 x 67";
void trans_61_67(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, k, l;
    for (i = 0; i < N; i += 17) {
        for (j = 0; j < M; j += 17) {
            for (k = i; k < i + 17 && k < N; k++) {
                for (l = j; l < j + 17 && l < M; l++) {
                    B[l][k] = A[k][l];
                }
            }
        }
    }
}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        trans_32_32(M, N, A, B);
        return;
    }
    if (M == 64 && N == 64) {
        trans_64_64(M, N, A, B);
        return;
    }
    if (M == 61 && N == 67) {
        trans_61_67(M, N, A, B);
        return;
    }

    // 处理其他情况的转置
    trans(M, N, A, B);
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

