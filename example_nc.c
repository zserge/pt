#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "pt.h"

typedef pt_queue(uint8_t, 4096) byte_queue_t;

static void stdin_loop(struct pt *pt, byte_queue_t *out) {
  static uint8_t b;
  static int r;
  pt_begin(pt);
  for (;;) {
    pt_sys(pt, r = read(STDIN_FILENO, &b, 1));
    if (r == 0) {
      pt_wait(pt, pt_queue_empty(out));
      pt_exit(pt, 1);
    }
    pt_wait(pt, !pt_queue_full(out));
    pt_queue_push(out, b);
  }
  pt_end(pt);
}

static void socket_write_loop(struct pt *pt, int fd, byte_queue_t *in) {
  static uint8_t *b;
  pt_begin(pt);
  for (;;) {
    pt_wait(pt, !pt_queue_empty(in));
    b = pt_queue_pop(in);
    pt_sys(pt, send(fd, b, 1, 0));
  }
  pt_end(pt);
}

static void socket_read_loop(struct pt *pt, int fd) {
  static uint8_t b;
  static int r;
  pt_begin(pt);
  for (;;) {
    pt_sys(pt, r = recv(fd, &b, 1, 0));
    if (r == 0) {
      pt_exit(pt, 1);
    }
    pt_sys(pt, write(STDOUT_FILENO, &b, 1));
  }
  pt_end(pt);
}

static int nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "USAGE: example_nc <ip> <port>\n");
    return 1;
  }

  char *host = argv[1];
  int port = atoi(argv[2]);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket()");
    return 1;
  }
  if (nonblock(fd) < 0) {
    perror("nonblock() socket");
    return 1;
  }
  if (nonblock(STDIN_FILENO) < 0) {
    perror("nonblock() stdin");
    return 1;
  }
  if (nonblock(STDOUT_FILENO) < 0) {
    perror("nonblock() stdout");
    return 1;
  }
  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr =
          {
              .s_addr = inet_addr(host),
          },
      .sin_port = htons(port),
  };
  connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

  struct pt pt_stdin = pt_init();
  struct pt pt_socket_read = pt_init();
  struct pt pt_socket_write = pt_init();
  byte_queue_t queue = pt_queue_init();

  while (pt_status(&pt_stdin) == 0 && pt_status(&pt_socket_read) == 0) {
    if (pt_queue_empty(&queue)) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      FD_SET(fd, &fds);
      select(fd + 1, &fds, NULL, NULL, NULL);
    }
    socket_read_loop(&pt_socket_read, fd);
    socket_write_loop(&pt_socket_write, fd, &queue);
    stdin_loop(&pt_stdin, &queue);
  }

  close(fd);
  return 0;
}

