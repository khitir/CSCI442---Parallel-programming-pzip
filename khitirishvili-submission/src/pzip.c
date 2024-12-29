#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pzip.h"

#define NUM_CHARS 26
pthread_mutex_t lock;
//int *global_array;
int *global_char_frequency;
int global_array[26] = {0};

struct zipped_char *global_zipped_chars;

struct ThreadInfo {
	int thread_id;
	char *input_chars;
	int input_chars_size;
	struct zipped_char *local_zipped_chars;
	pthread_barrier_t *barrier;
	int n_threads;
	int number_of_compressions;
	int insertion_point;
	int *zipped_chars_count;
};

void *process_characters(void *arg)
{
	struct ThreadInfo *info = (struct ThreadInfo *)arg;
	int *zipped_chars_count = info->zipped_chars_count;
	int thread_id = info->thread_id;
	char *input_chars = info->input_chars;
	int input_chars_size = info->input_chars_size;
	struct zipped_char *local_zipped_chars = info->local_zipped_chars;
	pthread_barrier_t *barrier = info->barrier;
	int n_threads = info->n_threads; // Retrieve the number of threads
	//int insertion_point = info->insertion_point;

	// Calculate the portion of characters each thread should handle
	int chunk_size = input_chars_size / n_threads;
	int start = thread_id * chunk_size;
	int end;

	if (thread_id == info->n_threads - 1) {
		end = input_chars_size;
	} else {
		end = start + chunk_size;
	}

	// Initialize local variables for tracking consecutive occurrences
	char current_char = input_chars[start];
	int char_count = 1;
	int chars = 0;
	global_char_frequency[input_chars[start] - 'a'] ++;

	// Process the characters in the assigned portion
	for (int i = start + 1; i < end; i++) {
		global_char_frequency[input_chars[i] - 'a'] ++;
		if (input_chars[i] == current_char) {
			char_count++;
		}

		else {
			// Store the consecutive occurrences in local_zipped_chars
			local_zipped_chars[chars].character = current_char;
			local_zipped_chars[chars].occurence = char_count;
			//pthread_mutex_lock(&lock);
			
			//pthread_mutex_unlock(&lock);

			// Update char_frequency and reset local variables
			chars++;
			current_char = input_chars[i];
			char_count = 1;
		}
	}
	local_zipped_chars[chars].character = current_char;
	local_zipped_chars[chars].occurence = char_count;
	chars++;

	for (int i = start; i < end; i++) {
		global_array[thread_id] = chars;
		// printf("global_array[%d] = %c\n", thread_id, global_array[thread_id]);
	}

	pthread_barrier_wait(barrier);


    pthread_mutex_lock(&lock);
    *zipped_chars_count += chars;
	pthread_mutex_unlock(&lock);

	int sum = 0;
	for (int i = 0; i < thread_id; i++) {
		sum += global_array[i];
	}

	//printf("Made it here!\n");

	pthread_mutex_lock(&lock);
	for (int j = 0; j < chars; j++) {
		global_zipped_chars[sum + j].character =
			local_zipped_chars[j].character;
		global_zipped_chars[sum + j].occurence =
			local_zipped_chars[j].occurence;
	}
	pthread_mutex_unlock(&lock);

	return NULL;
}

void pzip(int n_threads, char *input_chars, int input_chars_size,
	  struct zipped_char *zipped_chars, int *zipped_chars_count,
	  int *char_frequency)
{
	global_char_frequency = char_frequency;
	global_zipped_chars = zipped_chars;


	pthread_t threads[n_threads];
	struct ThreadInfo
		thread_info[n_threads]; // create struct of size n_threads

	pthread_barrier_t barrier;
	// pthread_mutex_t mutex;

	pthread_barrier_init(&barrier, NULL, n_threads);
	pthread_mutex_init(&lock, NULL);

	// Create and run threads
	for (int i = 0; i < n_threads; i++) {
		thread_info[i].thread_id = i;
		thread_info[i].n_threads = n_threads;
		thread_info[i].input_chars = input_chars;
		thread_info[i].input_chars_size = input_chars_size;
		thread_info[i].local_zipped_chars =
			(struct zipped_char *)malloc(
				input_chars_size * sizeof(struct zipped_char));
		thread_info[i].barrier = &barrier;
		thread_info[i].insertion_point =
			i * input_chars_size / n_threads;
		thread_info[i].zipped_chars_count = zipped_chars_count;

		pthread_create(&threads[i], NULL, process_characters,
			       (void *)&thread_info[i]);
	}

	for (int i = 0; i < n_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	for (int i = 0; i < n_threads; i++) {
		free(thread_info[i].local_zipped_chars);
	}

	// Clean up resources
	pthread_barrier_destroy(&barrier);
	pthread_mutex_destroy(&lock);
}

