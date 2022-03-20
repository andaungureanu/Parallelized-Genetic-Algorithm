#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "genetic_algorithm.h"


int read_input(sack_object **objects, int *object_count, int* p, int *sack_capacity, int *generations_count, int argc, char *argv[]) {
	FILE *fp;

	if (argc != 4) {
		fprintf(stderr, "Usage:\n\t./main in_file generations_count threads_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	*p = (int) strtol(argv[3], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count) {
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit) {
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}
		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation, int thread_id) {
	if (thread_id == 0) {
		printf("%d\n", generation[0].fitness);
	}
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int thread_id, int p, int sack_capacity) {
	int weight;
	int profit;
	int start = thread_id * (double) object_count/p;
	int end = min((thread_id + 1) * (double) object_count/p, object_count);

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(individual first, individual second) {
	int i;
	int res = second.fitness - first.fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;
		for (i = 0; i < first.chromosome_length && i < second.chromosome_length; ++i) {
			first_count += first.chromosomes[i];
			second_count += second.chromosomes[i];
		}
		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second.index - first.index;
		}
	}
	return res;
}

void merge(int low, int mid, int high, individual* current_generation) {
    individual* left = (individual*) malloc((mid - low + 1) * sizeof(individual));
    individual* right = (individual*) malloc((high - mid) * sizeof(individual));
    int n1 = mid - low + 1, n2 = high - mid; 
    for (int i = 0; i < n1; i++) {
        left[i] = current_generation[i + low];
    }
    for (int i = 0; i < n2; i++) {
        right[i] = current_generation[i + mid + 1];
    }
    int k = low;
    int i = 0, j = 0;
    while (i < n1 && j < n2) {
        if (cmpfunc(left[i], right[j]) <= 0) {	
            current_generation[k++] = left[i++];
        } else {
            current_generation[k++] = right[j++];
        }
    }
    while (i < n1) {
        current_generation[k++] = left[i++];
    }
    while (j < n2) {
        current_generation[k++] = right[j++];
    }
}
 
void merge_sort(int low, int high, individual* current_generation) {
    int mid = low + (high - low) / 2;
    if (low < high) {
        merge_sort(low, mid, current_generation);
        merge_sort(mid + 1, high, current_generation);
        merge(low, mid, high, current_generation);
    }
}
 
void merge_sort_par(individual* current_generation, int object_count, int thread_id, int p) {
	// calculcam low si high in functie de ce thread avem pentru a trimite fiecare
	// thread pe portiunea lui de vector
    int start = thread_id * ((double)object_count / p);
    int end = (thread_id + 1) * ((double)object_count / p) - 1;
    int mid = start + (end - start) / 2;
    if (start < end) {
        merge_sort(start, mid, current_generation);
        merge_sort(mid + 1, end, current_generation);
        merge(start, mid, end, current_generation);
    }
}

// functie pentru a face merge pe partile de vector create anterior
void merge_threads(individual* current_generation, int object_count, int p) {
    int** a = (int**) malloc (p * sizeof(int*));
    for (int i = 0; i < p; i++) {
        a[i] = (int*) malloc (2 * sizeof(int));
    }
    int start, end;
    int low, high;
    int cnt = p;
    int mid;
    for (int i = 0; i < p; i++) {
        start = i * (double)object_count/p;
        end = (i + 1) * (double)object_count/p - 1;
        a[i][0] = start;
        a[i][1] = end;
    }
	// matrice in care retin start si end (low si high) pentru fiecare thread
    while (cnt != 1) { // pana cand terminam toate merge-urile
        if (cnt % 2 == 0) {
			// parcurgem matricea si facem merge intre parti 2 cate 2 si salvam
			// pe prima linie noile valori de low si high, stergand apoi 
			// urmatoarea linie folosita pentru merge-ul anterior
            for (int i = 0; i < cnt; i++) {
                low = a[i][0];
                high = a[i + 1][1];
                mid = a[i][1];
                a[i][0] = low;
                a[i][1] = high;
                merge(low, mid, high, current_generation);
                for (int j = i + 1; j < cnt - 1; j++) {
                    a[j][0] = a[j + 1][0];
                    a[j][1] = a[j + 1][1];
                }
                cnt--; // scadem numarul de elemente
            }
        }
        if (cnt % 2) {
			// la fel ca mai sus, doar ca pentru numar impar de elemente
            for (int i = 0; i < cnt - 1; i++) {
                low = a[i][0];
                high = a[i + 1][1];
                mid = a[i][1];
                a[i][0] = low;
                a[i][1] = high;
                merge(low, mid, high, current_generation);
                for (int j = i + 1; j < cnt - 1; j++) {
                    a[j][0] = a[j + 1][0];
                    a[j][1] = a[j + 1][1];
                }
                cnt--;
            }
        }
    }
	// dau free la matricea alocata mai sus
    int i;
    for (i = 0; i < p; ++i) {
        free(a[i]);
    }
    free(a);
}

