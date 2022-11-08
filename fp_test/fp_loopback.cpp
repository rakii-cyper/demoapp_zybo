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

#include<iostream>
#include<opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

void allwrite(int fd, unsigned char *buf, int len);  
void read_from_fifo(void* device_name_to_read);
void write_to_fifo(void* device_name_to_write);

int main(int argc, char *argv[]) {

}

void allwrite(int fd, unsigned char *buf, int len) {
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

void read_from_fifo(void* device_name_to_read) {
  int fd, rc;
  unsigned char buf[1228800];
  

  if (argc!=2) {
    fprintf(stderr, "Usage: %s devfile\n", argv[0]);
    exit(1);
  }
  
  fd = open(argv[1], O_RDONLY);
  
  if (fd < 0) {
    if (errno == ENODEV)
      fprintf(stderr, "(Maybe %s a write-only file?)\n", argv[1]);

    perror("Failed to open devfile");
    exit(1);
  }

  int cols = 0;
  int rows = 0;
  Mat output = Mat::zeros(Size(640, 480), CV_8UC1);
  while (cols < 640 && rows < 480) {
    int len = 0;
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
    cout << "Number of bytes read: " << rc << endl;
    while (len < rc) {
      output.at<uchar>(rows, cols) = (int)buf[len];
      /* Vec3b &intensity = output.at<Vec3b>(rows, cols);
      for(int k = 0; k < 1; k++) {
        intensity.val[k] = int(buf[len + k]);
      } */
      // cout << "reading [" << cols << ", " << rows << "]" << endl;
      rows++;
      len = len + 4;
      if (rows == 480) {
        rows = 0;
        cols++;

        if (cols == 640) break;
      }
    }
  imwrite("output.jpg", output);
  }
  imwrite("output.jpg", output);
  cout << "DONE!";
  exit(0);
}