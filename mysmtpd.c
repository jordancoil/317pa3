#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
    return 1;
  }

  run_server(argv[1], handle_client);

  return 0;
}

void parse_message(net_buffer_t nb, int fd) {
	char out[1024];
	nb_read_line(nb, out);
	send_formatted(fd, "Nice Message! (%s) \r\n", out);
}

void handle_client(int fd) {

  char recvbuf[MAX_LINE_LENGTH + 1];
  net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

  struct utsname my_uname;
  uname(&my_uname);

  /* TO BE COMPLETED BY THE STUDENT */
	send_formatted(fd, "Welcome\r\n");
	parse_message(nb, fd);

  nb_destroy(nb);
	send_formatted(fd, "Goodbye\r\n");
}
