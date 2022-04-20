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
sem_t *sem_char_render;
sem_t *sem_count_in;
sem_t *sem_count_out;
int render_count;
typedef struct circular_buf_t {
    uint8_t * buffer;
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    bool full;
}circular_buf_t;

circular_buf_t *input_buf = malloc(sizeof *input_buf);

circular_buf_t *output_buf = malloc(sizeof *output_buf);

void reset_requested() {
    // stop the reader thread from reading any more input
    // make sure it is safe to reset
    log_counts();
}
void rest_finished() {
    // resume the reader thread
}

size_t getCircBufSize(circular_buf_t buf){
    size_t size = buf->max;
    if(!buf->full)
    {
        if(buf->head >= buf->tail)
        {
            size = (buf->head - buf->tail);
        }
        else
        {
            size = (buf->max + buf->head - buf->tail);
        }
    }
return size;

}
static void addValue(circular_buf_t buf, uint8_t data, sem_t sem_type){
    if(buf->full)
    {
        sem_wait(sem_type);
        buf->tail = (buf->tail + 1) % buf->max;
    }

    //buf->head = (buf->head + 1) % buf->max;
    buf->full = (buf->head == buf->tail + 1);
    buf->buffer[buf->tail] = data;

}
static void moveHead(circular_buf_t buf){
    if(buf->head != buf->tail)
    {
        buf->head = (buf->head + 1) % buf->max;
        buf->full = (buf->head == buf->tail+1);
    }

}
int getIndexOfCircBuf(circular_buf_t buf, int behind){
    int index = buf->tail;
    if(buf->tail-behind < 0){
        index = buf->max+(buf->tail-behind);
    }
    else{
        index = buf->tail-behind;
    }
    return index;
}

void *renderThread(void *vargp){
    addValue(input_buf, read_input(), sem_char_render);
    render_count++;
    sem_post(sem_count_in);
}

void *inputCounterThread(void *vargp){
    sem_wait(sem_count_in);
    if(render_count > get_input_total_count()){ //maybe we dont need this if cause we use sems
        count_input(input_buf->buf[getIndexOfCircBuf(input_buf, render_count-get_input_total_count())]);
    }
}
void *encryptThread(void *vargp){
    if(render_count-get_input_total_count() < input_buf->max && !output_buf->full){ // might need to be equal if issues debug there
        //encrypt();//count_input(input_buf->buf[getIndexOfCircBuf(input_buf, get_input_total_count()-render_count)]);
        addValue(output_buf, encrypt(), sem_count_out);
        moveHead(input_buf);
        sem_post(sem_char_render);
    }
}

void *outputCounterThread(void *vargp){
    if(writer_count < get_output_total_count()){ //maybe we dont need this if cause we use sems
        count_output(input_buf->buf[getIndexOfCircBuf(output_buf, get_output_total_count()-writer_count)]);
    }
}

void *writerThread(void *vargp){
    writer_count++;
    if(output_buf->head != tail){
        write_output(output_buf->head);
        moveHead(output_buf);
        sem_post(sem_count_out);
    }
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

	input_buf->buffer = (uint8_t *)malloc(N * sizeof(uint8_t));
	input_buf->max = N;
	input_buf->full = 0;
	input_buf->head = 0;
	input_buf->tail = 0;

    output_buf->buffer = (uint8_t *)malloc(M * sizeof(uint8_t));
    output_buf->max = M;
    output_buf->full = 0;
    output_buf->head = 0;
    output_buf->tail = 0;

    pthread_t tid;
    sem_char_render = sem_open("/sem_test_render", O_CREAT, 0644, 0);
    sem_unlink("/sem_test_render");
    sem_count_in = sem_open("/sem_test_count_in", O_CREAT, 0644, 0);
    sem_unlink("/sem_test_count_in");
    sem_count_out = sem_open("/sem_test_count_out", O_CREAT, 0644, 0);
    sem_unlink("/sem_test_count_out");
    pthread_create(&tid, NULL, renderThread, (void *)&tid);
    pthread_create(&tid, NULL, countInThread, (void *)&tid);
    pthread_create(&tid, NULL, countOutThread, (void *)&tid);

    //pthread_exit(NULL);

	while ((c = read_input()) != EOF) {
	    count_input(c);
		c = encrypt(c);
		count_output(c);
		write_output(c);
	}
	printf("End of file reached.\n");
	log_counts();
}
