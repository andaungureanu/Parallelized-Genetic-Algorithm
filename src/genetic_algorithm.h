#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include "sack_object.h"
#include "individual.h"
#include <pthread.h>

// structura de argumente pentru thread function
struct param {
	sack_object* objects;
	int object_count;
	int generations_count;
	int sack_capacity;
	int p;
	int thread_id;
	pthread_barrier_t* barrier;
	individual *current_generation;
	individual *next_generation;
	individual *tmp;
};

#define min(x, y) (((x) < (y)) ? (x) : (y))

// reads input from a given file
int read_input(sack_object **objects, int *object_count, int* p, int *sack_capacity, int *generations_count, int argc, char *argv[]);

// displays all the objects that can be placed in the sack
void print_objects(const sack_object *objects, int object_count);

// displays all or a part of the individuals in a generation
void print_generation(const individual *generation, int limit);

// displays the individual with the best fitness in a generation
void print_best_fitness(const individual *generation, int thread_id);

// computes the fitness function for each individual in a generation
void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int thread_id, int p, int sack_capacity);

// compares two individuals by fitness and then number of objects in the sack (to be used with qsort)
int cmpfunc(individual first, individual second);

// performs a variant of bit string mutation
void mutate_bit_string_1(const individual *ind, int generation_index);

// performs a different variant of bit string mutation
void mutate_bit_string_2(const individual *ind, int generation_index);

// performs one-point crossover
void crossover(individual *parent1, individual *child1, int generation_index);

// copies one individual
void copy_individual(const individual *from, const individual *to);

// deallocates a generation (paralel)
void free_generation(individual *generation, int thread_id, int p);

// free generation neparalel
void free_gen(individual* generation);

// merge-ul clasic
void merge(int low, int mid, int high, individual* current_generation);

// merge-sort clasic
void merge_sort(int low, int high, individual* current_generation);

// merge-sort paralel - imparte vectorul in p parti apoi fiecare thread face merge-sort pe partea asignata lui
void merge_sort_par(individual* current_generation, int object_count, int thread_id, int p);

// face merge la final intre partile din vector ce au fost impartite initial la cele p thread-uri
void merge_threads(individual* current_generation, int object_count, int p);

// runs the genetic algorithm
void* run_genetic_algorithm(void* arguments);

#endif