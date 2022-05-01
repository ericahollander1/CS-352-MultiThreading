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
sem_t *sem_encrypt1;
sem_t *sem_encrypt2;
sem_t *sem_writer;
int render_count, writer_count, encrypt_count;
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
            //return 0;
            sem_post(sem_count_in);
            return 0;
        }
        if(done[3] == 1){
            return 0;
        }
        else {
            //printf("ADD VALUE %c\n", data);
            //buf.head = (buf.head + 1) % buf.max;
            if(input_buf.tail == input_buf.max-1){
                printf("full\n");
                input_buf.full = (input_buf.head == 0);
            }
            else{
                input_buf.full = (input_buf.head == input_buf.tail+1);
            }



            printf("BUFFER FULL %d, tail: %d, head: %d\n", input_buf.full, input_buf.tail, input_buf.head);
            input_buf.buffer[input_buf.tail] = data;

            printf("input_buf[");
            for(int i = 0; i < input_buf.max; i++){
                printf("%c, ", input_buf.buffer[i]);

            }
            printf("]\noutput_buf[");
            for(int i = 0; i < output_buf.max; i++){
                printf("%c, ", output_buf.buffer[i]);

            }
            printf("]\n");

            if (input_buf.full) {
                printf("make sem wait \n");
                sem_wait(sem_char_render);
                // buf.tail = (buf.tail + 1) % buf.max;
            }

            input_buf.tail = (input_buf.tail + 1) % input_buf.max;

           // printf("increment tail: %d, tail: %d, buf max: %d\n", (input_buf.tail + 1) % input_buf.max, input_buf.tail, input_buf.max);
            //input_buf.tail = (input_buf.tail + 1) % input_buf.max;
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
        else if(render_count == get_input_total_count()){
           done[1] = 1;
           printf("INPUT DONE");
            return 0;
        }
        printf("input\n");

        printf("render count: %d, counter: %d\n", render_count, get_input_total_count());
        if (render_count > get_input_total_count()) { //maybe we dont need this if cause we use sems
            count_input(input_buf.buffer[getIndexOfCircBuf(input_buf, render_count - get_input_total_count())]);
            sem_post(sem_encrypt1);
        }

    }

}
void *encryptThread(void *vargp){
    while(1){
        //printf("encrypt 1: %d, 2: %d\n", render_count-get_input_total_count(), !output_buf.full);
        if(done[1] == 0) {
            sem_wait(sem_encrypt1);
        }
            //sem_wait(sem_count_out);
            //if(render_count-get_input_total_count() < input_buf.max && !output_buf.full){ // might need to be equal if issues debug there
            printf("encrypt\n");
            //encrypt();//count_input(input_buf.buf[getIndexOfCircBuf(input_buf, get_input_total_count()-render_count)]);
            //printf("add THIS Value %c at %d\n", input_buf.buffer[input_buf.head], input_buf.head);
            // addValue(output_buf, encrypt(input_buf.buffer[input_buf.head]), sem_count_out);
            if (output_buf.tail == output_buf.max - 1) {
                //printf("full\n");
                output_buf.full = (output_buf.head == 0);
            } else {
                output_buf.full = (output_buf.head == output_buf.tail + 1);
            }

            //printf("Encrypt BUFFER FULL %d, tail: %d, head: %d\n", output_buf.full, output_buf.tail, output_buf.head);
            char encryptchar = encrypt(input_buf.buffer[input_buf.head]);
            output_buf.buffer[output_buf.tail] = encryptchar;

            if (output_buf.full) {
                printf("make sem wait out \n");
                sem_wait(sem_count_out);
            }
            output_buf.tail = (output_buf.tail + 1) % output_buf.max;
            encrypt_count++;
            //moveHead(input_buf);
            printf("head: %d, tail %d\n", input_buf.head, input_buf.tail);
            if (input_buf.head != input_buf.tail) {
                printf("head!=tail\n");
                input_buf.head = (input_buf.head + 1) % input_buf.max;
                input_buf.full = (input_buf.head == input_buf.tail + 1);
            }

                printf("input !done\n");
            if(done[0] == 0){
                printf("func1 is done");
                sem_post(sem_char_render);
            }
            printf("post count out");
            sem_post(sem_count_out);



        if(done[1] == 1){//output_buf.max == output_buf.tail-1 || render_count == 12){//TAKE THIS OUT!!!
            printf("encrypt post DONE encrypt count = %d, input_total_count == %d, output_count = %d\n", encrypt_count, get_input_total_count(), get_output_total_count());
            sem_post(sem_count_out);
            if(encrypt_count == get_input_total_count()){
                done[2] = 1;
                printf("END OF ENCRYPT\n");
                return 0;
            }
        }
    }

}

void *outputCounterThread(void *vargp){

    while(1){
        if(done[2] == 1){
            sem_post(sem_writer);
            if(get_input_total_count() == render_count){
                printf("OUTPUT DONE\n");
                done[3] = 1;
                return 0;
            }

        }
        else{
            sem_wait(sem_count_out);
        }

        printf("output %d\n", get_output_total_count());
        //if(writer_count < get_output_total_count()){ //maybe we dont need this if cause we use sems
            count_output(input_buf.buffer[getIndexOfCircBuf(output_buf, get_output_total_count()-writer_count)]);
            sem_post(sem_writer);
            //sem_post(sem_encrypt2);
       // }


    }

}

void *writerThread(void *vargp){
    while(1) {
        printf("writer\n");
        if(done[3] == 0){
            sem_wait(sem_writer);
        }

        //if (output_buf.head != output_buf.tail) {
            printf("writer if\n");
            write_output(output_buf.buffer[output_buf.head]);
            //moveHead(output_buf);
            printf("head: %d, tail %d\n", output_buf.head, output_buf.tail);
            if (output_buf.head != output_buf.tail) {
                printf("head: %d, tail %d\n", output_buf.head, output_buf.tail);
                output_buf.head = (output_buf.head + 1) % output_buf.max;
                output_buf.full = (output_buf.head == output_buf.tail + 1);
            }
            writer_count++;
            //sem_post(sem_count_out);
        //}
        if(done[3] == 1 && writer_count == render_count){
            printf("DONE WITH IT ALL\n");
            return 0;
        }
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
    sem_encrypt1 = sem_open("/sem_encrypt1", O_CREAT, 0644, 0);
    sem_unlink("/sem_encrypt1");
    sem_encrypt2 = sem_open("/sem_encrypt2", O_CREAT, 0644, 0);
    sem_unlink("/sem_encrypt2");
    sem_writer = sem_open("/sem_writer", O_CREAT, 0644, 0);
    sem_unlink("/sem_writer");
    printf("sems created\n");

    pthread_create(&reader, NULL, renderThread, NULL);
    pthread_create(&input, NULL, inputCounterThread, NULL);

    pthread_create(&encrypt, NULL, encryptThread, NULL);
    pthread_create(&output, NULL, outputCounterThread, NULL);
    pthread_create(&writer, NULL, writerThread, NULL);
    pthread_join(reader, NULL);
    pthread_join(input, NULL);

    pthread_join(encrypt, NULL);
    pthread_join(output, NULL);
    pthread_join(writer, NULL);

    printf("pthread create\n");


    //pthread_exit(NULL);
	printf("End of file reached.\n");
	log_counts();
	printf("log_counts\n");
}
