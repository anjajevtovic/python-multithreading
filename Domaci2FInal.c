#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define CARS_A 22
#define CARS_B 8
#define CARS_C 14
#define CARS_D 12
#define TOTAL_CARS 56


const int dir_a = 0, dir_b = 1, dir_c = 2, dir_d = 3;
const int total_cars_to_pass = TOTAL_CARS
const float adjustment_inc = 4.0f / 10.0f;


sem_t a_sem;
sem_t b_sem;
sem_t c_sem;
sem_t d_sem;

int   cars_passed      = 0;
float sleep_ms_ac      = 30.0f / 10.0f;
float sleep_ms_bd      = 30.0f / 10.0f;

float total_dir_wait_time_ms[4] = {0 ,0 ,0 ,0};
float prev_total_wait_time = 0;
float total_wait_time = 0;

float adjust_multiplier_ac = -1;
float adjust_multiplier_bd = -1;

void create_semaphores();
void destroy_semaphores();
void thread_simulation();
char is_total_wait_time_growing();
float calculate_total_wait_time();

int main(int argc, char *argv[])
{	
	create_semaphores();

	printf("Running first simulation...\n");
	thread_simulation();

	float prev_sleep_ac = 0;
	float prev_sleep_bd = 0;

	while(1)// main loop
	{
		printf("------------------Running new iteration---------------------\n");

		//sacuvaj prethodna trajanja zelenog
		prev_sleep_ac = sleep_ms_ac;
		prev_sleep_bd = sleep_ms_bd;

		float delta = adjust_multiplier_ac * adjustment_inc;
		printf("Changing green light for AC by %f\n", delta);
		sleep_ms_ac += delta; 

		thread_simulation();
		calculate_total_wait_time();

		if(is_total_wait_time_growing())
		{
			if(adjust_multiplier_ac != 1)
				adjust_multiplier_ac = 1;
			else
				break;
		}

		delta = adjust_multiplier_bd * adjustment_inc;
		printf("Changing green light for BD by %f\n", delta);
		sleep_ms_bd += delta; 

		thread_simulation();
		calculate_total_wait_time();

		if(is_total_wait_time_growing())
		{
			if(adjust_multiplier_bd != 1)
				adjust_multiplier_bd = 1;
			else
				break;
		}

		printf("--------------------------------------------------\n");
	}

	printf("Optimized green light values are:\nAC:%f\nBD:%f\n", prev_sleep_ac, prev_sleep_bd);

	destroy_semaphores();
    exit(0);
}


void simulate_bd_semaphores();
void simulate_ac_semaphores();
void restart_state();
void create_cars();

void thread_simulation()
{
	printf("....Simulating....\nAC Green: %f, BD Green: %f\n", sleep_ms_ac, sleep_ms_bd);

	restart_state();
	create_cars();

	while(cars_passed < total_cars_to_pass)
	{
		simulate_ac_semaphores();
		simulate_bd_semaphores();
	}
}

void *thread_auto(void *args)
{
	int direction = *((int *) args);
	sem_t* direction_sem;

	if(direction == dir_a)
		direction_sem = &a_sem;
	else if(direction == dir_c)
		direction_sem = &c_sem;
	else if(direction == dir_b)
		direction_sem = &b_sem;
	else if(direction == dir_d)
		direction_sem = &d_sem;
	else
		return NULL;

	clock_t started_waiting = clock();
	sem_wait(direction_sem);
	total_dir_wait_time_ms[direction] += clock() - started_waiting;
	sem_post(direction_sem);

	cars_passed++;
	// printf("car --> %c\n",65+direction);
}

void create_cars()
{
	pthread_t thread;
	for (int i = 0; i < CARS_A; ++i)
		pthread_create(&thread, NULL, thread_auto, (void*)&dir_a);
	for (int i = 0; i < CARS_B; ++i)
		pthread_create(&thread, NULL, thread_auto, (void*)&dir_b);
	for (int i = 0; i < CARS_C; ++i)
		pthread_create(&thread, NULL, thread_auto, (void*)&dir_c);
	for (int i = 0; i < CARS_D; ++i)
		pthread_create(&thread, NULL, thread_auto, (void*)&dir_d);
}

void restart_state()
{
	for (int i = 0; i < 4; ++i)
		total_dir_wait_time_ms[i] = 0;
	
	cars_passed = 0;
	prev_total_wait_time = total_wait_time;
	total_wait_time = 0;
}

void simulate_ac_semaphores()
{
	printf("A -> green\n");
	sem_post(&a_sem); // zeleno A
	printf("C -> green\n");
	sem_post(&c_sem); // zeleno C
	
	sleep(sleep_ms_ac); // traje zeleno
	
	printf("A -> red\n");
	sem_wait(&a_sem); // crveno A
	printf("C -> red\n");
	sem_wait(&c_sem); // crveno C
}

void simulate_bd_semaphores()
{
	printf("B -> green\n");
	sem_post(&b_sem); // zeleno B
	printf("D -> green\n");
	sem_post(&d_sem); // zeleno D

	sleep(sleep_ms_bd); // traje zeleno

	printf("B -> red\n");
	sem_wait(&b_sem); // crveno B
	printf("D -> red\n");
	sem_wait(&d_sem); // crveno D
}


void create_semaphores()
{
	sem_init(&a_sem, 0, 0);
	sem_init(&b_sem, 0, 0);
	sem_init(&c_sem, 0, 0);
	sem_init(&d_sem, 0, 0);

}

void destroy_semaphores()
{
	sem_destroy(&a_sem);
	sem_destroy(&b_sem);

	sem_destroy(&c_sem);
	sem_destroy(&d_sem);
}


float calculate_total_wait_time()
{
	float a_avg = total_dir_wait_time_ms[dir_a] / CARS_A;
	float b_avg = total_dir_wait_time_ms[dir_b] / CARS_B;
	float c_avg = total_dir_wait_time_ms[dir_c] / CARS_C;
	float d_avg = total_dir_wait_time_ms[dir_d] / CARS_D;

	total_wait_time = a_avg + b_avg + c_avg + d_avg;

	printf("Avg wait time for AC: %f\nAvg wait time for BD: %f\nTotal avg wait time: %f\n", a_avg + c_avg, b_avg + d_avg, total_wait_time); // je l ovako sabiram za AC I BD ili kao sto se total time racuna?

	return total_wait_time;
}

char is_total_wait_time_growing()
{
	return total_wait_time > prev_total_wait_time;
}