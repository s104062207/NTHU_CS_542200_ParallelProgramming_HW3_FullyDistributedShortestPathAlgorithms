#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <fstream> 
int Dist[2000000000];
int rank, size;
int n ;
int edge;
FILE *fp;
int task;
int idx;
int nextidx;

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);	
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	fp = fopen(argv[1], "rb"); 
    fread(&n   , sizeof(int), 1, fp); 
	fread(&edge, sizeof(int), 1, fp); 
	
	int group[n];
	FILE *fpPart = fopen(argv[3], "r"); 
	if (fpPart) 
	{
		for (int j = 0; j < n; ++j) {
			fscanf(fpPart , "%d" , &group[j]); 
			//printf("%d\n",group[j]);
		}
	}
	
	
	task = n/size;
	if(task>size&&n%size!=0)
		task++;
    
	idx = rank*task ;
    if(rank == size-1)
		nextidx =  n;
	else
		nextidx = task+idx;
		

	#pragma omp parallel for collapse(2)
	for (int i = idx; i < nextidx; ++i) {
		for (int j = 0; j < n; ++j) {
			Dist[i*n+j] = 100000;
		}
	}
	#pragma omp parallel for
	for (int i = idx; i < nextidx; ++i)
		Dist[i*n+i] = 0;
	
	int a,b,v;
	//#pragma omp parallel for 
	for (int i = 0; i < edge; ++i) {
		fread(&a, sizeof(int), 1, fp); 
		fread(&b, sizeof(int), 1, fp);
		fread(&v, sizeof(int), 1, fp);
		if(a>=idx&&a<nextidx)
			Dist[a*n+b] = v;
		//printf("%d %d %d\n",a,b,v);
	}
	
	if(size>1)
	{
		for (int t = 0; t < size-1; t++) {		
			for (int k = task*t; k < task*(t+1); k++) {
				//MPI_Barrier(MPI_COMM_WORLD);
				if(group[k]==0||!fpPart)
					MPI_Bcast (Dist+k*n, n, MPI_INT, t, MPI_COMM_WORLD);	
				#pragma omp parallel for collapse(2)
				for (int i = idx; i < nextidx; i++) {
					for (int j = 0; j < n; j++) {
						if (Dist[i*n+j] > Dist[i*n+k] + Dist[k*n+j]) {
							Dist[i*n+j] = Dist[i*n+k] + Dist[k*n+j];
						}
					}
				}
			}
		}
		for (int k = task*(size-1); k < n; k++) {
			//MPI_Barrier(MPI_COMM_WORLD);
			MPI_Bcast (Dist+k*n, n, MPI_INT, size-1, MPI_COMM_WORLD);
			#pragma omp parallel for collapse(2)
			for (int i = idx; i < nextidx; i++) {
				for (int j = 0; j < n; j++) {
					if (Dist[i*n+j] > Dist[i*n+k] + Dist[k*n+j]) {
						Dist[i*n+j] = Dist[i*n+k] + Dist[k*n+j];
					}
				}
			}
			
		}
		if(rank == 0)
		{
			FILE *fp = fopen(argv[2], "ab+");
			//fseek(fp, idx*n*sizeof(int), SEEK_SET);
			fwrite(Dist+idx*n, sizeof(int), (nextidx-idx)*n, fp);
			fclose(fp);
		}
		for (int t = 1; t < size; t++)
		{
			MPI_Barrier(MPI_COMM_WORLD);
			if(rank == t)
			{
				FILE *fp = fopen(argv[2], "ab+");
				//fseek(fp, idx*n*sizeof(int), SEEK_SET);
				fwrite(Dist+idx*n, sizeof(int), (nextidx-idx)*n, fp);
				fclose(fp);
			}
		}
	}else
	{
		for (int k = 0; k < n; k++) {
			#pragma omp parallel for collapse(2)
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					if (Dist[i*n+j] > Dist[i*n+k] + Dist[k*n+j]) {
						Dist[i*n+j] = Dist[i*n+k] + Dist[k*n+j];
					}
				}
			}
		}
		FILE *fp = fopen(argv[2], "ab+");
		fwrite(Dist, sizeof(int), n*n, fp);
		fclose(fp);
	}
	MPI_Finalize();
	return 0;
}