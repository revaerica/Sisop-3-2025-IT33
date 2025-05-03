// File: image_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define LOG_PATH "server/server.log"
#define DB_PATH "server/database/"

// Logging helper
void write_log(const char* source, const char* action, const char* info) {
    FILE* log = fopen(LOG_PATH, "a");
    if (!log) return;
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    fprintf(log, "[%s][%04d-%02d-%02d %02d:%02d:%02d]: [%s] [%s]\n",
        source, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec, action, info);
    fclose(log);
}

// Hex decode
int hex_to_bin(const char* hex, unsigned char* out) {
    int len = strlen(hex);
    for (int i = 0; i < len; i += 2) {
        if (sscanf(hex + i, "%2hhx", &out[i / 2]) != 1) return -1;
    }
    return len / 2;
}

// Reverse string in place
void reverse_string(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len/2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

// Daemonize
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Parent exits
    umask(0);
    setsid();
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// Ensure directories exist
void ensure_directories() {
    mkdir("server", 0755);
    mkdir(DB_PATH, 0755);
}

int main() {
    ensure_directories();
    daemonize();

    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) exit(EXIT_FAILURE);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 5) < 0) exit(EXIT_FAILURE);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        memset(buffer, 0, BUFFER_SIZE);
        recv(client_fd, buffer, BUFFER_SIZE, 0);

        if (strncmp(buffer, "DECRYPT:", 8) == 0) {
            write_log("Client", "DECRYPT", "Text data");

            char* hexdata = buffer + 8;
            reverse_string(hexdata);

            int hex_len = strlen(hexdata);
            unsigned char* bindata = malloc(hex_len / 2);
            int bin_len = hex_to_bin(hexdata, bindata);

            if (bin_len < 0) {
                send(client_fd, "ERROR: Failed to decode hex.\n", 30, 0);
                free(bindata);
                close(client_fd);
                continue;
            }

            time_t now = time(NULL);
            char filename[64];
            snprintf(filename, sizeof(filename), "%ld.jpeg", now);

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s", DB_PATH, filename);

            FILE* img = fopen(fullpath, "wb");
            if (!img) {
                send(client_fd, "ERROR: Failed to save image.\n", 30, 0);
                free(bindata);
                close(client_fd);
                continue;
            }

            fwrite(bindata, 1, bin_len, img);
            fclose(img);

            write_log("Server", "SAVE", filename);
            send(client_fd, filename, strlen(filename), 0);
            free(bindata);

        } else if (strncmp(buffer, "DOWNLOAD:", 9) == 0) {
            char* filename = buffer + 9;
            write_log("Client", "DOWNLOAD", filename);

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s", DB_PATH, filename);
            FILE* img = fopen(fullpath, "rb");
            if (!img) {
                send(client_fd, "ERROR: File not found.\n", 24, 0);
                write_log("Server", "ERROR", "File not found");
                close(client_fd);
                continue;
            }

            fseek(img, 0, SEEK_END);
            long size = ftell(img);
            rewind(img);

            unsigned char* data = malloc(size);
            fread(data, 1, size, img);
            fclose(img);

            write_log("Server", "UPLOAD", filename);
            send(client_fd, data, size, 0);
            free(data);
        }

        close(client_fd);
    }

    return 0;
}
