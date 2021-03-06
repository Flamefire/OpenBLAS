/***************************************************************************
Copyright (c) 2014, The OpenBLAS Project
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the OpenBLAS project nor the names of
its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifdef __CYGWIN32__
#include <sys/time.h>
#endif
#include "common.h"

#undef TPSV

#ifndef COMPLEX

#ifdef DOUBLE
#define TPSV   BLASFUNC(dtpsv)
#else
#define TPSV   BLASFUNC(stpsv)
#endif

#else

#ifdef DOUBLE
#define TPSV   BLASFUNC(ztpsv)
#else
#define TPSV   BLASFUNC(ctpsv)
#endif

#endif

#if !defined(__WIN32__) && !defined(__WIN64__) && !defined(__CYGWIN32__) && 0

static void *huge_malloc(BLASLONG size)
{
    int shmid;
    void *address;

#ifndef SHM_HUGETLB
#define SHM_HUGETLB 04000
#endif

    if ((shmid =shmget(IPC_PRIVATE,
             (size + HUGE_PAGESIZE) & ~(HUGE_PAGESIZE - 1),
             SHM_HUGETLB | IPC_CREAT |0600)) < 0) {
        printf( "Memory allocation failed(shmget).\n");
        exit(1);
    }

    address = shmat(shmid, NULL, SHM_RND);

    if ((BLASLONG)address == -1) {
        printf( "Memory allocation failed(shmat).\n");
        exit(1);
    }

    shmctl(shmid, IPC_RMID, 0);

    return address;
}

#define malloc huge_malloc

#endif

int main(int argc, char *argv[])
{

    FLOAT *a, *x;
    char *p;

    char uplo ='U';
    char trans='N';
    char diag ='U';

    int loops = 1;
    int l;
    blasint inc_x=1;

    if ((p = getenv("OPENBLAS_UPLO"))) uplo=*p;
    if ((p = getenv("OPENBLAS_TRANS"))) trans=*p;
    if ((p = getenv("OPENBLAS_DIAG"))) diag=*p;
    if ((p = getenv("OPENBLAS_LOOPS")))  loops = atoi(p);
    if ((p = getenv("OPENBLAS_INCX")))   inc_x = atoi(p);

    blasint n, i, j;

    int from =   1;
    int to   = 200;
    int step =   1;

    struct timespec start = { 0, 0 }, stop = { 0, 0 };
    double time1, timeg;

    argc--;argv++;

    if (argc > 0) { from     = atol(*argv);        argc--; argv++;}
    if (argc > 0) { to       = MAX(atol(*argv), from);    argc--; argv++;}
    if (argc > 0) { step     = atol(*argv);        argc--; argv++;}

    fprintf(stderr, "From : %3d  To : %3d Step = %3d Uplo = %c Trans = %c  Diag = %c Loops=%d Inc_x=%d\n", from,
            to, step, uplo, trans, diag, loops, inc_x);

    if (( a = (FLOAT *)malloc(sizeof(FLOAT) * to * to * COMPSIZE)) == NULL) {
        fprintf(stderr,"Out of Memory!!\n");exit(1);
    }

    if (( x = (FLOAT *)malloc(sizeof(FLOAT) * to * abs(inc_x) * COMPSIZE)) == NULL){
        fprintf(stderr,"Out of Memory!!\n");exit(1);
    }

#ifdef __linux
    srandom(getpid());
#endif

    fprintf(stderr, "   SIZE       Flops\n");

    for(n = from; n <= to; n += step) {
        timeg=0;

        fprintf(stderr, " %6d : ", (int)n);
        for(j = 0; j < n; j++) {
            for(i = 0; i < n * COMPSIZE; i++) {
                a[(long)i + (long)j * (long)n * COMPSIZE] = ((FLOAT) rand() / (FLOAT) RAND_MAX) - 0.5;
            }
        }

        for (i = 0; i < n * COMPSIZE * abs(inc_x); i++) {
            x[i] = ((FLOAT) rand() / (FLOAT) RAND_MAX) - 0.5;
        }

        for (l = 0; l < loops; l++) {
            clock_gettime(CLOCK_REALTIME, &start);
            TPSV (&uplo, &trans, &diag, &n, a, x, &inc_x);
            clock_gettime(CLOCK_REALTIME, &stop);

            time1 = (double)(stop.tv_sec - start.tv_sec) + (double)((stop.tv_nsec - start.tv_nsec)) / 1.e9;
            timeg += time1;
        }

        timeg /= loops;
        fprintf(stderr, " %10.2f MFlops %12.9f sec\n",
                COMPSIZE * COMPSIZE * 1. * (double)n * (double)n / timeg / 1.e6, timeg);
    }

    return 0;
}

// void main(int argc, char *argv[]) __attribute__((weak, alias("MAIN__")));
