#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#define MEMORY_SIZE_A 81
#define MEMORY_ADDRESS_B 0x3CCCDEDE
#define GENERATOR_THREADS_NUM_D 17
#define FILE_SIZE_E 8
#define BLOCK_SIZE_G 2
#define BLOCK_SIZE_DEFAULT 4096
#define BLOCK_SIZE_DEFAULT_LAST 256

#define READER_THREADS_NUM_I 6

#define FILES_NUMBER 11

typedef struct {
    int id;
} my_thread_data_t;
typedef struct my_thread_t{
    my_thread_data_t data;
    pthread_t thread;    
} my_thread_t;


const int SIZE_PER_WRITER_THREAD = MEMORY_SIZE_A * 1024 * 1024 / GENERATOR_THREADS_NUM_D;  
const int SIZE_LAST_WRITER_THREAD = MEMORY_SIZE_A * 1024 * 1024 % GENERATOR_THREADS_NUM_D;  
const int SIZE_OF_LAST_FILE = MEMORY_SIZE_A % FILE_SIZE_E;  

FILE *devurandom_file;
int infinity_loop = 1; 
const char *devurandom_filename = "/dev/urandom";
my_thread_t *generators;
my_thread_t *readers;

const char *filenames[] = {"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10"};
pthread_mutex_t locks[FILES_NUMBER];
unsigned char *memory_actual_address;
int max = INT_MIN;

void init_generator_threads();
void init_reader_threads();
my_thread_t *init_threads(int threads_num);

void *write_to_memory_and_to_files_loop(void *);
void *write_to_memory(void *);
void *write_to_memory_and_to_files(void *thread_data);
void *read_files_loop(void *thread_data);

void start_threads(my_thread_t *writer_arr, int thread_num, void*(*f)(void *));
void stop_threads(my_thread_t *writer_arr, int thread_num);

void *write_to_file(void *);
void seq_write_to_file(int fd, int block_num, int offset);

void *read_file(void *thread_data);
char* seq_read_from_file(int fd, int file_size);

int calculate_max(int *numbers, int buffer_size);
void set_new_max(int new_max);


