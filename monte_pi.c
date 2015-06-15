/*A program that approximates the value of pi using the Monte-Carlo simulation
 * Kevin Ou
 */
#include <pthread.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h> 
#include <math.h>

#define MAX_THREADS 1000
#define CENTER .50
#define MILLION 1000000

//Declare global counters
bool done = false;
int totalSimulationCount = 0;
int insideCircleCount = 0;

//Mutex declaration
pthread_mutex_t simulationCountLock;
pthread_mutex_t insideCircleCountLock;
pthread_mutex_t randLock;

//Mutex Conditional
pthread_cond_t isMillion;

/* Generate random points on the unit square and keeps track if its in the circle.
 * If the point is in the circle, it updates a global counter.
 * Signals the printing thread to print the current value.
 */
void *simulation (void *arg)
{
  int simulationLimit = *(int *) arg; //the amount of simulations to be performed
  double firstNumber; //first random number
  double secondNumber; //second random number
  double xDistance; 
  double yDistance;
  double distance; //distance from the center (.5,.5)
  
  while(!done)
  {
    //Calculates two random doubles from 0 to 1.0
    pthread_mutex_lock(&randLock);
    firstNumber = (rand() % 1000)/1000.00; 
    secondNumber = (rand() % 1000)/1000.00; 
    pthread_mutex_unlock(&randLock);
    
    //Distance calculation
    xDistance = (firstNumber-CENTER) * (firstNumber-CENTER); //(X2-X1)^2
    yDistance = (secondNumber-CENTER) * (secondNumber-CENTER); //(Y2-Y1)^2
    distance =  sqrt(xDistance+yDistance);
    
    //If distance is less than the radius, it's inside the circle
    if(distance <= CENTER)
    {
      pthread_mutex_lock(&insideCircleCountLock);
      insideCircleCount++;
      pthread_mutex_unlock(&insideCircleCountLock);
    }
    
    //if the total simulation count is equal to the limit, done is set to true
    pthread_mutex_lock(&simulationCountLock);
    totalSimulationCount++;
    if(totalSimulationCount == simulationLimit)
    {
      pthread_cond_broadcast(&isMillion);
      done = true;
    }
    else if(totalSimulationCount % MILLION == 0)
    {
      pthread_cond_broadcast(&isMillion);
    }
    pthread_mutex_unlock(&simulationCountLock);
  }
  return NULL;
}

/* The printing thread is responsible for calculating
 * the current approximation of pi when signalled by the
 * simulation thread
 */
void *printing (void *arg)
{
  double approximation;
  pthread_mutex_lock(&simulationCountLock);
  while(!done)
  {
    pthread_cond_wait(&isMillion,&simulationCountLock);
    pthread_mutex_lock(&insideCircleCountLock);
    approximation = (double) insideCircleCount/totalSimulationCount * 4.0;
    printf("The current approximation of pi is %f.\n",approximation);
    pthread_mutex_unlock(&insideCircleCountLock);
  }
  pthread_mutex_unlock(&simulationCountLock);
 
  return NULL;
}

/* The main thread is responsible for initializing the shared
 * variables and the random number generator. It creates the simulation
 * and printing threads and waits for them to terminate
 */
int main (int argc, char *argv[])
{
  //gets command line values
  int n_threads; 
  int n_simulations;
  sscanf( argv[1], "%d", &n_threads);
  sscanf( argv[2], "%d", &n_simulations);
  
  //pthread initialization
  pthread_t *tid[MAX_THREADS];
  void* thread_result;
  
  //initializes the mutex
  pthread_mutex_init(&simulationCountLock, NULL);
  pthread_mutex_init(&insideCircleCountLock, NULL);
  pthread_mutex_init(&randLock, NULL);
  pthread_cond_init(&isMillion, NULL);
  
  //initializes random number generator
  srand (time(NULL)); 
  
  //Allocate memory for the pthreads
  int i;
  for (i = 0; i < MAX_THREADS; i++) 
  {
    tid[i] = (pthread_t *) malloc(sizeof(pthread_t));
  }
  
  //printing thread
  if (pthread_create (tid[n_threads], NULL,
			 printing,
			 NULL )) 
      {
        fprintf (stderr, "Error creating thread %d.\n", i);
        exit (-1);
      }
      
  //creates the simulation threads;
  for(i = 0; i < n_threads; i++)
  {
     if (pthread_create (tid[i], NULL,
			 simulation,
			 &n_simulations )) 
      {
        fprintf (stderr, "Error creating thread %d.\n", i);
        exit (-1);
      }
  }
      
  //joins the threads
  for(i = 0; i < n_threads+1; i++)
  {
    pthread_join (*tid[i], &thread_result);
  }
  
  //clean up mutex
  pthread_mutex_destroy(&insideCircleCountLock);
  pthread_mutex_destroy(&simulationCountLock);
  pthread_mutex_destroy(&randLock);
  pthread_cond_destroy(&isMillion);
}
