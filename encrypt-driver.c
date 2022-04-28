#include <stdio.h>
#include <stdlib.h>
#include "encrypt-module.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
    Authors: Erica Hollander & Lakin Jenkins
*/
//buffer = sizeof(char)

sem_t *sem_char_render;
sem_t *sem_count_in;
sem_t *sem_count_out;
int render_count, writer_count;
typedef struct circular_buf_t {
    char * buffer;
    int head;
    int tail;
    int max; //of the buffer
    int full;
}circular_buf_t;

circular_buf_t input_buf;
circular_buf_t output_buf;
int done[] = {0,0,0,0};

void reset_requested() {
    log_counts();
}

void reset_finished() {
}

size_t getCircBufSize(circular_buf_t buf){
    int size = buf.max;
    if(!buf.full)
    {
        if(buf.head >= buf.tail)
        {
            size = (buf.head - buf.tail);
        }
        else
        {
            size = (buf.max + buf.head - buf.tail);
        }
    }
return size;

}
static void addValue(circular_buf_t buf, char data, sem_t *sem_type){
    if(buf.full)
    {
        printf("make sem wait \n");
        sem_wait(sem_type);
       // buf.tail = (buf.tail + 1) % buf.max;
    }

    printf("ADD VALUE %c\n", data);
    //buf.head = (buf.head + 1) % buf.max;
    buf.full = (buf.head == buf.tail + 1);

    printf("BUFFER FULL %d, tail: %d\n", buf.full, buf.tail);
    buf.buffer[buf.tail] = data;
    printf("increment tail: %d, tail: %d, buf max: %d\n", (buf.tail + 1) % buf.max, buf.tail, buf.max);
    buf.tail = (buf.tail + 1) % buf.max;
    printf("tail: %d\n", buf.tail);


}
static void moveHead(circular_buf_t buf){
    printf("head: %d, tail %d\n", buf.head, buf.tail);
    if(buf.head != buf.tail)
    {
        printf("head: %d, tail %d\n", buf.head, buf.tail);
        buf.head = (buf.head + 1) % buf.max;
        buf.full = (buf.head == buf.tail+1);
    }

}
int getIndexOfCircBuf(circular_buf_t buf, int behind){
    int index;
    if(buf.tail-behind < 0){
        index = buf.max+(buf.tail-behind);
    }
    else{
        index = buf.tail-behind;
    }
    return index;
}

void *renderThread(void *vargp) {
    while (1) {
    //we might need a while loop in here? its only calling once
    printf("render %d\n", render_count);
    //addValue(input_buf, read_input(), sem_char_render);
    char data;
        if((data = read_input()) == EOF){
            printf("EOF");
            done[0] = 1;
            return 0;
            sem_post(sem_count_in);
        }
        if(done[3] == 1){
            return 0;
        }
        else {
            if (input_buf.full) {
                printf("make sem wait \n");
                sem_wait(sem_char_render);
                // buf.tail = (buf.tail + 1) % buf.max;
            }

            //printf("ADD VALUE %c\n", data);
            //buf.head = (buf.head + 1) % buf.max;
            input_buf.full = (input_buf.head == input_buf.tail + 1);

            //printf("BUFFER FULL %d, tail: %d\n", input_buf.full, input_buf.tail);
            input_buf.buffer[input_buf.tail] = data;
           // printf("increment tail: %d, tail: %d, buf max: %d\n", (input_buf.tail + 1) % input_buf.max, input_buf.tail, input_buf.max);
            input_buf.tail = (input_buf.tail + 1) % input_buf.max;
           // printf("tail: %d\n", input_buf.tail);

            //printf("tail: %d\n", input_buf.tail);
            render_count++;
            sem_post(sem_count_in);
        }
    }
}

