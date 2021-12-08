#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

#define CONN_AWAITING_HELO_EHLO 1
#define CONN_OK 2
#define CONN_MAIL_PRODECURE_AWAITING_RCPT 3
#define CONN_MAIL_PRODECURE_AWAITING_RCPT_OR_DATA 4
#define CONN_MAIL_PRODECURE_AWAITING_MORE_DATA 5
#define CONN_CLOSE -1

static void handle_client(int fd);

char * get_reverse_path(char * buf);
char * get_forward_path(char * buf);

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
    return 1;
  }

  run_server(argv[1], handle_client);

  return 0;
}

void handle_client(int fd) {

  char recvbuf[MAX_LINE_LENGTH + 1];
  net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

  struct utsname my_uname;
  uname(&my_uname);
	char * domain = my_uname.nodename;

	send_formatted(fd, "220 Welcome\r\n");

	char filename[] = "mailitemXXXXXX";
	int tempfile = mkstemp(filename);

	user_list_t user_list = create_user_list();

	int status = CONN_AWAITING_HELO_EHLO; // See beginning of files to internal server status codes.
	while(nb_read_line(nb, recvbuf) > 0) {
		// The command will be the first sequence of letters separated by a space
		// OR, the comand with be followed with a CRLF
		const char space[] = " ";
		const char crlf[] = "\r\n";

		char *command = NULL;
		command = strdup(recvbuf);
		command = strtok(command, space); // extract command string from message

		if (strlen(command) > 4) {
			command = strtok(command, crlf);
		}

		if (strcasecmp(command, "HELO") == 0 || strcasecmp(command, "EHLO") == 0) {
			if (status != CONN_AWAITING_HELO_EHLO && status != CONN_OK) {
				send_formatted(fd, "503 bad sequence of commands (status: %d) \r\n", status);
				send_formatted(fd, "  status should be %d \r\n", CONN_AWAITING_HELO_EHLO);
			} else {
				char *client;
				client = strtok(strchr(recvbuf, ' '), crlf);

				if (client == NULL) {
					send_formatted(fd, "501 syntax error in parameters or arguments \r\n");
				} else {
					send_formatted(fd, "250 %s \r\n", domain);
					status = CONN_OK;
				}
			}
		} else if (strcasecmp(command, "MAIL") == 0) {
			if (status != CONN_OK) {
				send_formatted(fd, "503 bad sequence of commands \r\n");
			} else {
				char * from = get_reverse_path(recvbuf);

				if (from == NULL) {
					send_formatted(fd, "501 who is this from? \r\n");
				} else {
					status = CONN_MAIL_PRODECURE_AWAITING_RCPT;
					send_formatted(fd, "250 OK \r\n");
				}
			}

		} else if (strcasecmp(command, "RCPT") == 0) {
			if (status != CONN_MAIL_PRODECURE_AWAITING_RCPT && status != CONN_MAIL_PRODECURE_AWAITING_RCPT_OR_DATA) {
					send_formatted(fd, "503 bad sequence of commands \r\n");
			} else {
				char * rcpt = get_forward_path(recvbuf);

				if (rcpt == NULL) {
					send_formatted(fd, "501 syntax error in parameters or arguments \r\n");
				} else {
					if (is_valid_user(rcpt, NULL) == 1) {
						add_user_to_list(&user_list, rcpt);
						status = CONN_MAIL_PRODECURE_AWAITING_RCPT_OR_DATA;
						send_formatted(fd, "250 OK \r\n");
					} else {
						send_formatted(fd, "550 mailbox unavailable \r\n");
					}
				}
			}
		} else if (strcasecmp(command, "DATA") == 0 || status == CONN_MAIL_PRODECURE_AWAITING_MORE_DATA) {
			char *data;
			data = strdup(recvbuf);

			if (status == CONN_MAIL_PRODECURE_AWAITING_RCPT_OR_DATA) {
				send_formatted(fd, "354 start mail input; end with <CLRF>.<CLRF> \r\n");
				status = CONN_MAIL_PRODECURE_AWAITING_MORE_DATA;
				// if (strlen(data) > 5) {
				// 	char *body;
				// 	body = strchr(data, ' ');
				// 	body = strtok(body, " ");
				// 	write(tempfile, body, strlen(body));
				// }
			} else if (status == CONN_MAIL_PRODECURE_AWAITING_MORE_DATA) {

				// if (strcmp(data, ".\r\n") == 0 || strcmp(data, ".\n") == 0 ) {
				if (strcmp(data, ".\r\n") == 0) {
					status = CONN_OK;
					close(tempfile);
					save_user_mail(filename, user_list);
					send_formatted(fd, "250 OK \r\n");
				} else {
					write(tempfile, data, strlen(data));
				}
			} else {
				send_formatted(fd, "503 bad sequence of commands \r\n");
			}
		} else if (strcasecmp(command, "RSET") == 0) {
			if (status != CONN_AWAITING_HELO_EHLO) {
				status = CONN_OK;
			}
			close(tempfile);
			char template[] = "mailitemXXXXXX";
			tempfile = mkstemp(template);
			destroy_user_list(user_list);
			user_list = create_user_list();
			send_formatted(fd, "250 ok \r\n");
		} else if (strcasecmp(command, "VRFY") == 0) {
			char *param;
			param = strdup(recvbuf);
			param = strtok(strchr(param, ' '), crlf);

			if (param == NULL) {
				send_formatted(fd, "501 please provide a user \r\n");
			} else {
				*param = '\0';
				++param; // advance chracter to the username withrecvbuf a space at beginning.

				if (is_valid_user(param, NULL) == 1) {
					send_formatted(fd, "250 %s \r\n", param);
				} else {
					send_formatted(fd, "550 %s is not a valid user \r\n", param);
				}
			}
		} else if (strcasecmp(command, "NOOP") == 0) {
			send_formatted(fd, "250 OK\r\n");
		} else if (strcasecmp(command, "QUIT") == 0) {
			send_formatted(fd, "221 OK\r\n");
			status = CONN_CLOSE;
		} else {
			send_formatted(fd, "500 command \"%s\" is not recognized. \r\n", command);
			// TODO, send 502 for unsupported Command and 500 for unrecognized command
		}

		if (status == CONN_CLOSE) {
			nb_destroy(nb);
			close(fd);
			exit(0);
		}
	}
}


