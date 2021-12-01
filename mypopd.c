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
#define OK 0
#define UNAUTHORIZED 1
#define WAITING_FOR_PASSWORD 2

static void handle_client(int fd);

int handle_requests(const char *request, int fd, int status);

char username_cache[MAX_LINE_LENGTH];
mail_list_t mail_list_cache;

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
  send_formatted(fd, "+OK I really hope that POP3 server ready\r\n");
  
  int status = UNAUTHORIZED;
  while(nb_read_line(nb, recvbuf) > 0) {
    status = handle_requests(recvbuf, fd, status);
		if (status = CLOSE) {
			nb_destroy(nb);
      status = UNAUTHORIZED;
		}
    
  }

}

int handle_requests(const char *request, int fd, int status) {
  
	// TODO: verify that the string ends with \r\n

	// The command will be the first sequence of letters separated by a space
	// OR, the comand with be followed with a CRLF
	const char space[] = " ";
	const char crlf[] = "\r\n";

	char *command;
	command = strtok(request, space); // extract command string from message
  // MAYBE DO SOME THING FOR CATCHING THE S P A C E

	if (strlen(command) > 4) {
		command = strtok(request, crlf);
	} 

  char *param = command + 5;

  // ---  AUTHORIZATION MODE ---

  if (strcasecmp(command, "QUIT") == 0) {
    send_formatted(fd, "+OK POP3 server signing off\r\n");
    return CLOSE;
  }

  if(status == UNAUTHORIZED) {
    if (strcasecmp(command, "USER") == 0) {
      if(is_valid_user(param, NULL)){         // Correct combination
        send_formatted(fd, "+OK welcome\r\n");
        strcmp(username_cache, param);
        return WAITING_FOR_PASSWORD;
      } else {      // ELSE
        send_formatted(fd, "-ERR You picked the wrong house\r\n");
        return UNAUTHORIZED;
      }
    } else {
      send_formatted(fd, "-ERR Please authorize first \r\n");
      return UNAUTHORIZED;
    }

  } else if(status == WAITING_FOR_PASSWORD) {
    if (strcasecmp(command, "PASS") == 0) {
      if(is_valid_user(username_cache, param)){         // Correct combination
        send_formatted(fd, "+OK welcome\r\n");
        mail_list_cache = locad_user_mail(username_cache);
        return OK;
      } else {      // ELSE
        send_formatted(fd, "-ERR Authorize failed. Please try again\r\n");
        return UNAUTHORIZED;
      }
    } else {
      send_formatted(fd, "-ERR Please authorize first \r\n");
      return UNAUTHORIZED;
    }
  }else {
  
  // ---  TRANSACTION MODE ---
    if(strcasecmp(command, "STAT") == 0) {
      send_formatted(fd, "+OK %d %d\r\n", get_mail_count(mail_list_cache), get_user_mail_size(mail_list_cache));
    } else if(strcasecmp(command, "LIST") == 0) {
      if (*param >= '0' || *param <= '9') {
        int index = atoi(param);
        if (index < get_mail_count(mail_list_cache)) {
          send_formatted(fd, "+OK %d %d\r\n", index, get_mail_size(mail_list_cache, index));
        }
      } else {
        send_formatted(fd, "+OK %d %d\r\n", get_mail_count(mail_list_cache), get_user_mail_size(mail_list_cache));
      }
    } else if(strcasecmp(command, "RETR") == 0) {
      // TODO
    } else if(strcasecmp(command, "DELE") == 0) {
      // TODO
    } else if(strcasecmp(command, "NOOP") == 0) {
      send_formatted(fd, "+OK\r\n");
    } else if(strcasecmp(command, "RSET") == 0) {
      // TODO
    } else {
      send_formatted(fd, "-ERR Unknown command\r\n");
    }


}