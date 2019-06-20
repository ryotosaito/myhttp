#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

char *EOL = "\r\n";

volatile sig_atomic_t e_flag = 0;

void handler(int signum) {
	e_flag = 1;
}

void add_header(char *buf, const char *str) {
	strcat(buf, str);
	strcat(buf, EOL);
}

void render(int status, int sockfd) {
	char message[64];
	char line[4096];
	char header[1024];
	char body[1024];
	char s[64];
	switch (status) {
		case 400:
			strcpy(message, "Bad request");
			break;
	}
	// body
	sprintf(body, "<html><head><title>%s</title></head><body><h1><center>%d %s<center></h1><hr><center>myhttp</center</body></html>",
			message, status, message);
	// header
	sprintf(s, "HTTP/1.0 %d %s", status, message);
	add_header(header, s);
	sprintf(s, "Content-Length: %lu", strlen(body));
	add_header(header, s);
	// send
	write(sockfd, header, strlen(header));
	write(sockfd, EOL, strlen(EOL));
	write(sockfd, body, strlen(body));
	close(sockfd);
}

void simpe_server(int new_sockfd) {
	char *line = NULL;
	char header[1024];
	char body[1024];
	char method[10], path[256], version[10];
	char s[64];
	size_t buf_len, read;
	header[0] = '\0';
	body[0] = '\0';
	int phase = 0;
	FILE *sockfile = fdopen(new_sockfd, "r");
	while((read = getline(&line, &buf_len, sockfile)) > 0) {
		if (phase == 0) {
			phase = 1;
			if (sscanf(line, "%s %s HTTP%s", method, path, version) != 3) {
				render(400, new_sockfd);
				free(line);
				return;
			}
		}
		if (phase == 1) {
			if (strcmp(line, EOL) == 0) {
				phase = 2;
				break;
			}
		}
		line = NULL;
	}
	strcat(body, "<html><body>yay!</body></html>");
	add_header(header, "HTTP/1.0 200 OK");
	sprintf(s, "Content-Length: %lu", strlen(body));
	add_header(header, s);
	write(new_sockfd, header, strlen(header));
	write(new_sockfd, EOL, strlen(EOL));
	write(new_sockfd, body, strlen(body));
	close(new_sockfd);  /* ソケットを閉鎖 */
	free(line);
	return;
}

int main(int argc, char *argv[]) {
	int sockfd;
	int new_sockfd;
	unsigned int writer_len;
	struct sockaddr_in reader_addr; 
	struct sockaddr_in writer_addr;
	int yes = 1;

	if (signal(SIGINT, handler) == SIG_ERR) {
		/* エラー処理 */
		fprintf(stderr, "SIG_ERR\n");
		exit(EXIT_FAILURE);
	}


	/* ソケットの生成 */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("reader: socket");
		exit(EXIT_FAILURE);
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

	/* 通信ポート・アドレスの設定 */
	bzero((char *) &reader_addr, sizeof(reader_addr));
	reader_addr.sin_family = PF_INET;
	reader_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	reader_addr.sin_port = htons(5000);

	/* ソケットにアドレスを結びつける */
	if (bind(sockfd, (struct sockaddr *)&reader_addr, sizeof(reader_addr)) < 0) {
		perror("reader: bind");
		exit(EXIT_FAILURE);
	}

	/* コネクト要求をいくつまで待つかを設定 */
	if (listen(sockfd, 5) < 0) {
		perror("reader: listen");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	while (!e_flag) {
		/* コネクト要求を待つ */
		if ((new_sockfd = accept(sockfd,(struct sockaddr *)&writer_addr, &writer_len)) < 0) {
			perror("reader: accept");
			exit(EXIT_FAILURE);
		}
		simpe_server(new_sockfd);
	}
	close(sockfd);  /* ソケットを閉鎖 */
	return EXIT_SUCCESS;
}