char * get_forward_path(char * buf) {
	const char tolc[] = "to:<";
	const char touc[] = "TO:<";
	const char lbracket[2] = "<";
	const char rbracket[2] = ">";
	char *result;

	result = strstr(buf, tolc);
	if (result == NULL) {
		result = strstr(buf, touc);
	}

	if (result == NULL) {
		return NULL;
	} else {
		if (strchr(result, '>') == NULL) {
			return NULL;
		}
		result = strstr(result, lbracket);  // "FROM:<str>" --> "<str>"
		result = strtok(result, lbracket);  // "<str>" --> "str>"
		result = strtok(result, rbracket); // str>" --> "str"
		return result;
	}
}

char * get_reverse_path(char * buf) {
	const char fromlc[] = "from:<";
	const char fromuc[] = "FROM:<";
	const char lbracket[2] = "<";
	const char rbracket[2] = ">";
	char *result;

	result = strstr(buf, fromlc);
	if (result == NULL) {
		result = strstr(buf, fromuc);
	}

	if (result == NULL) {
		return NULL;
	} else {
		if (strchr(result, '>') == NULL) {
			return NULL;
		}
		result = strstr(result, lbracket);  // "FROM:<str>" --> "<str>"
		result = strtok(result, lbracket);  // "<str>" --> "str>"
		result = strtok(result, rbracket); // str>" --> "str"
		return result;
	}
}
