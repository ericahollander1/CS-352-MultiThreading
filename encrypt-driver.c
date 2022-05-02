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

    Please see the readme for specific changelog information
*/


/*
 * GLOBAL VARIABLES
 *
 * Here are the semaphore that we use for this project. They are globally initialized, so the entire file can access them
 *
 * In addition, we initialize render_count, writer_count, and encrypt count globally because we
 * also use these throughout the file
 *
 * For our buffer data structure, we used a circular buffer. This struct is created and we globally declare the
 * two buffers, input_buf and output_Buf
 *
 * The array done is an array of integers acting as booleans to help our semaphores avoid deadlock
 *
 * keyChanged is another integer acting as a boolean to help the process of when the key randomly changes
 */

sem_t *sem_char_render;
sem_t *sem_count_in;
sem_t *sem_count_out;
sem_t *sem_encrypt1;
sem_t *sem_encrypt2;
sem_t *sem_writer;
sem_t *sem_key;
sem_t *sem_key_return;
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
int done[] = {0,0,0,0,0};
int keyChanged = 0;


/*
 * Reset requested sets encryption module in a blocked state
 * When it is safe to reset it unblocks the encryption module
 * Stops the reader thread from reading anymore input
 */
void reset_requested() {
    // stop the reader thread from reading any more input
    // make sure it is safe to reset
    keyChanged = 1;
    printf("HELP ME\n");
    sem_wait(sem_key_return);
    printf("sem_key_return\n");
    log_counts();
}


/*
 * Reset finished resumes all the threads' processes with the new key
 */
void reset_finished() {
    // resume the reader thread
    printf("RESET FINISHED");
    done[0] = 0;
    done[1] = 0;
    done[2] = 0;
    done[3] = 0;
    done[4] = 0;
    keyChanged = 0;
    render_count = 0;
    encrypt_count = 0;
    writer_count = 0;
    sem_post(sem_key);
}

/*
 * gets the size of the circular queue buffer
 * returns the size of the circular queue buffer
 */
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

/*
 * adds a value to the circular queue buffer
 */
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


/*
 * get the index of the circular buffer
 * returns the index
 */
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


/*
 *Render thread is the thread for reading in the data from the input file
 * This takes in the character and adds it to the input buffer
 */
void *renderThread(void *vargp) {
    while (1) {
        //we might need a while loop in here? its only calling once
        printf("render %d\n", render_count);
        //addValue(input_buf, read_input(), sem_char_render);
        char data;
        if(keyChanged == 1){
            //done[0] = 1;
            //sem_post(sem_count_in);
            printf(" keyChanged \n");
            if(done[0] == 0){
                sem_post(sem_count_in);
                done[0] = 1;
                sem_wait(sem_key);
            }
        }

        printf("rendercheck\n");
        if((data = read_input()) == EOF ){
            printf("EOF");
            done[0] = 1;
            //return 0;
            sem_post(sem_count_in);
            return 0;
        }
        printf("rendercheck3\n");
        /*if(done[3] == 1){
            printf("rendercheck2\n");
            return 0;
        }*/
       // else {
            //printf("ADD VALUE %c\n", data);
            //buf.head = (buf.head + 1) % buf.max;
            printf("rendercheck1\n");
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
      //  }
    }
}


/*
 *The input counter is the thread that counts the number of characters from the file that we have counted.
 */
void *inputCounterThread(void *vargp) {
    while (1) {

        if(done[0] == 0){
            sem_wait(sem_count_in);
        }
        printf("input\n");

        printf("render count: %d, counter: %d\n", render_count, get_input_total_count());
        if (render_count > get_input_total_count()) { //maybe we dont need this if cause we use sems
            count_input(input_buf.buffer[getIndexOfCircBuf(input_buf, render_count - get_input_total_count())]);
            sem_post(sem_encrypt1);
        }

        if(done[0] == 1 && render_count == get_input_total_count()){
            done[1] = 1;
            printf("INPUT DONE");
            sem_post(sem_encrypt1);
            if(keyChanged == 1){
                sem_post(sem_encrypt1);
                sem_wait(sem_count_in);
                printf("\n\nWE RESET!!!!\n\n");
            }
            else{
                //sem_post(sem_encrypt1);
                printf("WE STOPPED INPUT");
                return 0;
            }

        }
        //if(done[1] == 0) {

        // }
    }

}

/*
 * Encrypt Thread is the thread that calls encrypt module's encrypt function
 * It deals with two semaphores because this thread touches two data structures (input and output buffers)
 */
