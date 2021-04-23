//This program was created by Reid Edward Erb, UCID 10089638
//for the course CPSC 359 at the University of Calgary in the 2021 winter semester
//CPSC 359 is instructed by Dr. Jalal Kawash. The lead TA for this assignment is Desmond Larsen-Rosner

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_HORSE 100
#define INT_SIZE 4
#define TIME_SIZE 8

struct shared
{
	int *progress;
	int *rank;
	int lastranked;
	int *ranktrue;
	int raceFinished;
	time_t startTime;
	time_t *finishTime;
	int horse_n;
	int finishline;
	int starter;
};

struct shared racestruct;

void racedisplay(struct shared * racep)
{
	system("clear");
	int i, j;
	for (int i = 0; i < racep->horse_n; i++)
	{
		printf("HORSE%d:\t", i);
		for (j = 0; j < (racep->progress[i] - 1); j++)
		{
			printf("-");
		}
		if (racep->progress[i] == racep->finishline)
		{printf("*\n");}
		else {printf(">\n");}
	}
}

void * horserun(void * horseid)
{
	int horse = (int) horseid;
	struct shared * newp;
	newp = &racestruct;

	while (newp->starter = 0); //Wait until the race starts.

	int i, sec;
	for (i = 0; i < newp->finishline; i++)
	{
		sec = rand() & 7; //sec will have some value in [0, 7]
		sleep(sec);
		newp->progress[horse]++;
	}
	time_t fintime = time(0);
	newp->finishTime[horse] = fintime - newp->startTime;

	while (newp->raceFinished == 0);
	pthread_exit(NULL);
}

void * masterfunc(void * racep)
{
	struct shared * newp;
	newp = (struct shared *) racep;

	newp->starter = 0;
	
	int i;
	int dist = 0;
	char strin[4];

	while (dist < 1)
	{
		printf("How far do you want the horses to run? (Enter an integer greater than zero)\n");
		scanf("%s", strin);
		dist = atoi(strin);
		if (dist < 1) printf("ERROR: Invalid input detected\n");
	}
	newp->finishline = dist;

	int temp = newp->horse_n;
	int *progarr;
	progarr = (int *)malloc(temp * INT_SIZE);
	newp->progress = progarr; //Now there is space allocated for the progress array and the structure points to it.

	for (i = 0; i < newp->horse_n; i++)
	{newp->progress[i] = 0;}

	int *rankarr;
	rankarr = (int *)malloc(temp * INT_SIZE);
	newp->rank = rankarr; //Space is allocated for the rank array and the structure points to it.

	int *rankbool;
	rankbool = (int *)malloc(temp * INT_SIZE);
	newp->ranktrue = rankbool; //Space is allocated for an array of integers representing booleans
	//If a horse hasn't been added to the rank list yet, its corresponding int in the array will equal zero.

	for (i = 0; i < newp->horse_n; i++) newp->ranktrue[i] = 0; //This will initialize the array so all elements equal zero.

	time_t *timearr;
	timearr = (time_t *)malloc(temp * TIME_SIZE);
	newp->finishTime = timearr; //Space is allocated for the finishTime array
	
	for (i = 0; i < newp->horse_n; i++) newp->finishTime[i] = -1;

	pthread_t *threadarr;
	threadarr = (pthread_t *)malloc(temp * sizeof(pthread_t)); //Space allocated for an array of threads.

	newp->raceFinished = 0;
	newp->lastranked = 0;

	for (i = 0; i < temp; i++)
	{pthread_create(&threadarr[i], NULL, horserun, (void *) i);}

	newp->startTime = time(0);
	newp->starter = 1; 
	//We use the starter flag because don't want the horses to start running until all the threads have been created.

	while (newp->raceFinished == 0)
	{
		racedisplay(newp);
		for (i = 0; i < newp->horse_n; i++)
		{
			if (newp->finishTime[i] != -1 && newp->ranktrue[i] == 0)
			{
				newp->rank[i] = newp->lastranked;
				newp->lastranked++;
				newp->ranktrue[i] = 1;
			}
		}
		if (newp->lastranked >= newp->horse_n) newp->raceFinished = 1;
	}

	for (i = 0; i < newp->horse_n; i++)
	{pthread_join(threadarr[i], NULL);} //Joins the horse threads to the master

	i = 0;
	int j;
	while (i < newp->horse_n)
	{
		for (j = 0; j < newp->horse_n; j++)
		{
			if (newp->rank[j] == i)
			{
				printf("%d: HORSE%d\n", (i + 1), j);
				i++;
			}
		}
	}

	free(progarr);
	free(rankarr);
	free(rankbool);
	free(timearr);
	free(threadarr);

	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	srand(time(0));
	int n;
	char nin[4];
	if (argc > 1)
	{
		n = atoi(argv[1]); 
	}
	else
	{
		printf("How many horses do you want in the race (Enter an integer in the range [1, %d])\n", MAX_HORSE);
		scanf("%s", nin);
		n = atoi(nin);
	}
	
	while (n < 1 || n > MAX_HORSE)
	{
		printf("ERROR: Number of horses outside acceptable range.\n");
		printf("Please enter an integer in the range [1, %d]\n", MAX_HORSE);
		scanf("%s", nin);
		n = atoi(nin);
	}
	
	struct shared * racep;
	racep = &racestruct;
	racep->horse_n = n;
	printf("The number of horses in the race will be %d\n", racep->horse_n);

	pthread_t master = 777;
	pthread_create(&master, NULL, masterfunc, racep);
	
	pthread_join(master, NULL);

	return 0;
}
