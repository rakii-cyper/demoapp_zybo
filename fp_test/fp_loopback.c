#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>

#define NUMBER_OF_FRAME 128

struct arg_struct {
    char *device_name_0;
    char *device_name_1;
    char *file_name_0;
    char *file_name_1;
};

void allwrite(int fd, float *buf, int len);
void *read_from_fifo(void *arg);
void *write_to_fifo(void *arg);

int main(int argc, char *argv[]) {
  pthread_t thread_id[2];
  struct arg_struct arg;

  if (argc!=6) {
    fprintf(stderr, "Usage: %s read_devfile write_devfile_0 write_devfile_1 input_file_0 input_file_1\n", argv[0]);
    exit(1);
  }

  arg.device_name_0 = argv[2];
  arg.device_name_1 = argv[3];
  arg.file_name_0 = argv[4];
  arg.file_name_1 = argv[5];
  // write_to_fifo((void *) &arg);
  if (pthread_create(&thread_id[0], NULL, read_from_fifo, (void *) argv[1])) {
    perror("Failed to create thread");
    exit(1);
  }

  if (pthread_create(&thread_id[1], NULL, write_to_fifo, (void *) &arg)) {
    perror("Failed to create thread");
    exit(1);
  }

  pthread_join(thread_id[0], NULL);
  pthread_join(thread_id[1], NULL);

  return -1;
}

void allwrite(int fd, float *buf, int len) {
  int sent = 0;
  int rc;

  while (sent < len) {
    rc = write(fd, buf + sent, len - sent);
	
    if ((rc < 0) && (errno == EINTR))
      continue;

    if (rc < 0) {
      perror("allwrite() failed to write");
      exit(1);
    }
	
    if (rc == 0) {
      fprintf(stderr, "Reached write EOF (?!)\n");
      exit(1);
    }
 
    sent += rc;
  }
} 

void *read_from_fifo(void* arg) {
  printf("READING!!!\n");
  char *device_name = (char *) (arg);
  int fd, rc;
  int counter = 0;
  int read_time_cnt = 1;
  float buf;
  
  fd = open(device_name, O_RDONLY);
  
  if (fd < 0) {
    if (errno == ENODEV)
      fprintf(stderr, "(Maybe %s a write-only file?)\n", device_name);

    perror("Failed to open read devfile");
    exit(1);
  }

  while (1) {
    rc = read(fd, buf, sizeof(buf));

    if (rc > 0) 
      printf("Value: %f \n", buf);
  }
  // while (read_time_cnt < 2) {
  //   rc = read(fd, buf, sizeof(buf));
  //   counter += (int) rc / 4;
  //   if (counter > NUMBER_OF_FRAME) {
  //     counter = (int) rc / 4;
  //     read_time_cnt++;
  //   }
  //   printf("Read %d bytes.\n", counter);
    
  //   if ((rc < 0) && (errno == EINTR))
  //     continue;
    
  //   if (rc < 0) {
  //     perror("allread() failed to read");
  //     exit(1);
  //   }
    
  //   if (rc == 0) {
  //     fprintf(stderr, "Reached read EOF.\n");
  //     exit(0);
  //   }
 
  //   // Write all data to standard output = file descriptor 1
  //   // rc contains the number of bytes that were read.
  //   for (int i=0; i < counter; i++){
  //     printf("Value: %f \n", buf[i]);
  //   }
  // }
  printf("DONE READING!!!\n");
  return NULL;
}

void *write_to_fifo(void* arg) {
  // initial variables for writing to fifo
  printf("WRITING!!!\n");
  struct arg_struct *arguments = (struct arg_struct *) arg;
  int fd0, fd1;
  float buf[NUMBER_OF_FRAME];

  // initial variables for reading input file
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read_cnt;
  int counter = 0;

  fp = fopen(arguments->file_name_0, "r");
  fd0 = open(arguments->device_name_0, O_WRONLY);
  
  if (fd0 < 0) {
    if (errno == ENODEV)
      fprintf(stderr, "(Maybe %s a read-only file?)\n", arguments->device_name_0);

    perror("Failed to open write devfile");
    exit(1);
  }

  if (fp == NULL) {
    perror("Failed to open input file");
    exit(1);
  }

  while ((read_cnt = getline(&line, &len, fp)) != -1) {
    if (read_cnt != 0) {
      line[strlen(line)-1] = '\0';
      buf[counter++] = atof(line);
    }
  }
  allwrite(fd0, buf, counter*4);
  fclose(fp);
  close(fd0);

  fd1 = open(arguments->device_name_1, O_WRONLY);
  fp = fopen(arguments->file_name_1, "r");
  counter = 0;
  len = 0;
  line = NULL;

  if (fd1 < 0) {
    if (errno == ENODEV)
      fprintf(stderr, "(Maybe %s a read-only file?)\n", arguments->device_name_1);

    perror("Failed to open write devfile");
    exit(1);
  }

  if (fp == NULL) {
    perror("Failed to open input file");
    exit(1);
  }

  while ((read_cnt = getline(&line, &len, fp)) != -1) {
    printf("RERUN\n");
    if (read_cnt != 0) {
      line[strlen(line)-1] = '\0';
      buf[counter++] = atof(line);
    }
  }
  allwrite(fd1, buf, counter*4);
  fclose(fp);
  close(fd1);
  if (line)
    free(line);

  printf("DONE WRITING!!!\n");
  return NULL;
}