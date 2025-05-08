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
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define MAX_FILENAME 256
#define LOG_PATH "server/server.log"
#define DB_PATH "server/database/"

void write_log(const char* source, const char* action, const char* info) {
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    
    FILE* log = fopen(LOG_PATH, "a");
    if (!log) return;
    
    fprintf(log, "[%s][%04d-%02d-%02d %02d:%02d:%02d]: [%s] [%s]\n",
            source, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, action, info);
    fclose(log);
}

int hex_to_bin(const char* hex, unsigned char* out, size_t hex_len) {
    if (hex_len % 2 != 0) return -1;
    
    for (size_t i = 0; i < hex_len; i += 2) {
        if (sscanf(hex + i, "%2hhx", &out[i/2]) != 1) return -1;
    }
    return hex_len / 2;
}

void reverse_string(char* str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len/2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

void ensure_directories() {
    mkdir("server", 0755);
    mkdir(DB_PATH, 0755);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';

    if (strncmp(buffer, "EXIT", 4) == 0) {
        write_log("Client", "EXIT", "Client requested to exit");
        close(client_fd);
        return;
    }

    if (strncmp(buffer, "DECRYPT:", 8) == 0) {
        char* hexdata = buffer + 8;
        
        char* filename_start = strstr(buffer, "FILENAME:");
        if (filename_start) {
            filename_start += 9; // Lewati "FILENAME:"
            char* filename_end = strchr(filename_start, ':');
            if (filename_end) {
                *filename_end = '\0'; // Pisahkan nama file
                write_log("Client", "DECRYPT", filename_start);
                *filename_end = ':'; // Kembalikan ke semula
                hexdata = filename_end + 1; // Update hexdata pointer
            } else {
                write_log("Client", "DECRYPT", "unknown_file");
            }
        } else {
            write_log("Client", "DECRYPT", "unknown_file");
        }

        size_t hex_len = strlen(hexdata);
        if (hex_len % 2 != 0) {
            send(client_fd, "ERROR: Invalid hex length", 24, 0);
            write_log("Server", "ERROR", "Invalid hex length");
            close(client_fd);
            return;
        }

        reverse_string(hexdata);
        
        unsigned char* bindata = malloc(hex_len / 2);
        if (!bindata) {
            send(client_fd, "ERROR: Memory allocation failed", 30, 0);
            close(client_fd);
            return;
        }

        int bin_len = hex_to_bin(hexdata, bindata, hex_len);
        if (bin_len < 0) {
            send(client_fd, "ERROR: Hex decode failed", 24, 0);
            write_log("Server", "ERROR", "Hex decode failed");
            free(bindata);
            close(client_fd);
            return;
        }

        time_t now = time(NULL);
        char filename[MAX_FILENAME];
        snprintf(filename, sizeof(filename), "%ld.jpeg", now);

        char fullpath[MAX_FILENAME * 2];
        snprintf(fullpath, sizeof(fullpath), "%s%s", DB_PATH, filename);

        FILE* img = fopen(fullpath, "wb");
        if (!img) {
            send(client_fd, "ERROR: Failed to create image file", 31, 0);
            write_log("Server", "ERROR", "File creation failed");
            free(bindata);
            close(client_fd);
            return;
        }

        fwrite(bindata, 1, bin_len, img);
        fclose(img);
        free(bindata);

        write_log("Server", "SAVE", filename);
        send(client_fd, filename, strlen(filename), 0);

    } else if (strncmp(buffer, "DOWNLOAD:", 9) == 0) {
        char* filename = buffer + 9;
        write_log("Client", "DOWNLOAD", filename);

        char fullpath[MAX_FILENAME * 2];
        snprintf(fullpath, sizeof(fullpath), "%s%s", DB_PATH, filename);

        FILE* img = fopen(fullpath, "rb");
        if (!img) {
            send(client_fd, "ERROR: File not found", 21, 0);
            write_log("Server", "ERROR", "File not found");
            close(client_fd);
            return;
        }

        fseek(img, 0, SEEK_END);
        long size = ftell(img);
        rewind(img);

        if (size > BUFFER_SIZE) {
            send(client_fd, "ERROR: File too large", 21, 0);
            fclose(img);
            close(client_fd);
            return;
        }

        unsigned char* data = malloc(size);
        if (!data) {
            fclose(img);
            close(client_fd);
            return;
        }

        fread(data, 1, size, img);
        fclose(img);

        write_log("Server", "UPLOAD", filename);
        send(client_fd, data, size, 0);
        free(data);
    }

    close(client_fd);
}

int main() {
    ensure_directories();
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            write_log("Server", "ERROR", "Accept failed");
            continue;
        }
        handle_client(client_fd);
    }

    return 0;
}
