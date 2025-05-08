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
### Oleh: Ni'mah Fauziyyah A

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
- Format log: `[ROLE][YYYY-MM-DD hh:mm:ss]: [ACTION] [file]`
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

---

## Soal 2

## Soal 3
### Oleh: Revalina Erica Permatasari
#### a. Entering the dungeon
dungeon.c akan bekerja sebagai server yang dimana client (player.c) dapat terhubung melalui RPC. dungeon.c akan memproses segala perintah yang dikirim oleh player.c. Lebih dari 1 client dapat mengakses server.
##### Code

##### Output

#### b. Sightseeing 
Anda melihat disekitar dungeon dan menemukan beberapa hal yang menarik seperti toko senjata dan pintu dengan aura yang cukup seram. Ketika player.c dijalankan, ia akan terhubung ke dungeon.c dan menampilkan sebuah main menu.
##### Code

##### Output

#### c. Status Check
Melihat bahwa terdapat sebuah toko senjata, anda mengecek status diri anda dengan harapan anda masih memiliki sisa uang untuk membeli senjata. Jika opsi Show Player Stats dipilih, maka program akan menunjukan Uang yang dimiliki (Jumlah dibebaskan), senjata yang sedang digunakan, Base Damage, dan jumlah musuh yang telah dimusnahkan. 
##### Code

##### Output

#### d. Weapon Shop
Ternyata anda memiliki sisa uang dan langsung pergi ke toko senjata tersebut untuk membeli senjata. Terdapat 5 pilihan senjata di toko tersebut dan beberapa dari mereka memiliki passive yang unik. Disaat opsi Shop dipilih, program akan menunjukan senjata apa saja yang dapat dibeli beserta harga, damage, dan juga passive (jika ada). List senjata yang ada dan dapat dibeli beserta logic/command untuk membeli senjata tersebut diletakan di code shop.c yang nanti akan dipakai oleh dungeon.c.
##### Code

##### Output

#### e. Handy Inventory
Setelah membeli senjata di toko tadi, anda membuka ransel anda untuk memakai senjata tersebut. Jika opsi View Inventory dipilih, program akan menunjukan senjata apa saja yang dimiliki dan dapat dipakai (jika senjata memiliki passive, tunjukan juga passive tersebut). Lalu apabila opsi Show Player Stats dipilih saat menggunakan weapon maka Base Damage player akan berubah dan jika memiliki passive, maka akan ada status tambahan yaitu Passive.
##### Code

##### Output

#### f.	Enemy Encounter
Anda sekarang sudah siap untuk melewati pintu yang seram tadi, disaat anda memasuki pintu tersebut, anda langsung ditemui oleh sebuah musuh yang bukan sebuah manusia. Dengan tekad yang bulat, anda melawan musuh tersebut. Saat opsi Battle Mode dipilih, program akan menunjukan health-bar musuh serta angka yang menunjukan berapa darah musuh tersebut dan menunggu input dengan opsi attack untuk melakukan sebuah serangan dan juga exit untuk keluar dari Battle Mode. Apabila darah musuh berkurang, maka health-bar musuh akan berkurang juga. Jika darah musuh sudah 0, maka program akan menunjukan rewards berupa berapa banyak gold yang didapatkan lalu akan muncul musuh lagi.
##### Code

##### Output

#### g. Other Battle Logic
-	**Health & Rewards**
Untuk darah musuh, seberapa banyak darah yang mereka punya dibuat secara random, contoh: 50-200 HP. Lakukan hal yang sama untuk rewards. 
##### Code

##### Output

-	**Damage Equation**
Untuk damage, gunakan base damage sebagai kerangka awal dan tambahkan rumus damage apapun (dibebaskan, yang pasti perlu random number agar hasil damage bervariasi). Lalu buatlah logic agar setiap serangan memiliki kesempatan untuk Critical yang membuat damage anda 2x lebih besar.
##### Code

##### Output

-	**Passive**
Jika senjata yang dipakai memiliki Passive setiap kali passive tersebut menyala, maka tunjukan bahwa passive tersebut aktif.
##### Code

##### Output

#### h. Error Handling
Berikan error handling untuk opsi-opsi yang tidak ada.
##### Code

##### Output

## Soal 4
### Oleh: Revalina Erica Permatasari
#### a. Sung Jin Woo memutuskan untuk membuat dua file, yaitu system.c dan hunter.c. Sung Jin Woo mendapat peringatan bahwa system.c merupakan shared memory utama yang mengelola shared memory hunter-hunter dari hunter.c. Untuk mempermudahkan pekerjaannya, Sung Jin Woo mendapat sebuah clue yang dapat membuat pekerjaannya menjadi lebih mudah dan efisien. NOTE : hunter bisa dijalankan ketika sistem sudah dijalankan.
##### Code

##### Output

Hunter tidak bisa dijalankan sebelum sistem dijalankan

<img width="548" alt="image" src="https://github.com/user-attachments/assets/8e02f0ce-f479-4978-b002-b37c6fa15252" />

Sistem dijalankan

<img width="480" alt="image" src="https://github.com/user-attachments/assets/ced58d52-31a3-4e1a-b5b1-d1653eabc198" />

Hunter dapat dijalankan

