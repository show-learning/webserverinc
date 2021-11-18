// this web server only for get and post request
// note : don't put <!- in the form text since we will directly change html
// can't show the correct jpg file while pust jpg file
// stupid way to find content, but I have no time
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct sockaddr_in server_addr, client_addr;
  socklen_t sin_len = sizeof(client_addr);
  int fd_server, fd_client;
  char buf[524288];
  int fdimg;
  int on = 1;
  char webpage[4096];
  char temp[50];
  int len;
  char *ptr = webpage;
  int fp;
  fp = open("index.html", O_RDONLY);
  char html_content[2048];
  read(fp, html_content, sizeof(html_content));
  webpage[0] = '\0';
  strcat(ptr, "HTTP/1.1 200 OK\r\n");
  strcat(ptr, "Content-Type: text/html\r\n");
  sprintf(temp, "Content-Length: %ld\r\n", strlen(html_content));
  strcat(ptr, temp);
  strcat(ptr, "Connection: close\r\n\r\n");
  strcat(ptr, html_content);
  strcat(ptr, "\r\n");
  close(fp);
  pid_t pid;
  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_server < 0) {
    perror("socket");
    exit(1);
  }
  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);

  if (bind(fd_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind");
    close(fd_server);
    exit(1);
  }
  if (listen(fd_server, 10) == -1) {
    perror("listen");
    close(fd_server);
    exit(1);
  }

  while (1) {
    fd_client = accept(fd_server, (struct sockaddr *)&client_addr, &sin_len);
    if (fd_client == -1) {
      perror("Connection failed......\n");
      continue;
    }
    printf("Got client connection.......\n");
    // use double fork to avoid zombine process
    pid = fork();
    if (pid == 0) {
      /* child process */
      pid = fork();
      if (pid == 0) { /*grand child process*/
        close(fd_server);
        memset(buf, 0, 524287);
        read(fd_client, buf, 524287);
        //check buf
        printf("%s\n", buf);
        if (!strncmp(buf, "GET /", 5)) {
          char tmp[20];
          int count = 0;
          if (buf[5] != ' ') {
            while (buf[5 + count] != ' ') {
              tmp[count] = buf[5 + count];
              count++;
            }
            tmp[count] = '\0';
            printf("filename=%s\n", tmp);
            fdimg = open(tmp, O_RDONLY);
            sendfile(fd_client, fdimg, NULL, 1000000);
            close(fdimg);
          } else
            write(fd_client, webpage, sizeof(webpage) - 1);
        } else if (!strncmp(buf, "POST /index.html", 16)) {
          printf("Here is POST\n");
          char *change_string;
          char save_string[40] = {'\0'};
          char boundary[70] = {'\0'};
          char filename[40] = {'\0'};
          char upload_path[80] = "images/uploads/";
          char content_type[80] = {'\0'};
          char save_html[2048] = {'\0'};
          int cnt = 0;
          int content_length;
          //find boundary
          if (change_string = strstr(buf, "boundary=")) {
            change_string = change_string + 10;
            while (*change_string != '\0' && *change_string != '\r' &&
                   *change_string != '\n') {
              boundary[cnt] = *(change_string);
              change_string = change_string + 1;
              cnt++;
            }
            boundary[cnt] = '\0';
            printf("boundary is %s\n", boundary);
          }
          //find content_length
          if (change_string = strstr(buf, "Content-Length:")) {
            change_string = change_string + 16;
            content_length = atoi(change_string);
            printf("Content Length is %d\n", content_length);
          }
          cnt = 0;
          // change the string
          if (change_string = strstr(buf, "name=\"text\"")) {
            change_string = change_string + 15;
            while (*change_string != '\0' && *change_string != '\r' &&
                   *change_string != '\n') {
              save_string[cnt] = *(change_string);
              change_string = change_string + 1;
              cnt++;
            }
            save_string[cnt] = '\0';
            printf("text is %s\n", save_string);
            fp = open("index.html", O_WRONLY);

            read(fp, html_content, sizeof(html_content));
            if (change_string = strstr(html_content, "</h1>")) {
              strcpy(save_html, change_string);
              change_string = strstr(html_content, "<h1>");
              change_string = change_string + 4;
              *change_string = '\0';
              strcat(html_content, save_string);
              change_string = change_string + cnt;
              *change_string = '\0';
              strcat(html_content, save_html);

              cnt = strlen(html_content);
              write(fp, html_content, sizeof(char) * cnt);
              close(fp);
              memset(webpage, 0, 4096);
              strcat(ptr, "HTTP/1.1 200 OK\r\n");
              strcat(ptr, "Content-Type: text/html\r\n");
              sprintf(temp, "Content-Length: %ld\r\n", strlen(html_content));
              strcat(ptr, temp);
              strcat(ptr, "Connection: close\r\n\r\n");
              strcat(ptr, html_content);
              strcat(ptr, "\r\n");
              write(fd_client, webpage, sizeof(webpage) - 1);
            }
          }
          cnt = 0;
          //find content_type... weird place
          if (change_string = strstr(change_string, "Content-Type:")) {
            // it has value but dont work....
            printf("Find the content type!\n");
            change_string = change_string + 14;
            while (*change_string != '\0' && *change_string != '\r' &&
                   *change_string != '\n') {
              content_type[cnt] = *(change_string);
              change_string = change_string + 1;
              cnt++;
            }
            content_type[cnt] = '\0';
            printf("Content-Type is %s\n", content_type);
          }
          cnt = 0;
          //find the upload file
          if (change_string = strstr(buf, "filename=")) {
            change_string = change_string + 10;
            while (*change_string != '"') {
              filename[cnt] = *(change_string);
              change_string = change_string + 1;
              cnt++;
            }
            filename[cnt] = '\0';
            printf("filename is %s\n", filename);
            // move pointer to content start
            change_string = change_string + 5;
            while (*change_string != '\n') {
              change_string++;
            }
            strcat(upload_path, filename);
            printf("Upload to %s\n", upload_path);
            change_string = change_string + 3;
            // create and open file
            fp = open(upload_path, O_WRONLY | O_CREAT);
            // count the length of content by minus else size
            content_length = content_length - 216 - strlen(save_string) -
                             (strlen(boundary) * 4) - strlen(filename) -
                             strlen(content_type);
            printf("file size is %d\n", content_length);
            if (write(fp, change_string, content_length)) {
              printf("Upload Successful! Check uploads folder\n");
            } else
              printf("Upload fail\n");
            close(fp);
          }
        }
        close(fd_client);
        printf("closing....\n");
        exit(0);
      }
      close(fd_client);
    }
    /* parent process*/
    close(fd_client);
    waitpid(pid, NULL, 0);
  }
  return 0;
}