void mutate_bit_string_1(const individual *ind, int generation_index) {
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index) {
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index) {
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to) {
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

// free generation paralel
void free_generation(individual *generation, int thread_id, int p) {
	int i;
	int start = thread_id * (double) generation -> chromosome_length/p;
	int end = min((thread_id + 1) * (double) generation -> chromosome_length/p, generation -> chromosome_length);
	for (i = start; i < end; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

// free generation neparalel
void free_gen(individual *generation) {
	int i;
	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void* run_genetic_algorithm(void* arguments) {

	struct param* argss = (struct param*) arguments;
	int object_count = argss -> object_count;
	int generations_count = argss -> generations_count;
	int sack_capacity = argss -> sack_capacity;
	int p = argss -> p;
	int thread_id = argss -> thread_id;
	sack_object* objects = (sack_object*) calloc(object_count, sizeof(sack_object));
	objects = argss -> objects;

	int start, end;
	int count, cursor;

	start = thread_id * (double) object_count/p;
	end = min((thread_id + 1) * (double) object_count/p, object_count);
	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = start; i < end; ++i) {
		argss -> current_generation[i].fitness = 0;
		argss -> current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		argss -> current_generation[i].chromosomes[i] = 1;
		argss -> current_generation[i].index = i;
		argss -> current_generation[i].chromosome_length = object_count;

		argss -> next_generation[i].fitness = 0;
		argss -> next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		argss -> next_generation[i].index = i;
		argss -> next_generation[i].chromosome_length = object_count;
	}
	pthread_barrier_wait(argss -> barrier);

	// iterate for each generation
	for (int k = 0; k < generations_count; ++k) {
		pthread_barrier_wait(argss -> barrier);
		cursor = 0;
		// compute fitness and sort by it
		compute_fitness_function(objects, argss -> current_generation, object_count, thread_id, p, sack_capacity);
		pthread_barrier_wait(argss -> barrier);
		merge_sort_par(argss -> current_generation, object_count, thread_id, p);
		pthread_barrier_wait(argss -> barrier);
		// pun doar thread-ul 0 sa faca aceasta parte secventiala
		if (thread_id == 0) {
			merge_threads(argss -> current_generation, object_count, p);
		}
		pthread_barrier_wait(argss -> barrier);
		// keep first 30% children (elite children selection)
		count = object_count * 3 / 10;
		start = thread_id * (double) count/p;
		end = min((thread_id + 1) * (double) count/p, count);
		for (int i = start; i < end; ++i) {
			copy_individual(argss -> current_generation + i, argss -> next_generation + i);
		}
		pthread_barrier_wait(argss -> barrier);
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = object_count * 2 / 10;
		start = thread_id * (double) count/p;
		end = min((thread_id + 1) * (double) count/p, count);
		for (int i = start; i < end; ++i) {
			copy_individual(argss -> current_generation + i, argss -> next_generation + cursor + i);
			mutate_bit_string_1(argss -> next_generation + cursor + i, k);
		}
		pthread_barrier_wait(argss -> barrier);
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = object_count * 2 / 10;
		start = thread_id * (double) count/p;
		end = min((thread_id + 1) * (double) count/p, count);
		for (int i = start; i < end; ++i) {
			copy_individual(argss -> current_generation + i + count, argss -> next_generation + cursor + i);
			mutate_bit_string_2(argss -> next_generation + cursor + i, k);
		}
		pthread_barrier_wait(argss -> barrier);
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = object_count * 3 / 10;
		// copierea unui singur individ (secventiala) o asignez doar thread-ului 0
		if (count % 2 == 1 && thread_id == 0) {
			copy_individual(argss -> current_generation + object_count - 1, argss -> next_generation + cursor + count - 1);
			count--;
		}
		if (count % 2 == 1 && thread_id != 0) {
			count--;
		}
		pthread_barrier_wait(argss -> barrier);
		start = thread_id * (double) count/p;
		end = min((thread_id + 1) * (double) count/p, count);
		if (start % 2) {
			start++;
		}
		for (int i = start; i < count - 1 && i < end; i += 2) {
			crossover(argss -> current_generation + i, argss -> next_generation + cursor + i, k);
		}
		pthread_barrier_wait(argss -> barrier);

		// switch to new generation
		argss -> tmp = argss -> current_generation;
		argss -> current_generation = argss -> next_generation;
		argss -> next_generation = argss -> tmp;
		pthread_barrier_wait(argss -> barrier);

		start = thread_id * (double) object_count/p;
		end = min((thread_id + 1) * (double) object_count/p, object_count);
		for (int i = start; i < end; ++i) {
			argss -> current_generation[i].index = i;
		}
		pthread_barrier_wait(argss -> barrier);

		if (k % 5 == 0) {
			print_best_fitness(argss -> current_generation, thread_id);
		}
	}

	pthread_barrier_wait(argss -> barrier);
	compute_fitness_function(objects, argss -> current_generation, object_count, thread_id, p, sack_capacity);
	pthread_barrier_wait(argss -> barrier);
	merge_sort_par(argss -> current_generation, object_count, thread_id, p);
	pthread_barrier_wait(argss -> barrier);
	if (thread_id == 0) {
		merge_threads(argss -> current_generation, object_count, p);
	}
	pthread_barrier_wait(argss -> barrier);
	print_best_fitness(argss -> current_generation, thread_id);

	// free resources for old generation
	pthread_barrier_wait(argss -> barrier);
	free_generation(argss -> current_generation, thread_id, p);
	free_generation(argss -> next_generation, thread_id, p);
	pthread_barrier_wait(argss -> barrier);
	// las doar thread-ul 0 sa faca free-ul final, fiind nevoie ca acesta sa se
	// faca o singura data
	if (thread_id == 0) {
		free(argss -> current_generation);
		free(argss -> next_generation);
	}	

	// free resources

	pthread_exit(NULL);
}