void *inputCounterThread(void *vargp) {
    while (1) {
        if(done[0] == 0){
            sem_wait(sem_count_in);
        }
        else if(render_count == 12){
            return 0;
        }
        printf("input\n");

        printf("sem_posted\n");
        printf("render count: %d, counter: %d\n", render_count, get_input_total_count());
        if (render_count > get_input_total_count()) { //maybe we dont need this if cause we use sems
            count_input(input_buf.buffer[getIndexOfCircBuf(input_buf, render_count - get_input_total_count())]);
        }

    }

}
void *encryptThread(void *vargp){
    printf("encrypt\n");
    if(render_count-get_input_total_count() < input_buf.max && !output_buf.full){ // might need to be equal if issues debug there
        //encrypt();//count_input(input_buf.buf[getIndexOfCircBuf(input_buf, get_input_total_count()-render_count)]);
        printf("add THIS Value %c at %d\n", input_buf.buffer[input_buf.head], input_buf.head);
        addValue(output_buf, encrypt(input_buf.buffer[input_buf.head]), sem_count_out);
        moveHead(input_buf);
        sem_post(sem_char_render);
    }
}

void *outputCounterThread(void *vargp){
    printf("output\n");
    if(writer_count < get_output_total_count()){ //maybe we dont need this if cause we use sems
        count_output(input_buf.buffer[getIndexOfCircBuf(output_buf, get_output_total_count()-writer_count)]);
    }
}

void *writerThread(void *vargp){
    printf("writer\n");
    writer_count++;
    if(output_buf.head != output_buf.tail){
        printf("writer if\n");
        write_output(output_buf.head);
        moveHead(output_buf);
        sem_post(sem_count_out);
    }
}




int main(int argc, char *argv[]) {
	init("input.txt", "out.txt", "log.txt");//change to input
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
    //printf("got here");
    printf("input buffer %d\n", N);
    printf("output buffer %d\n", M);
    printf("got here\n");

//    input_buf = cb_init(input_buf, N);
//    output_buf = cb_init(output_buf, M);
//input_buf = malloc(circular_buf_t);
    //input_buf = malloc(sizeof(bool) + sizeof(size_t)*3 + sizeof(uint8_t)*N);
    //output_buf = malloc(sizeof(bool) + sizeof(size_t)*3 + sizeof(uint8_t)*M);

    input_buf.buffer = (char *)malloc(N * sizeof(char));
	input_buf.max = N;
	input_buf.full = 0;
	input_buf.head = 0;
	input_buf.tail = 0;
    printf("got here1\n");


    output_buf.buffer = (char *)malloc(M * sizeof(char));
    output_buf.max = M;
    output_buf.full = 0;
    output_buf.head = 0;
    output_buf.tail = 0;
    printf("got here 2\n");


    pthread_t reader,input, encrypt, output, writer;
    printf("tid\n");

    sem_char_render = sem_open("/sem_test_render", O_CREAT, 0644, 0);
    sem_unlink("/sem_test_render");
    sem_count_in = sem_open("/sem_test_count_in", O_CREAT, 0644, 0);
    sem_unlink("/sem_test_count_in");
    sem_count_out = sem_open("/sem_test_count_out", O_CREAT, 0644, 0);
    sem_unlink("/sem_test_count_out");
    printf("sems created\n");

    pthread_create(&reader, NULL, renderThread, NULL);
    pthread_create(&input, NULL, inputCounterThread, NULL);
    /*
    pthread_create(&encrypt, NULL, encryptThread, NULL);
    pthread_create(&output, NULL, outputCounterThread, NULL);
    pthread_create(&writer, NULL, writerThread, NULL);*/
    pthread_join(reader, NULL);
    pthread_join(input, NULL);
    /*
    pthread_join(encrypt, NULL);
    pthread_join(output, NULL);
    pthread_join(writer, NULL);
     */
    printf("pthread create\n");


    //pthread_exit(NULL);
	printf("End of file reached.\n");
	//log_counts();
	printf("log_counts\n");
}
