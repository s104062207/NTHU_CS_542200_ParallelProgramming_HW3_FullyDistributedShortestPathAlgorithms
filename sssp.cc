#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <pthread.h>
#include <fstream> 
#include <map>

using namespace std;

int myrank, size;
int n ;
int edge;
FILE *fp;
int task;
int r;
int idx;
int nextidx;

int Dist[20000];
map<int, int> mapDist[20000];
map<int, int> mapDistTo[20000];
map<int, int> mapDistFrom[20000];

pthread_mutex_t mutex;
void *ssspSend(void *threadId) {
	int i =  *(int*) threadId;
	
	
	printf("pthread %d\n",i);
	map<int, int>::iterator iter;
	
	MPI_Request isReq[n];
	int j=0;
	int temp[n];
	for(iter = mapDistTo[i].begin(); iter != mapDistTo[i].end(); iter++)
	{
		pthread_mutex_lock(&mutex);
		Dist[iter->first] = (Dist[i] + iter->second<100000)?Dist[i] + iter->second:100000;
		//printf("%d=%d + %d\n",Dist[iter->first],Dist[i],iter->second);
		//printf("MPI_Isend() %d send to %d tag %d\n",Dist[iter->first],iter->first/task,iter->first);
		MPI_Isend(Dist+iter->first,1,MPI_INT,iter->first/task,iter->first,MPI_COMM_WORLD,&isReq[j++]);
		pthread_mutex_unlock(&mutex);
	}
	
	for(iter = mapDist[i].begin(); iter != mapDist[i].end(); iter++)
	{
		pthread_mutex_lock(&mutex);
		if (Dist[i] + iter->second < Dist[iter->first])
		{
			//printf("%d = %d + %d\n",Dist[iter->first],Dist[i],iter->second);
			Dist[iter->first] = Dist[i] + iter->second;
		}
		pthread_mutex_unlock(&mutex);
	}
	printf("pthread %d\n",i);
	//map<int, int>::iterator iter;
	int cnt=0;
	MPI_Request irReq[n];
	MPI_Status recvStats[n];
	
	
	for(iter = mapDistFrom[i].begin(); iter != mapDistFrom[i].end(); iter++)
	{
		pthread_mutex_lock(&mutex);
		int value;
		printf("%d   %d\n",iter->first,iter->second);
		MPI_Irecv(&value,1,MPI_INT,iter->first/task, MPI_ANY_TAG, MPI_COMM_WORLD, &irReq[cnt++]);		
		pthread_mutex_unlock(&mutex);
		MPI_Wait(&irReq[cnt-1],  &recvStats[cnt-1]);
		//printf("MPI_Irecv() receive %d from %d tag %d\n",value,iter->first/task,recvStats[cnt-1].MPI_TAG);
		//printf("Dist[i] = %d,value = %d\n",Dist[recvStats[cnt-1].MPI_TAG] ,value);
		if(value<Dist[recvStats[cnt-1].MPI_TAG])
			Dist[recvStats[cnt-1].MPI_TAG]=value;
		
	}
	
	
	
	
	
	
	MPI_Status sendStats[j];
	MPI_Waitall(j,isReq,sendStats);
	
	pthread_exit(NULL);
}
/*void *ssspRecv(void *threadId) {
	int i =  *(int*) threadId;
	
	
	printf("pthread %d\n",i);
	map<int, int>::iterator iter;
	int cnt=0;
	MPI_Request irReq[n];
	MPI_Status recvStats[n];
	
	
	for(iter = mapDistFrom[i].begin(); iter != mapDistFrom[i].end(); iter++)
	{
		pthread_mutex_lock(&mutex);
		int value;
		printf("%d   %d\n",iter->first,iter->second);
		MPI_Irecv(&value,1,MPI_INT,iter->first/task, MPI_ANY_TAG, MPI_COMM_WORLD, &irReq[cnt++]);		
		pthread_mutex_unlock(&mutex);
		MPI_Wait(&irReq[cnt-1],  &recvStats[cnt-1]);
		printf("MPI_Irecv() receive %d from %d tag %d\n",value,iter->first/task,recvStats[cnt-1].MPI_TAG);
		printf("Dist[i] = %d,value = %d\n",Dist[recvStats[cnt-1].MPI_TAG] ,value);
		if(value<Dist[recvStats[cnt-1].MPI_TAG])
			Dist[recvStats[cnt-1].MPI_TAG]=value;
		
	}
	pthread_exit(NULL);
}*/
int main(int argc, char* argv[])
{
	//MPI_Init(&argc, &argv);	
	int provided;
	MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE, &provided);
    
	MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	pthread_mutex_init(&mutex, NULL); 

	fp = fopen(argv[1], "rb"); 
    fread(&n   , sizeof(int), 1, fp); 
	fread(&edge, sizeof(int), 1, fp); 
	
	task = n/size;
	r = n%size;
	if(n%size!=0)
		task++;
    
	idx = myrank*task ;
	nextidx = task+idx;
    if(idx >n)
		idx =  n;
	if(nextidx >n)
		nextidx =  n;
		
	
	for (int i = 1; i < n; ++i)
		Dist[i] = 100000;
	
	
	int a,b,v;
	for (int i = 0; i < edge; ++i) {
		fread(&a, sizeof(int), 1, fp); 
		fread(&b, sizeof(int), 1, fp);
		fread(&v, sizeof(int), 1, fp);
		
		if(a==0)
			Dist[b]=v;
		if(a>=idx&&a<nextidx)
		{
			if(b>=idx&&b<nextidx)
			{
				mapDist[a][b]=v;
				printf("myrank=%d, %d to %d = %d\n",myrank,a,b,mapDist[a][b]);
			}else
				mapDistTo[a][b]=v;//a send to b
		}
		if(b>=idx&&b<nextidx)
		{
			if(a<idx||a>=nextidx)
			{
				mapDistFrom[b][a]=v;//b receive from a
				printf("myrank=%d, %d to %d = %d\n",myrank,a,b,mapDistFrom[b][a]);
			}
		}
		
	}
	
	
	int ID[20000];
	if(size>1)
	{
		
		for (int k = 0; k < n-1; k++)
		{
			pthread_t threads[nextidx-idx];
			for (int i = idx; i < nextidx; i++)
			{
				ID[i]=i;
				//printf("pthread_create() idx = %d\n", i);
				int rc = pthread_create(&threads[(i-idx)], NULL, ssspSend, (void*)&ID[i]);
				//rc = pthread_create(&threads[(i-idx)*2+1], NULL, ssspRecv, (void*)&ID[i]);
			}
			for (int i=0; i<nextidx-idx; i++) {
				pthread_join(threads[i], NULL);
				//pthread_join(threads[i*2+1], NULL);
				//printf("pthread_join()i = %d\n", i);
			}
			//printf("pthread_exit()k = %d\n", k);
		}		
	}else
	{	
		for (int k = 0; k < n-1; k++) 
			for (int i = 1; i < n; i++)
				for(map<int, int>::iterator iter = mapDist[i].begin(); iter != mapDist[i].end(); iter++)
				{
					printf("%d   %d\n",iter->first,iter->second);
					if (Dist[i] + iter->second < Dist[iter->first])
                    {
                        printf("%d = %d + %d\n",Dist[iter->first],Dist[i],iter->second);
						Dist[iter->first] = Dist[i] + iter->second;
                    }
				}
					
		
	}
	printf("start to output\n");
	Dist[0]=0;
	for (int t = 0; t < size; t++)
	{
		if(myrank == t)
		{
			FILE *fp = fopen(argv[2], "ab+");
			printf("print %d to %d\n",idx,nextidx);
			fwrite(Dist+idx, sizeof(int), (nextidx-idx), fp);
			fclose(fp);
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
	MPI_Finalize();
	return 0;
}