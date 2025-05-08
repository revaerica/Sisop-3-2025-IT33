# Sisop-3-2025-IT33

#### Nama Anggota
1. Revalina Erica Permatasari (5027241007)
2. Kaisar Hanif Pratama (5027241029)
3. Ni'mah Fauziyyah Atok (5027241103)

## Daftar Isi
1. [Soal 1](#soal-1)
2. [Soal 2](#soal-2)
3. [Soal3](#soal-3)
4. [Soal 4](#soal-4)

## Soal 1
### Oleh: Ni'mah Fauziyyah A (5027241103)

### Soal
Di tahun 2045, dunia mengalami kekacauan siber. Seorang mahasiswa Departemen Teknologi Informasi ITS memiliki misi kembali ke tahun 2025 untuk memanggil hacker legendaris “rootkids”. Petunjuk yang ditemukan dari deep web berupa file teks aneh yang harus didekripsi dan diubah menjadi file JPEG menggunakan sistem RPC berbasis server-client. Sistem ini harus: <br>
- Men-decrypt file teks (reverse text + decode hex). <br>
- Menyimpan hasilnya dalam folder server dengan nama file berupa timestamp. <br>
- Mengirim kembali file hasil ke client menggunakan RPC. <br>
- Menampilkan menu interaktif pada client. <br>
- Menjaga agar server tetap berjalan di background (daemon). <br>
- Menangani semua error dengan baik dan mencatat log aktivitas. <br>

### Jawaban
#### File terkait:
`image_server.c` -> Program server daemon <br>
`image_client.c` -> Program client interaktif <br>

#### A. Download Starter Kit
Text file rahasia terdapat pada [LINK BERIKUT] (https://drive.google.com/file/d/15mnXpYUimVP1F5Df7qd_Ahbjor3o1cVw/view)
##### Kode
```
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
```
##### Penjelasan
- Download dan unzip link dari google drive yang berisi file txt ke dalam folder `client`

---

#### B. Implementasi Client-Server menggunakan Socket TCP
- Menggunakan socket TCP (AF_INET, SOCK_STREAM) untuk komunikasi antara server dan client.
- Client dan server terhubung melalui IP 127.0.0.1 dan port 8080.
```
Penggunaan: ./server/image_server 
```

##### Kode
Client (image_client.c) <br>
```
sock = socket(AF_INET, SOCK_STREAM, 0);
server.sin_family = AF_INET;
server.sin_port = htons(PORT);
inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
connect(sock, (struct sockaddr *)&server, sizeof(server));
```
Server (image_server.c) <br>
```
server_fd = socket(AF_INET, SOCK_STREAM, 0);
bind(server_fd, (struct sockaddr *)&address, sizeof(address));
listen(server_fd, 3);
new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
```
##### Penjelasan
- Server membuat socket dan menunggu koneksi dari client (`listen` + `accept`).
- Client membuat socket, lalu melakukan `connect` ke server.

---

#### C. Proses Dekripsi Teks
- File teks terenkripsi dari file txt yang sudah ter-unzip dan disusun dengan reverse string dan kemudian di-encode dalam format hexadecimal.
- Server harus membalikkan string lalu mengubah hex menjadi karakter ASCII.

##### Kode
```
char* reverse_string(const char* str) {
    int len = strlen(str);
    char* rev = malloc(len + 1);
    for (int i = 0; i < len; i++)
        rev[i] = str[len - i - 1];
    rev[len] = '\0';
    return rev;
}

char* hex_to_ascii(const char* hex_str) {
    size_t len = strlen(hex_str);
    char* result = malloc(len / 2 + 1);
    for (int i = 0; i < len; i += 2) {
        sscanf(hex_str + i, "%2hhx", &result[i / 2]);
    }
    result[len / 2] = '\0';
    return result;
}
```

##### Penjelasan
- Fungsi `reverse_string()` membalik isi string.
- Fungsi `hex_to_ascii()` mengkonversi tiap dua digit hex menjadi satu karakter ASCII.
---

#### D. Penyimpanan Hasil Decrypt sebagai File JPEG
- File hasil dekripsi disimpan dalam folder `server/database` dengan nama berupa timestamp (unik).
- Format file adalah `.jpeg.`

##### Kode
```
time_t timestamp = time(NULL);
snprintf(path, sizeof(path), "server/database/%ld.jpeg", timestamp);
FILE* img = fopen(path, "w");
fwrite(decoded, 1, strlen(decoded), img);
```
##### Penjelasan
- `timestamp` digunakan sebagai nama file agar unik dan mencatat waktu file disimpan.
- File disimpan dalam folder database server.
---

#### E. Mengirim Kembali File ke Client
- Client dapat memilih opsi “Download file from server” untuk meminta file berdasarkan nama file.
- Server akan membuka file .jpeg dan mengirimkan isinya melalui socket.

##### Kode
Client (image_client.c)
```
send(sock, "DOWNLOAD", sizeof("DOWNLOAD"), 0);
send(sock, filename, sizeof(filename), 0);
recv(sock, buffer, sizeof(buffer), 0);
```
Server (image_server.c)
```
if (strcmp(cmd, "DOWNLOAD") == 0) {
    recv(new_socket, filename, sizeof(filename), 0);
    FILE* f = fopen(path, "r");
    fread(buffer, sizeof(char), sizeof(buffer), f);
    send(new_socket, buffer, strlen(buffer), 0);
}
```

##### Penjelasan
- Client mengirim perintah DOWNLOAD dan nama file ke server.
- Server membuka file yang diminta dan mengirimkan isinya ke client.

---

#### F. Menjalankan Server sebagai Daemon
Server dijalankan sebagai daemon agar bisa berjalan di background secara terus-menerus.

##### Kode
```
void daemonize() {
    pid_t pid = fork();
    if (pid > 0) exit(0); // Parent exit
    setsid();             // Buat sesi baru
    pid = fork();
    if (pid > 0) exit(0); // Exit parent kedua
    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
```
##### Penjelasan
Fungsi `daemonize()` memastikan server tetap berjalan di background bahkan setelah terminal ditutup.

---

#### G. Logging Aktivitas Server dan Client
- Semua aktivitas penting dicatat dalam file log `server/server.log`.
- Format log: `[Source][YYYY-MM-DD hh:mm:ss]: [ACTION] [Info]`
##### Kode
```
void log_event(const char* role, const char* action, const char* detail) {
    FILE* log = fopen("server/server.log", "a");
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(log, "[%s][%s]: [%s] [%s]\n", role, ts, action, detail);
    fclose(log);
}
```
##### Penjelasan
- Setiap kali ada aksi (decrypt, save, download, error), `log_event()` akan dipanggil untuk mencatat kejadian tersebut ke dalam log.

#### H. Kesimpulan
Program client-server `image_client.c` dan `image_server.c` berhasil membentuk sistem komunikasi berbasis RPC menggunakan socket TCP. Program ini mampu:
- Mengunduh dan membaca file teks terenkripsi dari file txt.
- Melakukan dekripsi menggunakan metode reverse text dan decode hex.
- Menyimpan hasil dekripsi dalam bentuk file `.jpeg` di server.
- Memungkinkan client untuk mengunduh file hasil dekripsi dari server melalui menu interaktif.
- Mencatat semua aktivitas ke dalam log `server.log` dengan format yang konsisten.
- Menangani kesalahan seperti koneksi gagal, file tidak ditemukan, dan error pada dekripsi. <br>
Dengan keberhasilan sistem ini, client dapat mengakses dan mengungkap isi pesan rahasia dari masa lalu untuk melacak keberadaan hacker legendaris “rootkids”. Sistem juga dirancang agar robust, modular, dan dapat digunakan secara berulang dengan proses yang otomatis maupun manual sesuai kebutuhan skenario.

### Kendala
Selama proses saya mengerjakan nomor 1, saya mengalami kendala saat mencoba mendekripsi isi file teks menjadi gambar. Masalah utamanya adalah:

#### Masalah
File hasil dekripsi tidak dapat dibuka sebagai gambar karena urutan data tidak sesuai. Setelah ditelusuri, ternyata saya lupa melakukan proses reverse text sebelum melakukan decoding dari format hex. Hal ini menyebabkan struktur file gambar (signature file) berada di akhir, bukan di awal seperti yang seharusnya (FFD8 untuk JPEG signature), sehingga file dianggap rusak oleh image viewer.

#### Solusi & Revisi 
Untuk mengatasi hal ini, saya menambahkan proses pembalikan string (reverse text) sebelum dikonversi dari hex ke biner. Dengan cara ini, struktur file kembali seperti semula dan hasil decoding berhasil dibuka sebagai file JPEG yang valid.

Kode tersebut diterapkan di sisi server sebelum proses penyimpanan file JPEG. Setelah perbaikan ini, seluruh proses dekripsi berjalan lancar dan file dapat ditampilkan dengan benar.
##### Kode
```
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
```
##### Penjelasan
Dua fungsi ini dibuat untuk mengatasi kendala dekripsi file JPEG yang rusak:
- `reverse_string()` digunakan untuk membalik urutan teks terenkripsi karena struktur file JPEG harus dimulai dengan signature tertentu (FFD8). Tanpa dibalik, signature berada di akhir sehingga gambar tidak terbaca.
- `hex_to_bin()` mengubah teks hasil reverse dari format heksadesimal menjadi data biner mentah, agar bisa disimpan sebagai file .jpeg yang valid dan dikenali image viewer.

### Dokumentasi
Dokumentasi pengerjaan nomor 1 <br> <br>

#### [1] Jika dijalankan `./server/image_server`
Perintah ``./server/image_server`` akan menjalankan sebagai daemon
![desc](assets/01%201.%20server.png) <br>

### [2] Jika dijalankan `./client/image_client`
Perintah ``./action -m Filter`` akan mendownload dan unzip dari link google drive yang sudah diperintahkan yang berisi file txt, lalu menampilkan menu interaktif<br>
![desc](assets/01%202.%20client.png) <br>

### [3] Menu 1
input angka 1 pada menu, dan menambahkan nama file yang akan di decrypt dan dikirimkan ke server <br>
![desc](assets/01%203.%20menu%201.png) <br>


### [4] Menu 2
input angka 2 pada menu, dan menambahkan nama file yang akan didownload atau dikirimkan kepada client<br>
![desc](assets/01%204.%20menu%202.png) <br>

### [5] Tampilan Gambar
Output dari file input_1.txt yang sudah di decrypt
![desc](assets/01%205.%20gambar.png)<br>

### [6] Isi dari `server.log`
![desc](assets/01%206.%20log.png)<br>

---

## Soal 2
### Oleh: Kaisar Hanif Pratama (5027241029)

### Soal
Tahun 2025, di tengah era perdagangan serba cepat, berdirilah sebuah perusahaan ekspedisi baru bernama RushGo. RushGo ingin memberikan layanan ekspedisi terbaik dengan 2 pilihan, Express (super cepat) dan Reguler (standar). Namun, pesanan yang masuk sangat banyak! Mereka butuh sistem otomatisasi pengiriman agar agen tidak kewalahan. Sistem ini terdiri dari dua bagian: <br>
1. `delivery_agent.c` untuk agen otomatis pengantar Express
2. `dispatcher.c` untuk pengiriman dan monitoring pesanan oleh user <br>

### Jawaban
#### A. Mengunduh File Order dan Menyimpannya ke Shared Memory
- File CSV (`delivery_order.csv`) diunduh dan disalin sebagai `delivery_order_downloaded.csv`
- Data dibaca dan disimpan ke shared memory menggunakan struktur `SharedMemory` <br>

##### Kode
```
void download_csv() {
    system("cp delivery_order.csv delivery_order_downloaded.csv");
    printf("File downloaded as delivery_order_downloaded.csv\n");
}

void read_csv_to_shared_memory() {
    FILE *file = fopen("delivery_order_downloaded.csv", "r");
    ...
    while (fgets(line, sizeof(line), file) && index < MAX_ORDERS) {
        ...
        strncpy(shared_mem->orders[index].name, token, ...);
        strncpy(shared_mem->orders[index].address, token, ...);
        strncpy(shared_mem->orders[index].type, token, ...);
        shared_mem->orders[index].delivered = 0;
        shared_mem->orders[index].order_time = time(NULL);
        index++;
    }
    shared_mem->count = index;
    fclose(file);
}
```
##### Penjelasan
- Fungsi `download_csv()` menyalin file order.
- Fungsi `read_csv_to_shared_memory()` membaca file CSV dan menyimpannya ke shared memory.

---

#### B. Pengiriman Bertipe Express
- Tiga agen: AGENT A, AGENT B, AGENT C dijalankan sebagai thread.
- Masing-masing agen secara otomatis mencari pesanan bertipe "Express" yang belum dikirim.

##### Kode
```
void *agent_delivery(void *arg) {
    char *agent_name = (char *)arg;
    while (1) {
        sem_wait(&shared_mem->semaphore);
        for (int i = 0; i < shared_mem->count; i++) {
            if (strcmp(shared_mem->orders[i].type, "Express") == 0 &&
                shared_mem->orders[i].delivered == 0) {
                shared_mem->orders[i].delivered = 1;
                sleep(1);
                log_delivery(agent_name, ...);
                break;
            }
        }
        sem_post(&shared_mem->semaphore);
        sleep(1);
    }
}
```
##### Penjelasan
- Tiap thread agen mencari pesanan Express yang belum terkirim.
- Setelah berhasil kirim, dicatat ke `delivery.log` dengan timestamp dan informasi agen.

---

#### C. Pengiriman Bertipe Reguler
- Dijalankan manual oleh user via `./dispatcher -deliver [Nama]`
- Agen adalah user yang menjalankan perintah tersebut

##### Kode - `dispatcher.c`
```
else if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
    for (int i = 0; i < orderList->count; i++) {
        if (strcmp(orderList->orders[i].name, argv[2]) == 0 &&
            strcmp(orderList->orders[i].type, "Reguler") == 0) {
            strcpy(orderList->orders[i].status, "Delivered");
            char *username = getenv("USER");
            strcpy(orderList->orders[i].agent, username);
            logDelivery(username, ...);
        }
    }
}
```
##### Penjelasan
- Cek apakah order Reguler atas nama tersebut masih "Pending".
- Tandai sebagai "Delivered", dan agent diset nama user.
---

#### D. Mengecek Status Pesanan
- Digunakan perintah `./dispatcher -status [Nama]`
- Program akan menampilkan status dan nama agent yang mengirimkan.

##### Kode `dispatcher.c`
```
else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
    for (int i = 0; i < orderList->count; i++) {
        if (strcmp(orderList->orders[i].name, argv[2]) == 0) {
            printf("Status for %s: %s by Agent %s\n", ...);
        }
    }
}
```
##### Penjelasan
- Program mencari order berdasarkan nama dan menampilkan status terkini dan nama agen.
---

#### E. Melihat Daftar Semua Pesanan
- Menggunakan `./dispatcher -list`
- Akan menampilkan seluruh order beserta status dan agen pengantar

##### Kode - `dispatcher.c`
```
if (strcmp(argv[1], "-list") == 0) {
    for (int i = 0; i < orderList->count; i++) {
        printf("%d. %s - %s - %s - %s\n", ...);
    }
}
```

##### Penjelasan
- Menampilkan semua order dari shared memory, termasuk nama, jenis, status, dan agen yang bertugas.

---

#### F. Kesimpulan
Dengan implementasi dua program utama `delivery_agent` dan `dispatcher`, sistem Delivery Management RushGo dapat menangani pengiriman express secara otomatis oleh agen, dan reguler secara manual oleh user. Shared memory digunakan sebagai penyimpanan bersama antara agent dan dispatcher, serta semua aktivitas dicatat pada `delivery.log`.

## Soal 3
### Oleh: Revalina Erica Permatasari (5027241007)
#### a. Entering the dungeon
dungeon.c akan bekerja sebagai server yang dimana client (player.c) dapat terhubung melalui RPC. dungeon.c akan memproses segala perintah yang dikirim oleh player.c. Lebih dari 1 client dapat mengakses server.
##### Code
``dungeon.c``
```
int main() {
    initClients();
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("%s[Server Started] Listening on port %d...%s\n", CYAN, PORT, RESET);
............

    while ((client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len))) {
        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        if (!pclient) {
            perror("Memory allocation failed");
            close(client_sock);
            continue;
        }
        *pclient = client_sock;
        if (pthread_create(&tid, NULL, handlePlayer, pclient) != 0) {
            perror("Thread creation failed");
            free(pclient);
            close(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

```
- Menjalankan dungeon sebagai **server TCP**
- Menerima koneksi dari client (`player.c`)
- Memproses setiap client secara **paralel dengan thread**
1. **Inisialisasi Data Client**
   - `initClients();`  
     Mengatur semua slot player agar tidak aktif (`active = 0`).
2. **Membuat TCP Socket**
   - `socket(AF_INET, SOCK_STREAM, 0);`  
     Membuat socket untuk koneksi TCP IPv4.
3. **Mengatur Opsi Reuse Address**
   - `setsockopt(..., SO_REUSEADDR, ...)`  
     Agar port bisa dipakai ulang tanpa error “Address already in use”.
4. **Mengisi Informasi Alamat Server**
   - IP: `INADDR_ANY` → menerima dari semua IP  
   - Port: `htons(PORT)` → port server
5. **Binding Socket ke Alamat dan Port**
   - `bind()`  
     Mengikat socket agar bisa menerima koneksi.
6. **Mendengarkan Koneksi Client**
   - `listen(..., 5);`  
     Server siap menerima hingga 5 koneksi pending.
7. **Mencetak Status Server**
   - Menampilkan log bahwa server aktif di terminal.
8. **Loop Accept Client**
   - `while ((client_sock = accept(...)))`  
     Server akan terus menunggu koneksi client baru.
9. **Alokasi Memori untuk Socket Client**
   - `malloc(sizeof(int))`  
     Menyimpan socket client sebagai pointer untuk digunakan di thread.
10. **Membuat Thread untuk Client**
    - `pthread_create()`  
      Menjalankan `handlePlayer()` di thread baru untuk setiap client.
11. **Melepas Thread Otomatis**
    - `pthread_detach()`  
      Supaya thread tidak perlu di-join dan tidak jadi zombie.
- Jika `socket`, `bind`, `listen`, atau `accept` gagal >> cetak pesan dan keluar.
- Jika `malloc` atau `pthread_create` gagal >> tolak client dengan aman.
12. **Menutup Socket Server**
    - `close(server_fd);`  
      Dilakukan jika loop keluar karena kesalahan.

``player.c``
```
int main() {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        close(sock);
        return 1;
    }

    printf("%sConnected to dungeon server.%s\n", GREEN, RESET);
......
```
- Menjalankan client yang menghubungkan pemain ke server (`dungeon.c`)
- Menampilkan menu utama dungeon kepada pemain
- Mengirim perintah ke server dan menerima respon

1. **Membuat Socket TCP**
   - `socket(AF_INET, SOCK_STREAM, 0);`  
     Membuat socket TCP untuk berkomunikasi dengan server.
2. **Mengatur Alamat Server**
   - `inet_addr("127.0.0.1")` → alamat IP lokal  
   - `htons(PORT)` → port server dungeon  
   - Mengisi `struct sockaddr_in` dengan informasi tersebut
3. **Mencoba Koneksi ke Server**
   - `connect(sock, (struct sockaddr *)&server, ...)`  
     Menghubungkan socket client ke socket server dungeon
4. **Menangani Gagal Koneksi**
   - Jika gagal konek (server belum aktif), tampilkan pesan error:
     ```
     Connect failed: Connection refused
     ```
5. **Koneksi Berhasil**
   - Jika sukses, tampilkan:
     ```
     Connected to dungeon server.
     ```
- Variabel `buffer[]` dan `input[]` digunakan untuk mengirim perintah dan menerima data.

##### Output


#### b. Sightseeing 
Anda melihat disekitar dungeon dan menemukan beberapa hal yang menarik seperti toko senjata dan pintu dengan aura yang cukup seram. Ketika player.c dijalankan, ia akan terhubung ke dungeon.c dan menampilkan sebuah main menu.
##### Code
``player.c``
```
while (1) {
        printf("\n%s╔════════════════════════════════╗%s\n", YELLOW, RESET);
        printf("%s║ %s🎮 %sWELCOME TO THE ADVENTURE!%s🎮 ║%s\n", YELLOW, GREEN, YELLOW, GREEN, RESET);
        printf("%s╠════════════════════════════════╣%s\n", YELLOW, RESET);
        printf("%s║ %s1. Show Stats       %s💥         ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s2. Show Shop        %s🛒         ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s3. Buy Weapon       %s⚔️         ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s4. Inventory & Equip %s🎒        ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s5. Battle           %s🔥         ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s6. Exit             %s🚪         ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s╚════════════════════════════════╝%s\n", YELLOW, RESET);

        printf("%sEnter your choice: %s", GREEN, RESET);
        if (!fgets(input, BUFFER_SIZE, stdin)) {
            printf("%sError reading input.%s\n", RED, RESET);
            continue;
        }
        input[strcspn(input, "\n")] = 0;
.....}
```

##### Output

#### c. Status Check
Melihat bahwa terdapat sebuah toko senjata, anda mengecek status diri anda dengan harapan anda masih memiliki sisa uang untuk membeli senjata. Jika opsi Show Player Stats dipilih, maka program akan menunjukan Uang yang dimiliki (Jumlah dibebaskan), senjata yang sedang digunakan, Base Damage, dan jumlah musuh yang telah dimusnahkan. 
##### Code
``dungeon.c``
```
void showStats(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE * 2];
    snprintf(buffer, sizeof(buffer),
        "\n%s╔════════════════════════════════════════════╗%s\n"
        "%s║              🎮 Player Stats 🎮            ║%s\n"
        "%s╠════════════════════════════════════════════╣%s\n"
        "%s║ 💰 Gold             : %-21d║%s\n"
        "%s║ 🗡️ Weapon           : %-21s║%s\n"
        "%s║ ⚔️ Base Damage      : %-21d║%s\n"
        "%s║ 🔮 Passive Ability  : %-21s║%s\n"
        "%s║ 👾 Enemies Defeated : %-21d║%s\n"
        "%s╚════════════════════════════════════════════╝%s\n",
        YELLOW, RESET, YELLOW, RESET, YELLOW, RESET,
        GREEN, p->gold, RESET,
        BLUE, p->hasWeapon ? p->currentWeapon.name : "None", RESET,
        CYAN, p->baseDamage, RESET,
        MAGENTA, p->hasWeapon && p->currentWeapon.hasPassive ? p->currentWeapon.passive : "-", RESET,
        RED, p->enemiesDefeated, RESET,
        YELLOW, RESET);
    send(client_sock, buffer, strlen(buffer), 0);
}
```

``player.c``
```
int main () {
........
if (strcmp(input, "1") == 0) {
            if (send(sock, "STATS", strlen("STATS"), 0) < 0) {
                perror("Send failed");
                break;
            }
.......
}
```

##### Output

#### d. Weapon Shop
Ternyata anda memiliki sisa uang dan langsung pergi ke toko senjata tersebut untuk membeli senjata. Terdapat 5 pilihan senjata di toko tersebut dan beberapa dari mereka memiliki passive yang unik. Disaat opsi Shop dipilih, program akan menunjukan senjata apa saja yang dapat dibeli beserta harga, damage, dan juga passive (jika ada). List senjata yang ada dan dapat dibeli beserta logic/command untuk membeli senjata tersebut diletakan di code shop.c yang nanti akan dipakai oleh dungeon.c.
##### Code
``shop.c``
```
Weapon weapons[MAX_WEAPONS] = {
    {"Iron Sword", 50, 10, "-", 0},
    {"Flame Blade", 200, 25, "Burn: 10% chance to deal 2x damage", 1},
    {"Poison Dagger", 90, 12, "Poison: Deals 5 damage per turn", 1},
    {"Thunder Staff", 250, 30, "Shock: 20% chance to chain attack", 1},
    {"Sky Piercer", 275, 20, "Execute: Auto-kill enemies <20%% HP", 1},
    {"Blade of Despair", 600, 45, "Despair: +25% damage to enemies <50%% HP", 1},
    {"Wind of Nature", 480, 0, "Wind Chant: Immune to phys damage (2s)", 1},
    {"Bloodlust Axe", 520, 28, "Bloodlust: 20%% spell vamp", 1},
    {"Sea Halberd", 500, 30, "Life Drain: -50%% HP regen", 1}
};

Weapon* buyWeapon(int choice, Player *p) {
    if (choice < 1 || choice > MAX_WEAPONS) {
        return NULL;
    }

    Weapon *selected = &weapons[choice - 1];
    if (p->gold < selected->price) {
        return NULL;
    }

    if (p->weaponCount >= MAX_WEAPONS) {
        return NULL;
    }

    p->gold -= selected->price;
    p->inventory[p->weaponCount] = *selected;
    p->weaponCount++;
    return selected;
}
```

``dungeon.c``
```
void displayShop(int client_sock) {
    char buffer[BUFFER_SIZE] = "";

    snprintf(buffer, sizeof(buffer),
        "\n%s╔════╦══════════════════════╦═══════╦════════╦════════════════════════════════════╗%s\n"
        "%s║ ID ║ Name                 ║ Price ║ Damage ║ Passive                            ║%s\n"
        "%s╠════╬══════════════════════╬═══════╬════════╬════════════════════════════════════╣%s\n",
        YELLOW, RESET, YELLOW, RESET, YELLOW, RESET);

    for (int i = 0; i < MAX_WEAPONS; i++) {
        const char *passive = weapons[i].hasPassive ? weapons[i].passive : "-";
        char passiveLine1[41] = "", passiveLine2[41] = "";

        strncpy(passiveLine1, passive, 40);
        passiveLine1[40] = '\0';
        if (strlen(passive) > 40) {
            strncpy(passiveLine2, passive + 40, 40);
            passiveLine2[40] = '\0';
        }
        char line[512];

        if (strlen(passiveLine2) == 0) {
            snprintf(line, sizeof(line),
                "%s║ %-2d ║ %-20s ║ %-5d ║ %-6d ║ %-40s ║%s\n",
                YELLOW, i + 1, weapons[i].name, weapons[i].price, weapons[i].base_damage, passiveLine1, RESET);
        } else {
            snprintf(line, sizeof(line),
                "%s║ %-2d ║ %-20s ║ %-5d ║ %-6d ║ %-40s ║%s\n"
                "%s║    ║                      ║       ║        ║ %-40s ║%s\n",
                YELLOW, i + 1, weapons[i].name, weapons[i].price, weapons[i].base_damage, passiveLine1, RESET,
                YELLOW, passiveLine2, RESET);
        }
        strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
    }
    strncat(buffer, YELLOW "╚════╩══════════════════════╩═══════╩════════╩════════════════════════════════════╝\n" RESET, sizeof(buffer) - strlen(buffer) - 1);
    send(client_sock, buffer, strlen(buffer), 0);
}
```

##### Output

#### e. Handy Inventory
Setelah membeli senjata di toko tadi, anda membuka ransel anda untuk memakai senjata tersebut. Jika opsi View Inventory dipilih, program akan menunjukan senjata apa saja yang dimiliki dan dapat dipakai (jika senjata memiliki passive, tunjukan juga passive tersebut). Lalu apabila opsi Show Player Stats dipilih saat menggunakan weapon maka Base Damage player akan berubah dan jika memiliki passive, maka akan ada status tambahan yaitu Passive.
##### Code
``dungeon.c``
```
void showInvent(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE * 2];
    strcpy(buffer,
        "\n╔════╦════════════════════════════╦════════╦════════════════════════════════════╗\n"
        "║ ID ║ Name                       ║ Damage ║ Passive                            ║\n"
        "╠════╬════════════════════════════╬════════╬════════════════════════════════════╣\n");

    for (int i = 0; i < p->weaponCount; i++) {
        char line[512];
        const char *name = p->inventory[i].name;
        int damage = p->inventory[i].base_damage;
        const char *passive = p->inventory[i].hasPassive ? p->inventory[i].passive : "-";

        char passive1[41] = "", passive2[41] = "";
        strncpy(passive1, passive, 40);
        passive1[40] = '\0';
        if (strlen(passive) > 40) {
            strncpy(passive2, passive + 40, 40);
            passive2[40] = '\0';
        }

        if (strlen(passive2) == 0) {
            snprintf(line, sizeof(line),
                "║ %-2d ║ %-28s ║ %-6d ║ %-36s ║\n",
                i + 1, name, damage, passive1);
        } else {
            snprintf(line, sizeof(line),
                "║ %-2d ║ %-28s ║ %-6d ║ %-36s ║\n"
                "║    ║                              ║        ║ %-36s ║\n",
                i + 1, name, damage, passive1, passive2);
        }

        strcat(buffer, line);
    }

    strcat(buffer, "╚════╩════════════════════════════╩════════╩════════════════════════════════════╝\n");
    send(client_sock, buffer, strlen(buffer), 0);
}
```

``player.c``
```
else if (strcmp(input, "4") == 0) {
            if (send(sock, "INVENTORY", strlen("INVENTORY"), 0) < 0) {
                perror("Send failed");
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
                printf("%sDisconnected from server.%s\n", RED, RESET);
                break;
            }
            printf("%s\n", buffer);
            printf("%sEnter Weapon ID to equip (0 to cancel): %s", GREEN, RESET);
            if (!fgets(input, BUFFER_SIZE, stdin)) {
                printf("%sError reading input.%s\n", RED, RESET);
                continue;
            }
            input[strcspn(input, "\n")] = 0;
            int choice;
            if (sscanf(input, "%d", &choice) != 1 || choice < 0) {
                printf("%sInvalid input!%s\n", RED, RESET);
                continue;
            }
            if (choice == 0) {
                printf("%sCanceled.%s\n", YELLOW, RESET);
                continue;
            }
            snprintf(buffer, sizeof(buffer), "EQUIP %d", choice);
            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("Send failed");
                break;
            }
```

##### Output

#### f.	Enemy Encounter
Anda sekarang sudah siap untuk melewati pintu yang seram tadi, disaat anda memasuki pintu tersebut, anda langsung ditemui oleh sebuah musuh yang bukan sebuah manusia. Dengan tekad yang bulat, anda melawan musuh tersebut. Saat opsi Battle Mode dipilih, program akan menunjukan health-bar musuh serta angka yang menunjukan berapa darah musuh tersebut dan menunggu input dengan opsi attack untuk melakukan sebuah serangan dan juga exit untuk keluar dari Battle Mode. Apabila darah musuh berkurang, maka health-bar musuh akan berkurang juga. Jika darah musuh sudah 0, maka program akan menunjukan rewards berupa berapa banyak gold yang didapatkan lalu akan muncul musuh lagi.
##### Code
``dungeon.c``
```
void handleBattle(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE];
    int enemy_hp = rand() % 151 + 50; 
    int player_hp = 100; 
    while (enemy_hp > 0 && player_hp > 0) {
        int bar_length = 20;
        int enemy_bar = (enemy_hp * bar_length) / 200;
        char health_bar[bar_length + 1];
        memset(health_bar, ' ', bar_length);
        for (int i = 0; i < enemy_bar; i++) {
            health_bar[i] = '#';
        }
        health_bar[bar_length] = '\0';

        snprintf(buffer, sizeof(buffer),
         "\n%s╔══════════════════════════════════════╗%s\n"
         "║ %sEnemy HP: %d%s [%s]%s\n"
         "║ %sYour HP: %d%s\n"
         "║ Options: [attack] [exit]            ║%s\n"
         "%s╚══════════════════════════════════════╝%s\n",
         YELLOW, RESET, RED, enemy_hp, RESET, health_bar, RESET,
         GREEN, player_hp, RESET, YELLOW, YELLOW, RESET);
        send(client_sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected during battle.\n");
            removeClient(client_sock);
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "exit") == 0) {
            enemy_hp = rand() % 151 + 50;
            snprintf(buffer, sizeof(buffer), "%sBattle exited. Enemy HP reset.%s\n", YELLOW, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
            return;
        } else if (strcmp(buffer, "attack") == 0) {
            int base_dmg = p->hasWeapon ? p->baseDamage : 5;
            int damage = base_dmg + (rand() % 10);
            int is_critical = (rand() % 100) < 20;
            if (is_critical) {
                damage *= 2;
                snprintf(buffer, sizeof(buffer), "%sCritical Hit!%s\n", RED, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
            }
            int passive_triggered = 0;
            if (p->hasWeapon && p->currentWeapon.hasPassive) {
                if (strcmp(p->currentWeapon.passive, "Burn: 10% chance to deal 2x damage") == 0 && rand() % 100 < 10) {
                    damage *= 2;
                    passive_triggered = 1;
                    snprintf(buffer, sizeof(buffer), "%sPassive Burn activated!%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Poison: Deals 5 damage per turn") == 0) {
                    enemy_hp -= 5;
                    passive_triggered = 1;
                    snprintf(buffer, sizeof(buffer), "%sPassive Poison activated! Dealt 5 extra damage.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Shock: 20% chance to chain attack") == 0 && rand() % 100 < 20) {
                    int chain_damage = rand() % 10 + 5;
                    enemy_hp -= chain_damage;
                    snprintf(buffer, sizeof(buffer), "%sPassive Shock activated! Chained attack! Dealt %d extra damage.%s\n", MAGENTA, chain_damage, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Execute: Auto-kill enemies <20%% HP") == 0 && enemy_hp < 40) {
                    enemy_hp = 0;
                    snprintf(buffer, sizeof(buffer), "%sPassive Execute activated! Enemy auto-killed due to low HP.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Despair: +25%% damage to enemies <50%% HP") == 0 && enemy_hp < 100) {
                    damage = (int)(damage * 1.25);
                    snprintf(buffer, sizeof(buffer), "%sPassive Despair activated! +25%% damage to enemy with HP below 50%%.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Wind Chant: Immune to phys damage (2s)") == 0) {
                    snprintf(buffer, sizeof(buffer), "%sPassive Wind Chant activated! Immune to physical damage for 2 seconds.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Bloodlust: 20%% spell vamp") == 0) {
                    int heal_amount = (int)(damage * 0.20);
                    player_hp += heal_amount;
                    snprintf(buffer, sizeof(buffer), "%sPassive Bloodlust activated! Healed %d HP.%s\n", MAGENTA, heal_amount, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Life Drain: -50%% HP regen") == 0) {
                    enemy_hp -= 10; 
                    snprintf(buffer, sizeof(buffer), "%sPassive Life Drain activated! Dealt 10 extra damage.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                }
            }
            enemy_hp -= damage;
            snprintf(buffer, sizeof(buffer),
                     "%sYou dealt %d damage! Enemy HP: %d%s\n",
                     GREEN, damage, enemy_hp > 0 ? enemy_hp : 0, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
            if (enemy_hp <= 0) {
                int reward = rand() % 51 + 50;
                p->gold += reward;
                p->enemiesDefeated++;
                snprintf(buffer, sizeof(buffer),
                         "%sEnemy defeated! You earned %d gold. Total gold: %d%s\n",
                         GREEN, reward, p->gold, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
                enemy_hp = rand() % 151 + 50;
                continue;
            }
            int enemy_dmg = rand() % 11 + 5;
            player_hp -= enemy_dmg;
            snprintf(buffer, sizeof(buffer),
                     "%sEnemy dealt %d damage! Your HP: %d%s\n",
                     RED, enemy_dmg, player_hp > 0 ? player_hp : 0, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
            if (player_hp <= 0) {
                snprintf(buffer, sizeof(buffer), "%sYou were defeated!%s\n", RED, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
                return;
            }
        } else {
            snprintf(buffer, sizeof(buffer), "%sInvalid option. Use 'attack' or 'exit'.%s\n", RED, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
}
```

``player.c``
```
int main () {
.....
else if (strcmp(input, "5") == 0) {
            if (send(sock, "BATTLE", strlen("BATTLE"), 0) < 0) {
                perror("Send failed");
                break;
            }
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t received = recv(sock, buffer, BUFFER_SIZE, 0);
                if (received <= 0) {
                    printf("%sDisconnected from server.%s\n", RED, RESET);
                    break;
                }
                printf("%s\n", buffer);
                if (strstr(buffer, "defeated") || strstr(buffer, "exited")) break;
                printf("%sEnter action (attack/exit): %s", GREEN, RESET);
                if (!fgets(input, BUFFER_SIZE, stdin)) {
                    printf("%sError reading input.%s\n", RED, RESET);
                    break;
                }
                input[strcspn(input, "\n")] = 0;
                if (send(sock, input, strlen(input), 0) < 0) {
                    perror("Send failed");
                    break;
                }
            }
            continue;
....}
```

##### Output

#### g. Other Battle Logic
-	**Health & Rewards**
Untuk darah musuh, seberapa banyak darah yang mereka punya dibuat secara random, contoh: 50-200 HP. Lakukan hal yang sama untuk rewards. 
##### Code
```
void handleBattle(int client_sock, Player *p) {
..........
if (enemy_hp <= 0) {
                int reward = rand() % 51 + 50;
                p->gold += reward;
                p->enemiesDefeated++;
                snprintf(buffer, sizeof(buffer),
                         "%sEnemy defeated! You earned %d gold. Total gold: %d%s\n",
                         GREEN, reward, p->gold, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
                enemy_hp = rand() % 151 + 50;
                continue;
            }
```

##### Output

-	**Damage Equation**
Untuk damage, gunakan base damage sebagai kerangka awal dan tambahkan rumus damage apapun (dibebaskan, yang pasti perlu random number agar hasil damage bervariasi). Lalu buatlah logic agar setiap serangan memiliki kesempatan untuk Critical yang membuat damage anda 2x lebih besar.
##### Code
```
void handleBattle(int client_sock, Player *p) {
..........
 } else if (strcmp(buffer, "attack") == 0) {
            int base_dmg = p->hasWeapon ? p->baseDamage : 5;
            int damage = base_dmg + (rand() % 10);
            int is_critical = (rand() % 100) < 20;
            if (is_critical) {
                damage *= 2;
                snprintf(buffer, sizeof(buffer), "%sCritical Hit!%s\n", RED, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
            }
```

##### Output

-	**Passive**
Jika senjata yang dipakai memiliki Passive setiap kali passive tersebut menyala, maka tunjukan bahwa passive tersebut aktif.
##### Code
```
void handleBattle(int client_sock, Player *p) {
.........
int passive_triggered = 0;
            if (p->hasWeapon && p->currentWeapon.hasPassive) {
                if (strcmp(p->currentWeapon.passive, "Burn: 10% chance to deal 2x damage") == 0 && rand() % 100 < 10) {
                    damage *= 2;
                    passive_triggered = 1;
                    snprintf(buffer, sizeof(buffer), "%sPassive Burn activated!%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Poison: Deals 5 damage per turn") == 0) {
                    enemy_hp -= 5;
                    passive_triggered = 1;
                    snprintf(buffer, sizeof(buffer), "%sPassive Poison activated! Dealt 5 extra damage.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Shock: 20% chance to chain attack") == 0 && rand() % 100 < 20) {
                    int chain_damage = rand() % 10 + 5;
                    enemy_hp -= chain_damage;
                    snprintf(buffer, sizeof(buffer), "%sPassive Shock activated! Chained attack! Dealt %d extra damage.%s\n", MAGENTA, chain_damage, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Execute: Auto-kill enemies <20%% HP") == 0 && enemy_hp < 40) {
                    enemy_hp = 0;
                    snprintf(buffer, sizeof(buffer), "%sPassive Execute activated! Enemy auto-killed due to low HP.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Despair: +25%% damage to enemies <50%% HP") == 0 && enemy_hp < 100) {
                    damage = (int)(damage * 1.25);
                    snprintf(buffer, sizeof(buffer), "%sPassive Despair activated! +25%% damage to enemy with HP below 50%%.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Wind Chant: Immune to phys damage (2s)") == 0) {
                    snprintf(buffer, sizeof(buffer), "%sPassive Wind Chant activated! Immune to physical damage for 2 seconds.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Bloodlust: 20%% spell vamp") == 0) {
                    int heal_amount = (int)(damage * 0.20);
                    player_hp += heal_amount;
                    snprintf(buffer, sizeof(buffer), "%sPassive Bloodlust activated! Healed %d HP.%s\n", MAGENTA, heal_amount, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Life Drain: -50%% HP regen") == 0) {
                    enemy_hp -= 10; 
                    snprintf(buffer, sizeof(buffer), "%sPassive Life Drain activated! Dealt 10 extra damage.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                }
            }
```

##### Output

#### h. Error Handling
Berikan error handling untuk opsi-opsi yang tidak ada.
##### Code
``dungeon.c``
###### Error Handling untuk attack atau exit saat Battle
```
void handleBattle(int client_sock, Player *p) {
..........
else {
            snprintf(buffer, sizeof(buffer), "%sInvalid option. Use 'attack' or 'exit'.%s\n", RED, RESET);
            send(client_sock, buffer, strlen(buffer), 0); }
```
###### Error Handling saat Memilih Weapon, Kekurangan Gold, Inventory Full, Pembelian yang tidak berhasil, dan Unknown Command 
```
void* handlePlayer(void* arg) {
........
} else {
                if (id < 1 || id > MAX_WEAPONS) {
                    snprintf(buffer, sizeof(buffer), "%s❌ Invalid weapon ID.%s\n", RED, RESET);
                } else if (player->gold < weapons[id - 1].price) {
                    snprintf(buffer, sizeof(buffer), "%s❌ Insufficient gold.%s\n", RED, RESET);
                } else if (player->weaponCount >= MAX_WEAPONS) {
                    snprintf(buffer, sizeof(buffer), "%s❌ Inventory full.%s\n", RED, RESET);
                } else {
                    snprintf(buffer, sizeof(buffer), "%s❌ Purchase failed.%s\n", RED, RESET);
                } }
......
} else {
            snprintf(buffer, sizeof(buffer), "%s❓ Unknown command.%s\n", RED, RESET);
            send(client_sock, buffer, strlen(buffer), 0);}
```
###### Error Handling saat Meemilih Menu
```
int main () {
....
else {
            printf("%sInvalid option. Please select 1–6.%s\n", RED, RESET);
            continue;
        }
....
```

##### Output

## Soal 4
### Oleh: Revalina Erica Permatasari
#### a. Sung Jin Woo memutuskan untuk membuat dua file, yaitu system.c dan hunter.c. Sung Jin Woo mendapat peringatan bahwa system.c merupakan shared memory utama yang mengelola shared memory hunter-hunter dari hunter.c. Untuk mempermudahkan pekerjaannya, Sung Jin Woo mendapat sebuah clue yang dapat membuat pekerjaannya menjadi lebih mudah dan efisien. NOTE : hunter bisa dijalankan ketika sistem sudah dijalankan.
##### Code
system.c
```
 signal(SIGINT, cleanup);
    srand(time(NULL));

    key_t key = get_system_key();  // dari shm_common.h
    shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    
    if (shmid == -1) {
        perror("Shared memory creation failed.");
        exit(1);
    }
    
    sys = (struct SystemData *)shmat(shmid, NULL, 0);

    if (sys == (void *) -1) {
        perror("Failed to attach shared memory.");
        exit(1);
    }
```
- ``SIGNINT`` Mengatur handler sinyal, biasanya Ctrl+C (untuk me-cancel program yang dijalankan).
- Saat program dihentikan, fungsi ``cleanup()`` akan dijalankan untuk menghapus shared memory agar tidak bocor.
- ``srand(time(NULL))`` untuk Menginisialisasi seed untuk fungsi ``rand()`` agar angka random selalu berubah setiap dijalankan.
- ``key_t key = get_system_key()`` Mengambil key shared memory dari file shm_common.h. Dan identifier untuk shared memory yang akan dipakai hunter juga.
- ``shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666)`` untuk Membuat shared memory (jika belum ada) atau membuka jika sudah ada. 0666 merupakan permission untuk baca-tulis semua user.
- ``if (shmid == -1) {
    perror("Shared memory creation failed.");
    exit(1);
   } ``
   Jika gagal, tampilkan error dan keluar program. Gagalnya bisa karena tidak ada permission, ukuran terlalu besar, key bentrok
- ``sys = (struct SystemData *)shmat(shmid, NULL, 0)`` Disini ``shmat`` itu "shared memory attach" dan Menghubungkan pointer sys ke shared memory yang dibuat.
- ``if (sys == (void *) -1) {
    perror("Failed to attach shared memory.");
    exit(1);
    }``
  Cek apakah attach berhasil. Jika tidak, keluar program.

hunter.c
```
 key_t key = get_system_key();

    int shmid = shmget(key, sizeof(struct SystemData), 0666);

    if (shmid == -1) {
        perror("[ERROR] Failed to access shared memory");
        printf("Make sure the system has been run first.\n");
        exit(1);
    }

    struct SystemData *sys = (struct SystemData *)shmat(shmid, NULL, 0);
```
- ``key_t key = get_system_key()`` Ambil key yang sama dengan system.c. Karena hunter dan sistem pakai key yang sama, mereka bisa akses memory yang sama.
- ``int shmid = shmget(key, sizeof(struct SystemData), 0666)`` Menyoba akses shared memory yang sudah dibuat oleh system.c. Tidak ada IPC_CREAT di sini artinya hunter tidak akan membuat memory baru, hanya akses. Kalau system belum dijalankan shmget akan gagal.
- ``if (shmid == -1) {
    perror("[ERROR] Failed to access shared memory");
    printf("Make sure the system has been run first.\n");
    exit(1);
}``
Error handler kalau system.c belum jalan >> memory belum dibuat.
- ``struct SystemData *sys = (struct SystemData *)shmat(shmid, NULL, 0)`` Attach pointer sys ke shared memory dan pointer ini nantinya dipakai untuk akses data dungeon serta hunter.
  
##### Output

Hunter tidak bisa dijalankan sebelum sistem dijalankan

<img width="548" alt="image" src="https://github.com/user-attachments/assets/8e02f0ce-f479-4978-b002-b37c6fa15252" />

Sistem dijalankan

<img width="480" alt="image" src="https://github.com/user-attachments/assets/ced58d52-31a3-4e1a-b5b1-d1653eabc198" />

Hunter dapat dijalankan

<img width="365" alt="image" src="https://github.com/user-attachments/assets/b97e1417-cb84-434f-9151-1ecd6e13dca1" />

#### b. Untuk memastikan keteraturan sistem, Sung Jin Woo memutuskan untuk membuat fitur registrasi dan login di program hunter. Setiap hunter akan memiliki key unik dan stats awal (Level=1, EXP=0, ATK=10, HP=100, DEF=5). Data hunter disimpan dalam shared memory tersendiri yang terhubung dengan sistem.
##### Code
```
void hunterRegister(struct SystemData *sys) {
    if (sys->num_hunters >= MAX_HUNTERS) {
        printf("The number of hunters is already full!\n");
        return;
    }

    struct Hunter h;
    printf("Username: ");
    scanf("%s", h.username);

    h.level = 1;
    h.exp = 0;
    h.atk = 10;
    h.hp = 100;
    h.def = 5;
    h.banned = 0;
    h.shm_key = rand();  

    int h_shmid = shmget(h.shm_key, sizeof(struct Hunter), IPC_CREAT | 0666);
    if (h_shmid == -1) {
        perror("[ERROR] shmget for hunter failed");
        return;
    }

    struct Hunter *h_ptr = (struct Hunter *)shmat(h_shmid, NULL, 0);
    *h_ptr = h;
    shmdt(h_ptr);

    sys->hunters[sys->num_hunters++] = h;
    printf("Registration succeeded.\n");
}
```

```
void login(struct SystemData *sys) {
    char username[50];
    printf("Hunter Name: ");
    scanf("%s", username);

    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            struct Hunter *current = &sys->hunters[i];
            printf("Login as %s\n", current->username);

            sys_global = sys;
            hunter_global = current;

            int choose;
            while (1) {
                printf("\n=== \033[1;34m%s's MENU\033[0m ===\n", current->username);
                printf("1. Show Dungeon\n"
                       "2. Dungeon Raid\n"
                       "3. Battle\n"
                       "4. Toggle Notification [%s]\n"
                       "5. Logout\n"
                       "Choose: ", notif_active ? "ON" : "OFF");
                scanf("%d", &choose);

                if (choose == 1) showDungeon(current, sys);
                else if (choose == 2) dungeonRaid(current, sys);
                else if (choose == 3) battle(sys, current);
                else if (choose == 4) {
                    if (!notif_active) {
                        stop_notif = 0;
                        pthread_create(&notif_thread, NULL, notifDungeon, NULL);
                        notif_active = 1;
                    } else {
                        stop_notif = 1;
                        pthread_join(notif_thread, NULL);
                        notif_active = 0;
                    }
                } else {
                    if (notif_active) {
                        stop_notif = 1;
                        pthread_join(notif_thread, NULL);  
                    }
                    break;
                }
            }

            return;
        }
    }

    printf("Hunter not found.\n");
}
```
##### Output
<img width="365" alt="image" src="https://github.com/user-attachments/assets/89c8947b-27a1-4d90-8046-43c2ca7df01f" />

<img width="470" alt="image" src="https://github.com/user-attachments/assets/f444ad78-6821-43a2-9d2f-659af28d134a" />

#### c. Sung Jin Woo menambahkan fitur di sistem yang dapat menampilkan informasi semua hunter yang terdaftar, termasuk nama hunter, level, exp, atk, hp, def, dan status (banned atau tidak). Ini membuat dia dapat melihat siapa hunter terkuat dan siapa yang mungkin melakukan kecurangan.
##### Code
```
void showHunter() {
        printf("=================================================================\n");
        printf("                \033[1;34m Hunter List \033[0m\n");
        printf("=================================================================\n");
        printf("| %-15s | LVL | EXP  | ATK | HP  | DEF | STATUS   |\n", "Name");
        printf("=================================================================\n");
        for (int i = 0; i < sys->num_hunters; i++) {
            struct Hunter h = sys->hunters[i];
            printf("| %-15s | %3d | %4d | %3d | %3d | %3d | %-8s |\n",
                   h.username, h.level, h.exp, h.atk, h.hp, h.def,
                   h.banned ? "BANNED" : "ACTIVE");
        }
        printf("=================================================================\n");
}
```

##### Output

#### d. Sung jin Woo memutuskan untuk membuat fitur unik dalam sistem yang dapat menghasilkan dungeon secara random dengan nama, level minimal hunter, dan stat rewards dengan nilai:
-	🏆Level Minimal : 1 - 5
-	⚔️ATK : 100 - 150 Poin
-	❤️HP  : 50 - 100 Poin
-	🛡️DEF : 25 - 50 Poin
-	🌟EXP : 150 - 300 Poin
Setiap dungeon akan disimpan dalam shared memory sendiri yang berbeda dan dapat diakses oleh hunter.
##### Code
```
void createDungeon() {
    if (sys->num_dungeons >= MAX_DUNGEONS) return;

    struct Dungeon *d = &sys->dungeons[sys->num_dungeons++];
    sprintf(d->name, "Dungeon-%d", rand() % 100);
    d->min_level = rand() % 5 + 1;
    d->atk = rand() % 51 + 100;
    d->hp = rand() % 51 + 50;
    d->def = rand() % 26 + 25;
    d->exp = rand() % 151 + 150;
    d->shm_key = rand(); 
    
    key_t dungeon_key = ftok(".", rand() % 100 + 1);
    d->shm_key = dungeon_key;

    int dungeon_shmid = shmget(dungeon_key, 128, IPC_CREAT | 0666); 
    if (dungeon_shmid == -1) {
        perror("Failed to create dungeon shared memory");
        exit(1);
    }

    printf("Dungeon %s created with shm_key %d.\n", d->name, dungeon_key);
}
```

##### Output

#### e. Sung Jin Woo menambahkan fitur yang menampilkan informasi detail semua dungeon. Fitur ini menampilkan daftar lengkap dungeon beserta nama, level minimum, reward (EXP, ATK, HP, DEF), dan key unik untuk masing-masing dungeon.
##### Code
```
void showDungeon() {
        printf("\n==========================================================================\n");
        printf("                         \033[1;35m Dungeon List \033[0m\n");
        printf("==========================================================================\n");
        printf("| %-15s | LVL | ATK | HP  | DEF | EXP | KEY     |\n", "Name");
        printf("==========================================================================\n");
        for (int i = 0; i < sys->num_dungeons; i++) {
            struct Dungeon d = sys->dungeons[i];
            printf("| %-15s | %3d | %3d | %3d | %3d | %3d | %7d |\n",
                   d.name, d.min_level, d.atk, d.hp, d.def, d.exp, d.shm_key);
        }
        printf("==========================================================================\n");   
}
```

##### Output

#### f. Pada saat yang sama, dungeon yang dibuat oleh sistem juga harus dapat diakses oleh hunter. Sung Jin Woo menambahkan fitur yang menampilkan semua dungeon yang tersedia sesuai dengan level hunter. Disini, hunter hanya dapat menampilkan dungeon dengan level minimum yang sesuai dengan level mereka.
##### Code
```
void showDungeon(struct Hunter *hunter, struct SystemData *sys) {
    printf("\n==========================================================================\n");
    printf("             \033[1;35m Dungeon List for %s (Lv %d) \033[0m\n", hunter->username, hunter->level);
    printf("==========================================================================\n");
    printf("| %-15s | LVL | ATK | HP  | DEF | EXP | KEY     |\n", "Name");
    printf("==========================================================================\n");
    int found = 0;
    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon d = sys->dungeons[i];
        if (hunter->level >= d.min_level) {
            printf("| %-15s | %3d | %3d | %3d | %3d | %3d | %7d |\n",
                   d.name, d.min_level, d.atk, d.hp, d.def, d.exp, d.shm_key);
            found = 1;
        }
    }
    if (!found) {
        printf("| %-70s |\n", "No dungeons available for your level.");
    }
    printf("==========================================================================\n");
}
```

##### Output

#### g. Sung Jin Woo memutuskan untuk menambahkan fitur untuk menguasai dungeon. Ketika hunter berhasil menaklukan sebuah dungeon, dungeon tersebut akan menghilang dari sistem dan hunter akan mendapatkan stat rewards dari dungeon. Jika exp hunter mencapai 500, mereka akan naik level dan exp kembali ke 0.
##### Code
```
void dungeonRaid(struct Hunter *hunter, struct SystemData *sys) {
    if (hunter->banned) {
        printf("You're banned!\n");
        return;
    }

    char dungeonsName[50];
    printf("Enter Dungeon's Name: ");
    scanf("%s", dungeonsName);

    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon d = sys->dungeons[i];
        if (strcmp(d.name, dungeonsName) == 0) {
            if (hunter->level < d.min_level) {
                printf("Insufficient Level.\n");
                return;
            }

            printf("Dungeon %s has been raided!\n", d.name);
            hunter->atk += d.atk;
            hunter->hp += d.hp;
            hunter->def += d.def;
            hunter->exp += d.exp;

            if (hunter->exp >= 500) {
                hunter->level++;
                hunter->exp = 0;
                printf("Level up to %d!\n", hunter->level);
            }

            for (int j = i; j < sys->num_dungeons - 1; j++) {
                sys->dungeons[j] = sys->dungeons[j + 1];
            }
            sys->num_dungeons--;
            return;
        }
    }

    printf("Dungeon not found.\n");
}

int totalStat(struct Hunter *h) {
    return h->atk + h->hp + h->def;
}
```

##### Output

#### h. Sung Jin Woo mengimplementasikan fitur dimana hunter dapat memilih untuk bertarung dengan hunter lain. Tingkat kekuatan seorang hunter bisa dihitung melalui total stats yang dimiliki hunter tersebut (ATK+HP+DEF). Jika hunter menang, maka hunter tersebut akan mendapatkan semua stats milik lawan dan lawannya akan terhapus dari sistem. Jika kalah, hunter tersebutlah yang akan dihapus dari sistem dan semua statsnya akan diberikan kepada hunter yang dilawannya.
##### Code
```
void battle(struct SystemData *sys, struct Hunter *self) {
    if (self->banned) {
        printf("You're banned!\n");
        return;
    }

    printf("\n=== \033[1;34m Other Hunter List\033[0m ===\n");
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *opponent = &sys->hunters[i];
        if (opponent != self) {
            printf("[%d] %s (Lv %d, ATK %d, HP %d, DEF %d)\n", i,
                   opponent->username, opponent->level, opponent->atk, opponent->hp, opponent->def);
        }
    }

    int idx;
    printf("Choose Opponent Index: ");
    scanf("%d", &idx);

    if (idx < 0 || idx >= sys->num_hunters || &sys->hunters[idx] == self) {
        printf("Invalid choice.\n");
        return;
    }

    struct Hunter *opponent = &sys->hunters[idx];
    if (opponent->banned) {
        printf("The opponent is being banned\n");
        return;
    }

    int stat_self = totalStat(self);
    int stat_enemy = totalStat(opponent);

    if (stat_self >= stat_enemy) {
        printf("\033[38;5;208m██╗░░░██╗░█████╗░██╗░░░██╗  ░██╗░░░░░░░██╗██╗███╗░░██╗\n");
        printf("\033[38;5;214m╚██╗░██╔╝██╔══██╗██║░░░██║  ░██║░░██╗░░██║██║████╗░██║\n");
        printf("\033[38;5;220m░╚████╔╝░██║░░██║██║░░░██║  ░╚██╗████╗██╔╝██║██╔██╗██║\n");
        printf("\033[38;5;226m░░╚██╔╝░░██║░░██║██║░░░██║  ░░████╔═████║░██║██║╚████║\n");
        printf("\033[38;5;228m░░░██║░░░╚█████╔╝╚██████╔╝  ░░╚██╔╝░╚██╔╝░██║██║░╚███║\n");
        printf("\033[38;5;229m░░░╚═╝░░░░╚════╝░░╚═════╝░  ░░░╚═╝░░░╚═╝░░╚═╝╚═╝░░╚══╝\n");
        printf("\033[0m");

        self->atk += opponent->atk;
        self->hp += opponent->hp;
        self->def += opponent->def;
        self->exp += opponent->exp;
        self->level += opponent->level;

        for (int j = idx; j < sys->num_hunters - 1; j++) {
            sys->hunters[j] = sys->hunters[j + 1];
        }
        sys->num_hunters--;

    } else {
        printf("\033[38;5;52m░██████╗░░█████╗░███╗░░░███╗███████╗  ░█████╗░██╗░░░██╗███████╗██████╗░\n");
        printf("\033[38;5;88m██╔════╝░██╔══██╗████╗░████║██╔════╝  ██╔══██╗██║░░░██║██╔════╝██╔══██╗\n");
        printf("\033[38;5;124m██║░░██╗░███████║██╔████╔██║█████╗░░  ██║░░██║╚██╗░██╔╝█████╗░░██████╔╝\n");
        printf("\033[38;5;160m██║░░╚██╗██╔══██║██║╚██╔╝██║██╔══╝░░  ██║░░██║░╚████╔╝░██╔══╝░░██╔══██╗\n");
        printf("\033[38;5;196m╚██████╔╝██║░░██║██║░╚═╝░██║███████╗  ╚█████╔╝░░╚██╔╝░░███████╗██║░░██║\n");
        printf("\033[38;5;210m░╚═════╝░╚═╝░░╚═╝╚═╝░░░░░╚═╝╚══════╝  ░╚════╝░░░░╚═╝░░░╚══════╝╚═╝░░╚═╝\n");
        printf("\033[0m");

        opponent->atk += self->atk;
        opponent->hp += self->hp;
        opponent->def += self->def;
        opponent->exp += self->exp;
        opponent->level += self->level;

        for (int i = 0; i < sys->num_hunters; i++) {
            if (&sys->hunters[i] == self) {
                for (int j = i; j < sys->num_hunters - 1; j++) {
                    sys->hunters[j] = sys->hunters[j + 1];
                }
                sys->num_hunters--;
                break;
            }
        }

        printf("You are removed from the system.\n");
        shmdt(sys);
        exit(0);
    }
}
```

##### Output

#### i. Saat sedang memonitoring sistem, Sung Jin Woo melihat beberapa hunter melakukan kecurangan di dalam sistem. Ia menambahkan fitur di sistem yang membuat dia dapat melarang hunter tertentu untuk melakukan raid atau battle. Karena masa percobaan tak bisa berlangsung selamanya 😇, Sung Jin Woo pun juga menambahkan konfigurasi agar fiturnya dapat memperbolehkan hunter itu melakukan raid atau battle lagi. 
##### Code
```
void banHunter() {
    char username[50];
    printf("Enter the hunter's username to ban: ");
    scanf("%s", username);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].banned = 1;
            printf("%s has been banned.\n", username);
            return;
        }
    }
    printf("Not found.\n");
}

void unbanHunter() {
    char username[50];
    printf("Enter the hunter's username to unban: ");
    scanf("%s", username);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].banned = 0;
            printf("%s has been unbanned.\n", username);
            return;
        }
    }
    printf("Not found.\n");
}
```

##### Output

#### j. Setelah beberapa pertimbangan, untuk memberikan kesempatan kedua bagi hunter yang ingin bertobat dan memulai dari awal, Sung Jin Woo juga menambahkan fitur di sistem yang membuat dia bisa mengembalikan stats hunter tertentu ke nilai awal. 
##### Code
```
void resetHunter() {
    char username[50];
    printf("Enter the hunter's username to reset: ");
    scanf("%s", username);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].level = 1;
            sys->hunters[i].exp = 0;
            sys->hunters[i].atk = 10;
            sys->hunters[i].hp = 100;
            sys->hunters[i].def = 5;
            printf("Stats for %s have been reset.\n", username);
            return;
        }
    }
    printf("Not found.\n");
}
```

##### Output

#### k.	Untuk membuat sistem lebih menarik dan tidak membosankan, Sung Jin Woo menambahkan fitur notifikasi dungeon di setiap hunter. Saat diaktifkan, akan muncul informasi tentang semua dungeon yang terbuka dan akan terus berganti setiap 3 detik.
##### Code
```
void *notifDungeon(void *arg) {
    while (!stop_notif) {
        int found = 0;
        for (int i = 0; i < sys_global->num_dungeons; i++) {
            struct Dungeon d = sys_global->dungeons[i];

            if (hunter_global->level >= d.min_level) {
                found = 1;
                printf("\n📢 \033[1;33m[Dungeon Alert]\033[0m %s is available for you! (Level required: %d)\n", d.name, d.min_level);
                fflush(stdout);
            }
        }

        if (!found) {
            printf("\n📢 \033[1;31mNo available dungeons at your level.\033[0m\n");
            fflush(stdout);
        }
        
        sleep(3); 
    }
    return NULL;
}
```

##### Output

#### l. Untuk menambah keamanan sistem agar data hunter tidak bocor, Sung Jin Woo melakukan konfigurasi agar setiap kali sistem dimatikan, maka semua shared memory yang sedang berjalan juga akan ikut terhapus. 
##### Code
```
void cleanup(int sig) {
    printf("\n[!] System shutting down, shared memory cleaned up.\n");
    if (sys != NULL) shmdt(sys);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}
```

##### Output

#### Kendala
##### 1. Hunter masih bisa diakses sebelum system diakses

![Screenshot 2025-05-03 075250](https://github.com/user-attachments/assets/d064485a-5e6c-42a3-a6f3-8f382f7f4688)

##### 2. Notif yang muncul, tidak semua dungeon yang aktif. namun, hanya Dungeon yang di list paling awal.

![Screenshot 2025-05-03 142427](https://github.com/user-attachments/assets/a8c10b08-9771-486e-a17c-a62a498f3cb1)
