#include <stdlib.h>
#include <stdio.h>
#include "genetic_algorithm.h"
#include <unistd.h>
#include <pthread.h>



int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// numar de tread-uri
	int p = 0;

	if (!read_input(&objects, &object_count, &p, &sack_capacity, &generations_count, argc, argv)) {
		return 0;
	}

	struct param argss[p];
	pthread_t tid[p];

	pthread_barrier_t* barrier = (pthread_barrier_t*) malloc(sizeof(pthread_barrier_t));
	pthread_barrier_init(barrier, NULL, p);
	individual* current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual* next_generation = (individual*) calloc(object_count, sizeof(individual));
	individual* tmp = NULL;


	for (int i = 0; i < p; i++) {
		argss[i].objects = (sack_object*) calloc(object_count, sizeof(sack_object));
		argss[i].object_count = object_count;
		argss[i].objects = objects;
		argss[i].generations_count = generations_count;
		argss[i].sack_capacity = sack_capacity;
		argss[i].p = p;
		argss[i].thread_id = i;
		argss[i].barrier = barrier;
		argss[i].current_generation = current_generation;
		argss[i].next_generation = next_generation;
		argss[i].tmp = tmp;
		pthread_create(&tid[i], NULL, &run_genetic_algorithm, (void*) &argss[i]);
	}

	for (int i = 0; i < p; i++) {
		pthread_join(tid[i], NULL);
	}

	free(objects);

	return 0;
}
