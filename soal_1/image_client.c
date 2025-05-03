// File: image_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>

#define PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 8192

// Cek apakah folder ada
int folder_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

// Jalankan perintah shell
void run_command(const char* cmd) {
    system(cmd);
}

// Download dan unzip secrets
void prepare_secrets() {
    if (!folder_exists("client/secrets")) {
        printf("[Client]: Downloading secrets from Google Drive...\n");
        run_command("mkdir -p client");
        run_command("cd client && wget --no-check-certificate 'https://drive.google.com/uc?export=download&id=15mnXpYUimVP1F5Df7qd_Ahbjor3o1cVw' -O secrets.zip");
        run_command("cd client && unzip -o secrets.zip && rm secrets.zip");
    }
}

// Kirim ke server dan terima balasan
int communicate_with_server(const char* message, unsigned char* response, int* response_size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("[Client]: ERROR creating socket.\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[Client]: ERROR connecting to server.\n");
        close(sock);
        return -1;
    }

    send(sock, message, strlen(message), 0);
    *response_size = recv(sock, response, BUFFER_SIZE, 0);
    close(sock);
    return 0;
}

// Balikkan string
void reverse_string(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len/2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

int main() {
    prepare_secrets();

    while (1) {
        printf("========================\n");
        printf("| Image Decoder Client |\n");
        printf("========================\n");
        printf("1. Send input file to server\n");
        printf("2. Download file from server\n");
        printf("3. Exit\n>> ");

        int choice;
        scanf("%d", &choice);
        getchar(); // flush newline

        if (choice == 1) {
            char filename[128];
            printf("Enter the file name: ");
            fgets(filename, sizeof(filename), stdin);
            filename[strcspn(filename, "\n")] = '\0';

            char filepath[256];
            snprintf(filepath, sizeof(filepath), "client/secrets/%s", filename);

            FILE* file = fopen(filepath, "r");
            if (!file) {
                printf("[Client]: ERROR file not found.\n");
                continue;
            }

            char* hexdata = malloc(BUFFER_SIZE);
            size_t read = fread(hexdata, 1, BUFFER_SIZE - 1, file);
            fclose(file);
            hexdata[read] = '\0';

            reverse_string(hexdata);

            char* request = malloc(strlen(hexdata) + 20);
            sprintf(request, "DECRYPT:%s", hexdata);

            unsigned char response[BUFFER_SIZE];
            int resp_size = 0;

            if (communicate_with_server(request, response, &resp_size) == 0) {
                response[resp_size] = '\0';
                printf("Server: Text decrypted and saved as %s\n", response);
            }

            free(hexdata);
            free(request);

        } else if (choice == 2) {
            char filename[128];
            printf("Enter the file name: ");
            fgets(filename, sizeof(filename), stdin);
            filename[strcspn(filename, "\n")] = '\0';

            char request[160];
            snprintf(request, sizeof(request), "DOWNLOAD:%s", filename);

            unsigned char response[BUFFER_SIZE];
            int resp_size = 0;

            if (communicate_with_server(request, response, &resp_size) == 0) {
                if (resp_size < 24 && strncmp((char*)response, "ERROR", 5) == 0) {
                    printf("[Client]: ERROR server response: %s\n", response);
                } else {
                    char save_path[256];
                    snprintf(save_path, sizeof(save_path), "client/%s", filename);

                    FILE* out = fopen(save_path, "wb");
                    if (out) {
                        fwrite(response, 1, resp_size, out);
                        fclose(out);
                        printf("Success! Image saved as %s\n", filename);
                    } else {
                        printf("[Client]: ERROR saving file.\n");
                    }
                }
            }

        } else if (choice == 3) {
            time_t now = time(NULL);
            struct tm* t = localtime(&now);
            printf("[Client][%04d-%02d-%02d %02d:%02d:%02d]: [EXIT] [Client requested to exit]\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
            break;
        } else {
            printf("[Client]: Invalid choice.\n");
        }
    }

    return 0;
}
