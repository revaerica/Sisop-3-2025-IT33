#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <zip.h>

#define PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 8192
#define MAX_FILENAME 256

void ensure_directory(const char *path);
int download_file(const char *url, const char *output_filename);
int extract_zip(const char *zip_filename, const char *extract_dir);
void download_and_unzip();
void print_menu();
int send_request(const char* request, char* response, size_t resp_size);
void handle_decrypt();
void handle_download();

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

void ensure_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0755);
    }
}

int download_file(const char *url, const char *output_filename) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return -1;
    }
    
    fp = fopen(output_filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s\n", output_filename);
        curl_easy_cleanup(curl);
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);  // Disable progress meter
    
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK) ? 0 : -1;
}

int extract_zip(const char *zip_filename, const char *extract_dir) {
    int err = 0;
    struct zip *za = zip_open(zip_filename, 0, &err);
    if (!za) {
        fprintf(stderr, "Failed to open zip file\n");
        return -1;
    }
    
    int num_entries = zip_get_num_entries(za, 0);
    for (int i = 0; i < num_entries; i++) {
        const char *name = zip_get_name(za, i, 0);
        if (!name) continue;
        
        char output_path[MAX_FILENAME * 2];
        snprintf(output_path, sizeof(output_path), "%s/%s", extract_dir, name);
        
        char *last_slash = strrchr(output_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            ensure_directory(output_path);
            *last_slash = '/';
        }
        
        struct zip_file *zf = zip_fopen_index(za, i, 0);
        if (!zf) {
            fprintf(stderr, "Failed to open file %s in zip\n", name);
            continue;
        }
        
        FILE *out = fopen(output_path, "wb");
        if (!out) {
            fputs(output_path, stderr);
            zip_fclose(zf);
            continue;
        }
        
        char buf[BUFFER_SIZE];
        int bytes_read;
        while ((bytes_read = zip_fread(zf, buf, sizeof(buf)))) {
            fwrite(buf, 1, bytes_read, out);
        }
        
        fclose(out);
        zip_fclose(zf);
    }
    
    zip_close(za);
    return 0;
}

void download_and_unzip() {
    const char *gdrive_link = "https://drive.google.com/uc?export=download&id=15mnXpYUimVP1F5Df7qd_Ahbjor3o1cVw";
    const char *zip_name = "client/download.zip";
    const char *extract_dir = "client";

    ensure_directory("client");

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (download_file(gdrive_link, zip_name) != 0) {
        fprintf(stderr, "Failed to download zip file\n");
        curl_global_cleanup();
        return;
    }

    if (extract_zip(zip_name, extract_dir) != 0) {
        fprintf(stderr, "Failed to extract zip file\n");
        curl_global_cleanup();
        return;
    }

    if (remove(zip_name) != 0) {
        fprintf(stderr, "Failed to delete zip file: %s\n", strerror(errno));
    }

    curl_global_cleanup();
}


void print_menu() {
    printf("\n================================\n");
    printf("| Image Decoder Client |\n");
    printf("================================\n");
    printf("1. Send input file to server\n");
    printf("2. Download file from server\n");
    printf("3. Exit\n");
    printf(">> ");
}

int send_request(const char* request, char* response, size_t resp_size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Socket error: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        printf("Connection failed: %s\n", strerror(errno));
        close(sock);
        return -1;
    }

    send(sock, request, strlen(request), 0);
    ssize_t bytes = recv(sock, response, resp_size - 1, 0);
    close(sock);

    if (bytes <= 0) return -1;
    response[bytes] = '\0';
    return 0;
}

void handle_decrypt() {
    char filename[MAX_FILENAME];
    printf("Enter input filename: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';

    char path[MAX_FILENAME * 2];
    snprintf(path, sizeof(path), "client/secrets/%s", filename);

    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }

    char hexdata[BUFFER_SIZE];
    size_t bytes = fread(hexdata, 1, BUFFER_SIZE - 1, file);
    fclose(file);
    hexdata[bytes] = '\0';

    char request[BUFFER_SIZE];

    if (strlen(filename) + strlen(hexdata) + 20 > BUFFER_SIZE) {
    printf("Request too large to send to server.\n");
    return;
    }
    snprintf(request, sizeof(request), "DECRYPT:FILENAME:%s:", filename);
    strncat(request, hexdata, sizeof(request) - strlen(request) - 1);

    char response[BUFFER_SIZE];
    if (send_request(request, response, sizeof(response)) == 0) {
        if (strncmp(response, "ERROR:", 6) == 0) {
            printf("Server error: %s\n", response);
        } else {
            printf("Server: Text decrypted and saved as %s\n", response);
        }
    } else {
        printf("Communication error\n");
    }
}

void handle_download() {
    char filename[MAX_FILENAME];
    printf("Enter filename to download: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Socket error: %s\n", strerror(errno));
        return;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        printf("Connection failed: %s\n", strerror(errno));
        close(sock);
        return;
    }

    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "DOWNLOAD:%s", filename);
    send(sock, request, strlen(request), 0);

    unsigned char response[BUFFER_SIZE];
    ssize_t bytes = recv(sock, response, BUFFER_SIZE, 0);
    close(sock);

    if (bytes <= 0) {
        printf("Download failed\n");
        return;
    }

    if (strncmp((char*)response, "ERROR:", 6) == 0) {
        printf("%s\n", response);
        return;
    }

    char path[MAX_FILENAME * 2];
    snprintf(path, sizeof(path), "client/%s", filename);

    FILE* file = fopen(path, "wb");
    if (!file) {
        printf("Failed to create file\n");
        return;
    }

    fwrite(response, 1, bytes, file);
    fclose(file);

    printf("Success! Image saved as %s\n", filename);
}

void notify_exit_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        close(sock);
        return;
    }

    send(sock, "EXIT", 4, 0);
    close(sock);
}

int main() {
    ensure_directory("client");
    ensure_directory("client/secrets");
    
    download_and_unzip();

    while (1) {
        print_menu();
        
        int choice;
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input\n");
            continue;
        }
        getchar();

        switch (choice) {
            case 1: handle_decrypt(); break;
            case 2: handle_download(); break;
            case 3: 
                notify_exit_to_server();
                return 0;
            default: printf("Invalid choice\n");
        }
    }
}