void *encryptThread(void *vargp){
    while(1){
        //printf("encrypt 1: %d, 2: %d\n", render_count-get_input_total_count(), !output_buf.full);
        if(done[1] == 0) {
            sem_wait(sem_encrypt1);
        }
        //if(done[2] == 0) {
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

        if (output_buf.full == 1) {
            printf("make sem wait out \n");
            sem_wait(sem_encrypt2);
            printf("sneaky sneaky\n");
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
        if (done[0] == 0) {
            printf("func1 is done");
            sem_post(sem_char_render);
        }
        printf("post count out");
        sem_post(sem_count_out);

        // }

        if(done[1] == 1){//output_buf.max == output_buf.tail-1 || render_count == 12){//TAKE THIS OUT!!!
            printf("encrypt post DONE encrypt count = %d, input_total_count == %d, output_count = %d\n", encrypt_count, get_input_total_count(), get_output_total_count());
            //sem_post(sem_count_out);
            //done[2] = 1;
            if(encrypt_count >= get_input_total_count()){
                sem_post(sem_count_out);
                done[2] = 1;
                printf("END OF ENCRYPT\n");
                if(keyChanged == 1){
                    printf("\nWE RESET ENCRYPT HERE!!\n");

                    sem_wait(sem_encrypt1);
                    //done[2] = 0;
                }
                else{
                    return 0;
                }

            }
        }
    }

}


/*
 * Output Counter thread is the thread that counts the number of characters from the file that we have outputed.
 */
void *outputCounterThread(void *vargp){

    while(1){
        if(done[2] == 1){
            printf("sem_writer post here \n");
            sem_post(sem_writer);
            if(get_output_total_count() >= render_count){
                printf("OUTPUT DONE\n");
                done[3] = 1;
                if(keyChanged == 1){
                    printf("\nWE RESET OUTPUT HERE!!\n");
                    sem_wait(sem_count_out);
                    //sem_post(sem_writer);
                    //done[3] = 0;
                }
                else{
                    return 0;
                }

            }

        }
        else{
            sem_wait(sem_count_out);
        }
        //if(done[3] == 0) {
        printf("output %d\n", get_output_total_count());
        //if(writer_count < get_output_total_count()){ //maybe we dont need this if cause we use sems
        count_output(output_buf.buffer[getIndexOfCircBuf(output_buf, encrypt_count - get_output_total_count())]);
        sem_post(sem_writer);
        //sem_post(sem_encrypt2);
         //}

        //}
    }
}

/*
 * The writer thread is the thread that writes to the output file.
 */
void *writerThread(void *vargp){
    while(1) {
        printf("writer\n");
        if(done[3] == 0){
            printf("done3 is true\n");
            sem_wait(sem_writer);
        }

        //if (output_buf.head != output_buf.tail) {
        //if(done[4] == 0) {
        write_output(output_buf.buffer[output_buf.head]);
        //moveHead(output_buf);
        printf("whead: %d, tail %d\n", output_buf.head, output_buf.tail);
        if (output_buf.head != output_buf.tail) {
            printf("whead: %d, tail %d\n", output_buf.head, output_buf.tail);
            output_buf.head = (output_buf.head + 1) % output_buf.max;
            output_buf.full = (output_buf.head == output_buf.tail + 1);
        }

        writer_count++;
        printf("writer count %d \n", writer_count);
        sem_post(sem_encrypt2);
        // }
        //}
        printf("done1 %d, done2 %d, done3 %d\n", done[1], done[2], done[3]);
        if(done[3] == 1 && writer_count >= render_count){
            printf("DONE WITH IT ALL\n");
            if(keyChanged == 1){
                done[4] = 1;
                printf("\nWE RESET EVERYTHING!!!!\n");
                sem_post(sem_key_return);
                sem_wait(sem_writer);
                done[4] = 0;
            }
            else{
                return 0;
            }

        }
    }
}



/*
 * MAIN
 * In the main, we take in three file names in this format: [INPUT_FILENAME] [OUTPUT_FILENAME] [LOG_FILENAME]
 * The program quits if there are not three arguments after ./encrypt in the command line
 *
 * Then it initializes the files, and prompts the user for the size of the input and output buffer. Then we allocate
 * memory for the buffers and initialize everything at 0
 *
 * Next, we open our semaphores and unlink them, create our pthreads and join them.
 * These threads run until the end of file is hit.
 *
 * At the end of the main we log the counts to the [LOG_FILENAME]
 *
 */
int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Please re-run program with correct input: [INPUT_FILENAME] [OUTPUT_FILENAME] [LOG_FILENAME]");
        return 0;
    }
    init(argv[1], argv[2], argv[3]);
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
    sem_key = sem_open("/sem_key", O_CREAT, 0644, 0);
    sem_unlink("/sem_key");
    sem_key_return = sem_open("/sem_key_return", O_CREAT, 0644, 0);
    sem_unlink("/sem_key_return");
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
