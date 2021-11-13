#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "80"
#define BUFFERSIZE 100000000
#define FILENAME_LEN 30

int createSocket() {
    const char* host = 0;
    const char* port = PORT;
    struct addrinfo hints, *bind_address;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;    // Use my IP

    getaddrinfo(host, port, &hints, &bind_address);

    // Create socket
    printf("Creating socket...\n");
    int listen_fd;
    if ((listen_fd = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol)) < 0) {
        perror("socket() failed.\n");
        exit(1);
    }

    // Bind
    printf("Binding socket to local address...\n");
    if (bind(listen_fd, bind_address->ai_addr, bind_address->ai_addrlen) < 0) {
        perror("bind() failed.\n");
        exit(1);
    }
    freeaddrinfo(bind_address);

    // Listen
    printf("Listening...\n");
    if(listen(listen_fd, 5) < 0) {
        perror("listen() failed.\n");
        exit(1);
    }

    return listen_fd;
}

const char *get_content_type(const char *path) {
    const char *extention = strrchr(path, '.');
    if (extention) {
        if (strcmp(extention, ".css") == 0) return "text/css";
        if (strcmp(extention, ".csv") == 0) return "text/csv";
        if (strcmp(extention, ".gif") == 0) return "image/gif";
        if (strcmp(extention, ".htm") == 0) return "text/html";
        if (strcmp(extention, ".html") == 0) return "text/html";
        if (strcmp(extention, ".ico") == 0) return "image/x-icon";
        if (strcmp(extention, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(extention, ".jpg") == 0) return "image/jpeg";
        if (strcmp(extention, ".js") == 0) return "application/javascript";
        if (strcmp(extention, ".json") == 0) return "application/json";
        if (strcmp(extention, ".png") == 0) return "image/png";
        if (strcmp(extention, ".pdf") == 0) return "application/pdf";
        if (strcmp(extention, ".svg") == 0) return "image/svg+xml";
        if (strcmp(extention, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}

void serveStaticPage(int fd, const char *path) {
    if (!strcmp(path, "/"))
        path = "/index.html";
    
    char full_path[128];
    sprintf(full_path, "src%s", path);
    
    FILE* fp = fopen(full_path, "rb");

    if(!fp) {
        //printf("send_404\n");
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);

    const char *ct = get_content_type(path);

    char buffer[1024];
    sprintf(buffer, "HTTP/1.1 200 OK\r\n"\
                    "Connection: close\r\n"\
                    "Content-Length: %lu\r\n"\
                    "Content-Type: %s\r\n"\
                    "\r\n", cl, ct);
    send(fd, buffer, strlen(buffer), 0);

    // Read file and send to client
    int r = fread(buffer, 1, 1024, fp);
    while (r) {
        send(fd, buffer, r, 0);
        r = fread(buffer, 1, 1024, fp);
    }

    fclose(fp);

    return;
}

void serveUploadFile(int fd, const char *filename, const char *content) {
    FILE *fp = fopen(filename, "wb");
    fprintf(fp, "%s", content);
    fclose(fp);
    chmod(filename, 0777);
    printf("Upload successful.\n");
    serveStaticPage(fd, "/");

    return;
}

void handleSocket(int fd) {
    char *request = (char*)malloc(sizeof(char)*(BUFFERSIZE+1));
    char *method = (char*)malloc(sizeof(char)*(BUFFERSIZE+1));
    
    int r = read(fd, request, BUFFERSIZE);   // Read request from browser
    if (r < 1) {
        perror("Unexpected disconnect.\n");
        exit(1);
    }
    strcpy(method, request);

    // String processing
    if (r>0 && r<BUFFERSIZE)
        method[r] = 0;
    else
        method[0] = 0;

    for (int i=0; i<r; i++)
        if (method[i]=='\r' || method[i]=='\n')
            method[i] = 0;

    if (!strncmp("GET /", method, 5)) {
        // Get request url
        char *path = method + 4;
        char *end_path = strstr(path, " ");
        if (!end_path) 
            printf("send_400\n");
        else {
            *end_path = 0;
            serveStaticPage(fd, path);
        }
    }
    else if (!strncmp("POST /", method, 6)) {
        // Get file name
        char *ptr = strstr(request, "filename=\"");
        ptr += 10;
        char filename[FILENAME_LEN+1];
        for (int i=0; *ptr != '\"'; i++)
        	filename[i] = *ptr++;
	
        // Get file content
        char *content = strstr(request, "Origin: ");
        content += 8;
        content = strstr(content, "Content-Type: ");
        content += 14;
        content = strstr(content, "\n");
        while(*content=='\r' || *content=='\n')   content++;
        char c[50000];
        int i = 0;
        while (!(*content=='\n' && *(content+1)=='-' && *(content+2)=='-'&&*(content+3)=='-')) {
        	c[i] = *content;
        	i++, content++;
        }
        c[i-1] = '\0';
        strcpy(content, &content[1]);
        
        FILE *fp = fopen(filename, "wb");
	fprintf(fp, "%s", c);
	fclose(fp);
	chmod(filename, 0777);
	printf("Upload successful.\n");
	serveStaticPage(fd, "/");
        
        //char *end_content = strstr(content, "---");
        //*end_content = 0;
        
        //serveUploadFile(fd, filename, c);
    }
    else 
        printf("send_400\n");

    return;
}

int main() {
    int listen_fd, conn_fd;
    struct sockaddr_storage address;
    socklen_t addrlen;
    pid_t pid;

    listen_fd = createSocket();

    signal(SIGCHLD, SIG_IGN);   // Ignore SIGCHLD to prevent zombie

    printf("Waiting for connections...\n");

    // Accept conncetions
    while(1) {
        addrlen = sizeof(address);
        if ((conn_fd = accept(listen_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept() failed.\n");
            return 1;
        }

        // Get client address
        //char address_buf[100];
        //getnameinfo((struct sockaddr *)&address, addrlen, address_buf, sizeof(address_buf), 0, 0, NI_NUMERICHOST);

        //printf("New connection from %s.\n", address_buf);

        pid = fork();
        if (pid == 0) {  // In child
            close(listen_fd);
            handleSocket(conn_fd);
            close(conn_fd);
            exit(0);
        }
        else if (pid < 0)
            exit(1);
        else    // In parent
            close(conn_fd);
    }

    return 0;
}