int main(int argc, char *argv[]){
    int do_start_loop;
    printf("pid - %ld\n", (long)getpid());

    printf("Before the allocation %d Kib of memory at the address %p. Pause", MEMORY_SIZE_A*1024, (void *) MEMORY_ADDRESS_B);
    getchar();
    memory_actual_address = mmap((void *) MEMORY_ADDRESS_B, MEMORY_SIZE_A*1024*1024,  PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("Data is allocated at the address %p. Pause", memory_actual_address);
    getchar();
    
    devurandom_file = fopen(devurandom_filename, "r");
    init_generator_threads();
    init_reader_threads();

    //fill memory at first time;
    puts("Write data to the memory. Wait");
    start_threads(generators, GENERATOR_THREADS_NUM_D, write_to_memory_and_to_files);
    stop_threads(generators, GENERATOR_THREADS_NUM_D);
    puts("The memory is filled. Reading...");
    start_threads(readers, READER_THREADS_NUM_I, read_file);
    stop_threads(readers, READER_THREADS_NUM_I);
    printf("Press enter to start deallocation");
    getchar();

    fclose(devurandom_file);
    free(generators);
    free(readers);
    munmap(memory_actual_address, MEMORY_SIZE_A * 1024 * 1024);
    printf("After deallocation. Pause");
    getchar();
    
    
    printf("Start a loop? (1 or not)\n");
    if(scanf("%d", &do_start_loop) == 1 && do_start_loop == 1){
        //start loop
        printf("The loop is started. Press enter to stop the loop");
        devurandom_file = fopen(devurandom_filename, "r");
        init_generator_threads();
        init_reader_threads();
        memory_actual_address = mmap((void *) MEMORY_ADDRESS_B, MEMORY_SIZE_A*1024*1024,  PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        start_threads(generators, GENERATOR_THREADS_NUM_D, write_to_memory_and_to_files_loop);
        start_threads(readers, READER_THREADS_NUM_I, read_files_loop);

        getchar();
        infinity_loop = 0;

        stop_threads(generators, GENERATOR_THREADS_NUM_D);
        stop_threads(readers, READER_THREADS_NUM_I);

        fclose(devurandom_file);
        free(generators);
        free(readers);
        munmap(memory_actual_address, MEMORY_SIZE_A * 1024 * 1024);
        getchar();
    }
    return 0;
}

void *read_files_loop(void *thread_data) {
    while (infinity_loop)
    {
        read_file(thread_data);
    }
    return NULL;
    
}

void *read_file(void *thread_data) {
    const my_thread_data_t *data = (my_thread_data_t *) thread_data;
    const int file_id = data->id % FILES_NUMBER;
    const int bytes_to_file = (file_id == FILES_NUMBER - 1 ? SIZE_OF_LAST_FILE : FILE_SIZE_E) * 1024 * 1024;
    int file;

    pthread_mutex_lock(&locks[file_id]);
    file = open(filenames[file_id], O_RDONLY, S_IRGRP|S_IWGRP|S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH);
    if (file == - 1) {
        printf ("Error no is : %d\n", errno);
        printf("[%d]Can't open %s for writing\n", data->id, filenames[file_id]);
        return NULL;
    }
        
    flock(file, LOCK_EX);
    int *int_buf = (int *) seq_read_from_file(file, bytes_to_file);
    flock(file, LOCK_UN);
    close(file);
    pthread_mutex_unlock(&locks[file_id]);
    
    set_new_max(calculate_max(int_buf, bytes_to_file / sizeof(int)));
    free(int_buf);
    return NULL;
}

char* seq_read_from_file(int fd, int file_size) {
    char *buffer = (char*) malloc(file_size);
    int offset;
    for (offset = 0; offset < file_size; offset += BLOCK_SIZE_G) {
        pread(fd,  buffer + offset, BLOCK_SIZE_G, offset);
    }
    return buffer;
}

int calculate_max(int *numbers, int buffer_size){
    int local_max = INT_MIN;
    for (int i = 0; i < buffer_size; i++) {
        local_max = numbers[i] < local_max ? local_max : numbers[i];
    }
    return local_max;
}

void set_new_max(int new_max){
    if(new_max > max){
        printf("New max value! (%d -> %d)\n", max, new_max);
        max = new_max;
    }
}

void *write_to_file(void * thread_data){
    my_thread_data_t *data = (my_thread_data_t *) thread_data;
    const int file_id = data->id % FILES_NUMBER;
    const int bytes_to_file = (file_id == FILES_NUMBER - 1 ? SIZE_OF_LAST_FILE : FILE_SIZE_E) * 1024 * 1024;
    const int block_num = bytes_to_file / BLOCK_SIZE_DEFAULT;
    int file;        

    pthread_mutex_lock(&locks[file_id]);
    file = open(filenames[file_id], O_WRONLY|O_CREAT|__O_DIRECT, S_IRGRP|S_IWGRP|S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH);
    //printf("[%d] write to f%d (%d). \n", data->id, file_id, file);

    if (file == - 1) {
        printf ("Error no is : %d\n", errno);
        printf("[%d]Can't open %s for writing\n", data->id, filenames[file_id]);
        return NULL;
    }

    flock(file, LOCK_EX);
    seq_write_to_file(file, block_num, FILE_SIZE_E * file_id * 1024 * 1024);
    flock(file, LOCK_UN);

    close(file);
    pthread_mutex_unlock(&locks[file_id]);  
    //printf("[%d] End write to f%d (%d). \n", data->id, file_id, file);
    return NULL;
}

void seq_write_to_file(int fd, int block_num, int offset){
    const int align = BLOCK_SIZE_DEFAULT-1;
    unsigned char *buff = malloc((int)BLOCK_SIZE_DEFAULT+align);
    unsigned char *wbuff = (unsigned char *)(((uintptr_t)buff+align)&~((uintptr_t)align));
    int i;
    for (i = 0; i < block_num; i++) {
        memcpy(wbuff, memory_actual_address + offset, BLOCK_SIZE_DEFAULT);
        if(pwrite(fd, wbuff, BLOCK_SIZE_DEFAULT, BLOCK_SIZE_DEFAULT * i) < 0 ){
            printf("Can't write to file(%d). Loop - %d/%d. Errno -  %d\n", fd, i + 1, block_num, errno);
            break;
        }
        offset += BLOCK_SIZE_DEFAULT;
    }
    free(buff);
}

void start_threads(my_thread_t *writer_arr, int thread_num, void*(*f)(void *)){
    int i;
    for (i = 0; i < thread_num; i++) {
        pthread_create(&(writer_arr[i].thread), NULL, f, &(writer_arr[i].data));
    }
}

void stop_threads(my_thread_t *writer_arr, int thread_num){
    int i;
    for (i = 0; i < thread_num; i++) {
        pthread_join(writer_arr[i].thread, NULL);
    }
 
}

void *write_to_memory(void *thread_data){
    my_thread_data_t *data = (my_thread_data_t *) thread_data;

    fread(memory_actual_address + data->id * SIZE_PER_WRITER_THREAD, 1, data->id == GENERATOR_THREADS_NUM_D - 1 ? SIZE_LAST_WRITER_THREAD : SIZE_PER_WRITER_THREAD, devurandom_file);

    return NULL;
}

void *write_to_memory_and_to_files(void *thread_data) {
    write_to_memory(thread_data);
    write_to_file(thread_data);
    return NULL;
}

void *write_to_memory_and_to_files_loop(void *thread_data) {
    while (infinity_loop){
        void *write_to_memory_and_to_files(void *thread_data);
    }
    
    return NULL;
}

void init_generator_threads(){
    generators = init_threads(GENERATOR_THREADS_NUM_D);
}

void init_reader_threads(){
    readers = init_threads(READER_THREADS_NUM_I);
}

my_thread_t *init_threads(int threads_num){
    my_thread_t *threads = malloc(threads_num * sizeof(my_thread_t));
    int i;
    for (i = 0; i < threads_num; i++) {
        threads[i].data.id = i;
    }
    return threads;
} 