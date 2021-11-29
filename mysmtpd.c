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

void parse_message(char * out, int fd) {
	const char space[] = " ";
	char *command;
	command = strtok(out, space); // extract command string from message

	if (strcasecmp(command, "HELO") == 0) {
		send_formatted(fd, "HELO to you too! \r\n");
	} else if (strcasecmp(command, "EHLO") == 0) {
		send_formatted(fd, "EHLO command received! \r\n");
	} else if (strcasecmp(command, "MAIL") == 0) {
		send_formatted(fd, "MAIL command received! \r\n");
	} else if (strcasecmp(command, "RCPT") == 0) {
		send_formatted(fd, "RCPT command received! \r\n");
	} else if (strcasecmp(command, "DATA") == 0) {
		send_formatted(fd, "DATA command received! \r\n");
	} else if (strcasecmp(command, "RSET") == 0) {
		send_formatted(fd, "RSET command received! \r\n");
	} else if (strcasecmp(command, "VRFY") == 0) {
		send_formatted(fd, "VRFY command received! \r\n");
	} else if (strcasecmp(command, "NOOP") == 0) {
		send_formatted(fd, "NOOP command received! \r\n");
	} else if (strcasecmp(command, "QUIT") == 0) {
		send_formatted(fd, "QUIT command received! \r\n");
	} else {
		send_formatted(fd, "Command \"%s\" is not recognized. \r\n", command);
	}



}

void handle_client(int fd) {

  char recvbuf[MAX_LINE_LENGTH + 1];
  net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

  struct utsname my_uname;
  uname(&my_uname);

  /* TO BE COMPLETED BY THE STUDENT */
	send_formatted(fd, "Welcome\r\n");
	char out[1024];
	while(nb_read_line(nb, out) > 0) {
		parse_message(out, fd);
	}
  nb_destroy(nb);
	send_formatted(fd, "Goodbye\r\n");
}
