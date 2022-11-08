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

#define NUMBER_OF_FRAME 128

struct arg_struct {
    char *device_name;
    char *file_name;
};

void allwrite(int fd, float *buf, int len);
void read_from_fifo(void *arg);
void write_to_fifo(void *arg);

int main(int argc, char *argv[]) {
  pthread_t thread_id[2];
  struct arg_struct arg;

  if (argc!=4) {
    fprintf(stderr, "Usage: %s read_devfile write_devfile input_file\n", argv[0]);
    exit(1);
  }

  arg.device_name = argv[2];
  arg.file_name = argv[3];
  write_to_fifo((void *) &arg);
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

void read_from_fifo(void* arg) {
  char *device_name = (char *) (arg);
  int fd, rc;
  float buf[NUMBER_OF_FRAME];
  
  fd = open(device_name, O_RDONLY);
  
  if (fd < 0) {
    if (errno == ENODEV)
      fprintf(stderr, "(Maybe %s a write-only file?)\n", device_name);

    perror("Failed to open devfile");
    exit(1);
  }

  while (1) {
    rc = read(fd, buf, sizeof(buf));
    
    if ((rc < 0) && (errno == EINTR))
      continue;
    
    if (rc < 0) {
      perror("allread() failed to read");
      exit(1);
    }
    
    if (rc == 0) {
      fprintf(stderr, "Reached read EOF.\n");
      exit(0);
    }
 
    // Write all data to standard output = file descriptor 1
    // rc contains the number of bytes that were read.

    allwrite(1, buf, rc);
  }
}

void write_to_fifo(void* arg) {
  // initial variables for writing to fifo
  struct arg_struct *arguments = (struct arg_struct *) arg;
  int fd, rc;
  float buf[NUMBER_OF_FRAME];

  // initial variables for reading input file
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read_cnt;
  int counter = 0;

  fp = fopen(arguments->file_name, "r");
  fd = open(arguments->device_name, O_WRONLY);
  
  if (fd < 0) {
    if (errno == ENODEV)
      fprintf(stderr, "(Maybe %s a read-only file?)\n", arguments->device_name);

    perror("Failed to open devfile");
    exit(1);
  }

  if (fp == NULL) {
    perror("Failed to open input file");
    exit(1);
  }

  while ((read_cnt = getline(&line, &len, fp)) != -1) {
    // Read from standard input = file descriptor 0
    if (read_cnt != 0) {
      line[strlen(line)-1] = '\0';
      buf[counter++] = atof(line);
      print("%f\n", buf[counter-1])
    }
    // printf("Retrieved line of length %zu:\n", read_cnt);
    // printf("%s", line);
    // allwrite(fd, buf, rc);
  }
  fclose(fp);
  if (line)
    free(line);
}