#include <mpi.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "pascalops.h"

#define MAX_ITER 1000

void initialize(double **A, int local_n, int n) {
  for (int i = 0; i <= local_n + 1; ++i) {
    for (int j = 0; j < n + 2; ++j) {
      A[i][j] = (i == 0 || j == 0) ? 1.0 : 0.0;
    }
  }
}

void exchange_halos(double **A, int local_n, int n, int rank, int size) {
  int up = (rank == 0) ? MPI_PROC_NULL : rank - 1;
  int down = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

  // Envia a primeira linha útil (1) para cima e recebe a linha fantasma inferior (local_n + 1)
  MPI_Sendrecv(A[1], n + 2, MPI_DOUBLE, up, 0,
               A[local_n + 1], n + 2, MPI_DOUBLE, down, 0,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  // Envia a última linha útil (local_n) para baixo e recebe a linha fantasma superior (0)
  MPI_Sendrecv(A[local_n], n + 2, MPI_DOUBLE, down, 1,
               A[0], n + 2, MPI_DOUBLE, up, 1,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

double matrix_calculation(double **A, int local_n, int start_row, int n, int rank, int size) {
  double tmp;
  double local_diff = 0.0;
  double global_diff = 0.0;
  int i, j;

  pascal_start(1);

  // --- REGIÃO 2: Troca de bordas e cálculo paralelo nos índices ímpares ---
  pascal_start(2);
  exchange_halos(A, local_n, n, rank, size);

  for (i = 1; i <= local_n; ++i) {
    int global_i = start_row + i - 1;
    for (j = 1; j <= n; ++j) {
      if ((global_i + j) % 2 == 1) {
        tmp = A[i][j];
        A[i][j] = 0.2 * (A[i][j] + A[i][j - 1] + A[i - 1][j] +
                         A[i][j + 1] + A[i + 1][j]);
        local_diff += fabs(A[i][j] - tmp);
      }
    }
  }
  pascal_stop(2);

  // --- REGIÃO 3: Cálculo nos índices pares (Gargalo no rank 0) ---
  pascal_start(3);
  int row_size = n + 2;
  std::vector<int> recvcounts(size), displs(size);
  int offset = row_size;

  for (int r = 0; r < size; ++r) {
    int r_rows = (n / size) + (r < (n % size) ? 1 : 0);
    recvcounts[r] = r_rows * row_size;
    displs[r] = offset;
    offset += recvcounts[r];
  }

  std::vector<double> full_A;
  if (rank == 0) {
    full_A.resize((n + 2) * row_size, 0.0);
  }

  MPI_Gatherv(A[1], local_n * row_size, MPI_DOUBLE,
              full_A.data(), recvcounts.data(), displs.data(), MPI_DOUBLE,
              0, MPI_COMM_WORLD);

  if (rank == 0) {
    for (i = 1; i <= n; ++i) {
      for (j = 1; j <= n; ++j) {
        if ((i + j) % 2 == 0) {
          int idx = i * row_size + j;
          tmp = full_A[idx];
          full_A[idx] = 0.2 * (full_A[idx] +
                               full_A[idx - 1] +
                               full_A[(i - 1) * row_size + j] +
                               full_A[idx + 1] +
                               full_A[(i + 1) * row_size + j]);
          local_diff += fabs(full_A[idx] - tmp);
        }
      }
    }
  }

  MPI_Scatterv(full_A.data(), recvcounts.data(), displs.data(), MPI_DOUBLE,
               A[1], local_n * row_size, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

  MPI_Allreduce(&local_diff, &global_diff, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  pascal_stop(3);

  pascal_stop(1);

  return global_diff;
}

void solve_parallel(double **A, int local_n, int start_row, int n, int rank, int size) {
  int iters;
  double diff = 0;
  for (iters = 1; iters < MAX_ITER; ++iters) {
    diff = matrix_calculation(A, local_n, start_row, n, rank, size);
  }
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int n = std::stoi(argv[1]);
  int interior_n = n - 1;

  // Divisão de linhas entre processos MPI
  int base_rows = interior_n / size;
  int rem = interior_n % size;
  int local_n = base_rows + (rank < rem ? 1 : 0);
  int start_row = rank * base_rows + (rank < rem ? rank : rem) + 1;

  // Alocação contígua em memória das (local_n + 2) linhas locais
  double *data = new double[(local_n + 2) * (n + 2)];
  double **A = new double *[local_n + 2];
  for (int i = 0; i < local_n + 2; ++i) {
    A[i] = &data[i * (n + 2)];
  }

  initialize(A, local_n, n);

  solve_parallel(A, local_n, start_row, interior_n, rank, size);

  delete[] A;
  delete[] data;

  MPI_Finalize();
  return 0;
}