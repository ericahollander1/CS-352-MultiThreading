#include <stdio.h>
#include <stdlib.h>
#include "encrypt-module.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/*
    Authors: Erica Hollander & Lakin Jenkins
*/
//buffer = sizeof(char)
typedef struct circular_buf_t {
    uint8_t * buffer;
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    bool full;
}circular_buf_t;

void reset_requested() {
	log_counts();
}

void reset_finished() {
}

int main(int argc, char *argv[]) {
	init("in.txt", "out.txt", "log.txt");
	char c;
	int N = 0, M = 0;
	while(N<2) {
        printf("What input buffer size to use?\n");
        scanf("%d", &N);
    }
	while(M<2) {
        printf("What output buffer size to use?\n");
        scanf("%d", &M);
    }
    printf("input buffer %d\n", N);
    printf("output buffer %d\n", M);
	circular_buf_t *input_buf = malloc(sizeof *input_buf);
	input_buf->buffer = sizeof(char);
	input_buf->max = N;
	input_buf->full = 0;
	input_buf->head = 0;
	input_buf->tail = 0;

    circular_buf_t *output_buf = malloc(sizeof *output_buf);
    output_buf->buffer = sizeof(char);
    output_buf->max = M;
    output_buf->full = 0;
    output_buf->head = 0;
    output_buf->tail = 0;

/*    pthread_t tid;
    pthread_create(&tid, NULL, myThreadFun, (void *)&tid);
    pthread_exit(NULL);*/

	while ((c = read_input()) != EOF) {
		count_input(c);
		c = encrypt(c);
		count_output(c);
		write_output(c);
	}
	printf("End of file reached.\n");
	log_counts();
}
