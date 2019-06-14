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
	char *line;
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
				return;
			}
		}
		if (phase == 1) {
			if (strcmp(line, EOL) == 0) {
				phase = 2;
				break;
			}
		}
	}
	strcat(body, "<html><body>yay!</body></html>");
	add_header(header, "HTTP/1.0 200 OK");
	sprintf(s, "Content-Length: %lu", strlen(body));
	add_header(header, s);
	write(new_sockfd, header, strlen(header));
	write(new_sockfd, EOL, strlen(EOL));
	write(new_sockfd, body, strlen(body));
	close(new_sockfd);  /* ソケットを閉鎖 */
}

int main(int argc, char *argv[]) {
	int sockfd;
	int new_sockfd;
	unsigned int writer_len;
	struct sockaddr_in reader_addr; 
	struct sockaddr_in writer_addr;

	/* ソケットの生成 */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("reader: socket");
		exit(1);
	}

	/* 通信ポート・アドレスの設定 */
	bzero((char *) &reader_addr, sizeof(reader_addr));
	reader_addr.sin_family = PF_INET;
	reader_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	reader_addr.sin_port = htons(5000);

	/* ソケットにアドレスを結びつける */
	if (bind(sockfd, (struct sockaddr *)&reader_addr, sizeof(reader_addr)) < 0) {
		perror("reader: bind");
		exit(1);
	}

	/* コネクト要求をいくつまで待つかを設定 */
	if (listen(sockfd, 5) < 0) {
		perror("reader: listen");
		close(sockfd);
		exit(1);
	}

	while (1) {
		/* コネクト要求を待つ */
		if ((new_sockfd = accept(sockfd,(struct sockaddr *)&writer_addr, &writer_len)) < 0) {
			perror("reader: accept");
			exit(1);
		}
		simpe_server(new_sockfd);
	}
	close(sockfd);  /* ソケットを閉鎖 */
}