<img width="365" alt="image" src="https://github.com/user-attachments/assets/b97e1417-cb84-434f-9151-1ecd6e13dca1" />

#### b. Untuk memastikan keteraturan sistem, Sung Jin Woo memutuskan untuk membuat fitur registrasi dan login di program hunter. Setiap hunter akan memiliki key unik dan stats awal (Level=1, EXP=0, ATK=10, HP=100, DEF=5). Data hunter disimpan dalam shared memory tersendiri yang terhubung dengan sistem.
##### Code

##### Output
<img width="365" alt="image" src="https://github.com/user-attachments/assets/89c8947b-27a1-4d90-8046-43c2ca7df01f" />

<img width="470" alt="image" src="https://github.com/user-attachments/assets/f444ad78-6821-43a2-9d2f-659af28d134a" />

#### c. Sung Jin Woo menambahkan fitur di sistem yang dapat menampilkan informasi semua hunter yang terdaftar, termasuk nama hunter, level, exp, atk, hp, def, dan status (banned atau tidak). Ini membuat dia dapat melihat siapa hunter terkuat dan siapa yang mungkin melakukan kecurangan.
##### Code

##### Output

#### d. Sung jin Woo memutuskan untuk membuat fitur unik dalam sistem yang dapat menghasilkan dungeon secara random dengan nama, level minimal hunter, dan stat rewards dengan nilai:
-	🏆Level Minimal : 1 - 5
-	⚔️ATK : 100 - 150 Poin
-	❤️HP  : 50 - 100 Poin
-	🛡️DEF : 25 - 50 Poin
-	🌟EXP : 150 - 300 Poin
Setiap dungeon akan disimpan dalam shared memory sendiri yang berbeda dan dapat diakses oleh hunter.
##### Code

##### Output

#### e. Sung Jin Woo menambahkan fitur yang menampilkan informasi detail semua dungeon. Fitur ini menampilkan daftar lengkap dungeon beserta nama, level minimum, reward (EXP, ATK, HP, DEF), dan key unik untuk masing-masing dungeon.
##### Code

##### Output

#### f. Pada saat yang sama, dungeon yang dibuat oleh sistem juga harus dapat diakses oleh hunter. Sung Jin Woo menambahkan fitur yang menampilkan semua dungeon yang tersedia sesuai dengan level hunter. Disini, hunter hanya dapat menampilkan dungeon dengan level minimum yang sesuai dengan level mereka.
##### Code

##### Output

#### g. Sung Jin Woo memutuskan untuk menambahkan fitur untuk menguasai dungeon. Ketika hunter berhasil menaklukan sebuah dungeon, dungeon tersebut akan menghilang dari sistem dan hunter akan mendapatkan stat rewards dari dungeon. Jika exp hunter mencapai 500, mereka akan naik level dan exp kembali ke 0.
##### Code

##### Output

#### h. Sung Jin Woo mengimplementasikan fitur dimana hunter dapat memilih untuk bertarung dengan hunter lain. Tingkat kekuatan seorang hunter bisa dihitung melalui total stats yang dimiliki hunter tersebut (ATK+HP+DEF). Jika hunter menang, maka hunter tersebut akan mendapatkan semua stats milik lawan dan lawannya akan terhapus dari sistem. Jika kalah, hunter tersebutlah yang akan dihapus dari sistem dan semua statsnya akan diberikan kepada hunter yang dilawannya.
##### Code

##### Output

#### i. Saat sedang memonitoring sistem, Sung Jin Woo melihat beberapa hunter melakukan kecurangan di dalam sistem. Ia menambahkan fitur di sistem yang membuat dia dapat melarang hunter tertentu untuk melakukan raid atau battle. Karena masa percobaan tak bisa berlangsung selamanya 😇, Sung Jin Woo pun juga menambahkan konfigurasi agar fiturnya dapat memperbolehkan hunter itu melakukan raid atau battle lagi. 
##### Code

##### Output

#### j. Setelah beberapa pertimbangan, untuk memberikan kesempatan kedua bagi hunter yang ingin bertobat dan memulai dari awal, Sung Jin Woo juga menambahkan fitur di sistem yang membuat dia bisa mengembalikan stats hunter tertentu ke nilai awal. 
##### Code

##### Output

#### k.	Untuk membuat sistem lebih menarik dan tidak membosankan, Sung Jin Woo menambahkan fitur notifikasi dungeon di setiap hunter. Saat diaktifkan, akan muncul informasi tentang semua dungeon yang terbuka dan akan terus berganti setiap 3 detik.
##### Code

##### Output

#### l. Untuk menambah keamanan sistem agar data hunter tidak bocor, Sung Jin Woo melakukan konfigurasi agar setiap kali sistem dimatikan, maka semua shared memory yang sedang berjalan juga akan ikut terhapus. 
##### Code

##### Output

#### Kendala
##### 1. Hunter masih bisa diakses sebelum system diakses

![Screenshot 2025-05-03 075250](https://github.com/user-attachments/assets/d064485a-5e6c-42a3-a6f3-8f382f7f4688)

##### 2. Notif yang muncul, tidak semua dungeon yang aktif. namun, hanya Dungeon yang di list paling awal.

![Screenshot 2025-05-03 142427](https://github.com/user-attachments/assets/a8c10b08-9771-486e-a17c-a62a498f3cb1)
