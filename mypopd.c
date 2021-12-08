/*
QUICK REFERENCES:
https://datatracker.ietf.org/doc/html/rfc1939
https://ca.prairielearn.com/pl/course_instance/2347/instance_question/13604434/

TODO :

--- Functional ---

RETR
DELE
RSET

--- Bug fix ---

ACCOUNT HANDLER????
Do i need this for multi-thread? or global variables are good enough?
https://piazza.com/class/kt7rxa3xm7tud?cid=554

Handling the "cut in the middle" problem?

--- Other ---

Modualize?

*/

#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

#define CLOSE -1
#define TRANSACTION 0
#define AUTHORIZATION 1
#define WAITING_FOR_PASSWORD 2
#define UPDATE 3

static void handle_client(int fd);

// int handle_requests(char *request, int fd, int status);
void OKresp(int fd, char *msg);
void ERRresp(int fd, char *msg);
int check_param(char * param);

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

  /* TO BE COMPLETED BY THE STUDENT */
  send_formatted(fd, "+OK POP3 server ready\r\n");

	char* user = NULL;
	mail_list_t mail_list;

  int status = AUTHORIZATION;
  while(nb_read_line(nb, recvbuf) > 0) {
		const char space[] = " ";
		const char crlf[] = "\r\n";

		char *command;
		command = strdup(recvbuf);
		command = strtok(command, space); // extract command string from message
	  // MAYBE DO SOME THING FOR CATCHING THE S P A C E

		if (strlen(command) > 4) {
			command = strtok(command, crlf);
		}

		if (strcasecmp(command, "QUIT") == 0) {
			if(status == AUTHORIZATION) {
				OKresp(fd, "POP3 server signing off");
				status = CLOSE;
			} else if (status == TRANSACTION) {
				OKresp(fd, "POP3 server signing off (maildrop empty)");
				status = UPDATE;
			}
		} else if (strcasecmp(command, "USER") == 0) {
			if (status != AUTHORIZATION) {
				ERRresp(fd, "woah there!");
			} else {
				char *param = strdup(recvbuf);
				param = strtok(strchr(param, ' '), crlf);
				if (param == NULL) {
					ERRresp(fd, "invalid args");
				} else {
					param++;
					if (is_valid_user(param, NULL)) {
						OKresp(fd, "welcome");
						user = param;
					} else {
						ERRresp(fd, "unrecognized user");
					}
				}
			}
		} else if (strcasecmp(command, "PASS") == 0) {
			if (status != AUTHORIZATION) {
				ERRresp(fd, "woah there");
			} else if (user == NULL) {
				ERRresp(fd, "please specify a user first");
			} else {
				char *param = strdup(recvbuf);
				param = strtok(strchr(param, ' '), crlf);
				if (param == NULL) {
					ERRresp(fd, "invalid args");
				} else {
					param++;
					if (is_valid_user(user, param)) {
						mail_list = load_user_mail(user);
						OKresp(fd, "logged in");
						status = TRANSACTION;
					} else {
						ERRresp(fd, "login failed");
					}
				}
			}
		} else if (strcasecmp(command, "STAT") == 0) {
			if (status != TRANSACTION) {
				ERRresp(fd, "please login");
			} else {
				send_formatted(fd, "+OK %d %ld\r\n", get_mail_count(mail_list), get_mail_list_size(mail_list));
			}
		} else if (strcasecmp(command, "LIST") == 0) {
			if (status != TRANSACTION) {
				ERRresp(fd, "please login");
			} else {
				char *param = strdup(recvbuf);
				param = strtok(strchr(param, ' '), crlf);

				if (param == NULL) {
					// No parameter specified, try listing all mail items
					int mail_count = get_mail_count(mail_list);
					send_formatted(fd, "+OK %d messages (%ld octets)\r\n", mail_count, get_mail_list_size(mail_list));

					int count = 0;
					while (count <= mail_count) {
						mail_item_t item = get_mail_item(mail_list, count);
						if (item != NULL) {
							send_formatted(fd, "%d %ld\r\n", count+1, get_mail_item_size(item));
						}
						count++;
					}

					send_formatted(fd, ".\r\n");

				} else {
					// Parameter specified, try listing specific mail item
					// Fire, ensure parameter represents a digit
					if (check_param(param) == 0) {
						param++; // increment param to get rid of space before
						int index = atoi(param);
						mail_item_t item = get_mail_item(mail_list, index-1);
						if (item != NULL) {
							send_formatted(fd, "+OK %d %ld\r\n", index, get_mail_item_size(item));
						} else {
							ERRresp(fd, "no such message");
						}

					} else {
						ERRresp(fd, "invalid arguments");
					}
				}
			}
		} else if (strcasecmp(command, "RETR") == 0) {
			if (status != TRANSACTION) {
				ERRresp(fd, "please login");
			} else {
				char *param = strdup(recvbuf);
				param = strtok(strchr(param, ' '), crlf);
				if (param == NULL) {
					ERRresp(fd, "invalid arguments");
				} else {
					if (check_param(param) == -1) {
						ERRresp(fd, "invalid arguments");
					} else {
						param++;
						int index = atoi(param);
						mail_item_t item = get_mail_item(mail_list, index-1);
						if (item == NULL) {
							ERRresp(fd, "message deleted");
						} else {
							int size = get_mail_item_size(item);
							send_formatted(fd, "+OK %ld octets\r\n", get_mail_item_size(item));
							FILE * fp = get_mail_item_contents(item);
							char buffer[256];
							while (fread(buffer, 1, size, fp) != 0) {
								send_formatted(fd, "%s\r\n", buffer);
							}
							fclose(fp);
							send_formatted(fd, ".\r\n");
						}

					}
				}
			}
		} else if (strcasecmp(command, "DELE") == 0) {
			if (status != TRANSACTION) {
				ERRresp(fd, "please login");
			} else {
				char *param = strdup(recvbuf);
				param = strtok(strchr(param, ' '), crlf);
				if (param == NULL) {
					ERRresp(fd, "invalid arguments");
				} else {
					if (check_param(param) == -1) {
						ERRresp(fd, "invalid arguments");
					} else {
						param ++;
						int index = atoi(param);
						int count = get_mail_count(mail_list);
						if (index >= count) {
							ERRresp(fd, "no such message");
						} else {
							mail_item_t item = get_mail_item(mail_list, index-1);
							if (item == NULL) {
								send_formatted(fd, "-ERR message %d already deleted\r\n", index);
							} else {
								mark_mail_item_deleted(item);
								send_formatted(fd, "+OK message %d deleted\r\n", index);
							}
						}
					}
				}
			}
		} else if (strcasecmp(command, "RSET") == 0) {
			if (status != TRANSACTION) {
				ERRresp(fd, "please login");
			} else {
				int i = reset_mail_list_deleted_flag(mail_list);
				send_formatted(fd, "+OK %d messages restored\r\n", i);
			}

		} else if (strcasecmp(command, "NOOP") == 0) {
			OKresp(fd, "nooped");
		}

		if (status == UPDATE) {
			destroy_mail_list(mail_list);
			nb_destroy(nb);
			close(fd);
			exit(0);
		}

		if (status == CLOSE) {
			nb_destroy(nb);
			close(fd);
			exit(0);
		}
  }

}

	//
  // // ---  TRANSACTION MODE ---

  //   } else if(strcasecmp(command, "RETR") == 0) {
  //     // TODO
  //   } else if(strcasecmp(command, "DELE") == 0) {
  //     // TODO
  //   } else if(strcasecmp(command, "NOOP") == 0) {
  //     send_formatted(fd, "+OK\r\n");
  //   } else if(strcasecmp(command, "RSET") == 0) {
  //     // TODO
  //   } else {
  //     send_formatted(fd, "-ERR Unknown command\r\n");
  //   }
	// 	return OK;
  // }
  // else {
  //   send_formatted(fd, "-ERR Wrong status, please start the connection again. \r\n");
  //   return CLOSE;
  // }

void OKresp(int fd, char *msg) {
	send_formatted(fd, "+OK %s \r\n", msg);
}

void ERRresp(int fd, char *msg) {
	send_formatted(fd, "-ERR %s \r\n", msg);
}

int check_param(char * param) {
	int count = 1;
	char curr = param[count];
	while(count < strlen(param)) {
		if (!isdigit(curr)) {
			return -1;
		}
		count++;
		curr = param[count];
	}

	return 0;
}
