#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
/// Naive check symmetry
int checkSym(float *M, int mat_size) {
    int is_sym = 1; // assumed symmetric
#pragma omp parallel num_threads(1)
    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            if (M[i * mat_size + j] != M[j * mat_size + i]) {
                is_sym = 0; // non-symmetric
            }
        }
    }
    return is_sym;
}

/// Parallel OMP check symmetry
int checkSymOMP(float *M, int mat_size) {
    int is_sym = 1; // assumed symmetric

    /*
     * If you want to use a different number of threads
     * using export OMP_NUM_THREADS
     * you must comment the next two lines of code,
     * otherwise 'export' it is overwritten
    */
    int num_thr = 4; //choose_num_threads(size);
    //    omp_set_num_threads(num_thr);

#pragma omp parallel for collapse(2) reduction(&&:is_sym)
    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            if (M[i * mat_size + j] != M[j * mat_size + i]) {
                is_sym = 0; // non-symmetric
            }
        }
    }

    return is_sym;
}

/// Parallel MPI check symmetry
int checkSymMPI(float *M, int mat_size, int rank, int num_procs) {
    int is_sym = 1, local_sym = 1; // assumed symmetric
    int rows_per_proc = mat_size / num_procs;
    int start = rows_per_proc * rank;
    int end = rows_per_proc * (rank + 1);

    // --- Broadcast matrix to all processes ---
    if (rank != 0) {
        // Every process must allocate space for matrix M
        M = allocate_sqr_matrix(mat_size);
    }
    MPI_Bcast(M,  mat_size * mat_size, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // ------ Compute local symmetry check and reduce ------
    local_sym = check_sym_local(M, mat_size, start, end);
    MPI_Allreduce(&local_sym, &is_sym, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

    //printf("process %d, local_sym: %d, is_sym = %d\n", rank, local_sym, is_sym);

    // --- Clean up ---
    if (rank != 0) {
        free(M);
    }
    return is_sym;
}
/// Check the symmetry of the square block on the diagonal
int check_sym_local(float *matrix, int mat_size, int start_row, int end_row) {
    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < mat_size; j++) {
            if (matrix[i * mat_size + j] != matrix[j * mat_size + i]) {
                return 0;
            }
        }
    }
    return 1;
}
