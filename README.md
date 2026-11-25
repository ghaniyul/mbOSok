# mbOSok

Tugas Besar IF2130 - Sistem Operasi 2026/2027

![Credit: 朧月](mascot.jpg)

**mbOSok** adalah sistem operasi 32-bit x86 Protected Mode yang dibangun dari nol. Kernel berjalan di *higher half* dengan paging 4 KiB, menyimpan data di file system EXT2 (IF2130 Edition), dan menjalankan program di user mode (ring 3) lewat system call. Scheduler *preemptive* Round Robin membuat beberapa process bisa berjalan bersamaan, misalnya jam yang terus berdetak di pojok layar selagi shell tetap dipakai.

## Daftar Isi

1. [Authors](#authors)
2. [Video Demo](#video-demo)
3. [Fitur Utama](#fitur-utama)
4. [Struktur Repository](#struktur-repository)
5. [Requirements](#requirements)
6. [Instalasi dan Cara Menjalankan](#instalasi-dan-cara-menjalankan)
7. [Shell Commands](#shell-commands)
8. [User Programs](#user-programs)
9. [File System](#file-system)
10. [Build Commands](#build-commands)
11. [Catatan Teknis](#catatan-teknis)
12. [Progres Milestone](#progres-milestone)
13. [Maskot](#maskot)

## Authors

| Nama | NIM |
| ---- | --- |
| Nabil Rabbani | 13525004 |
| Raditya Wibian Sastaka | 13525019 |
| Mochammad Nuha Al Ghifari | 13525056 |
| Ghaniyul Amri Caulava | 13525106 |

## Video Demo

Video demo mbOSok (bonus) dapat dilihat di Google Drive berikut:

**[Folder Video Demo mbOSok](https://drive.google.com/drive/folders/1VpPre4YQPDeZfukPOFSLcRnOs5XG7J4H?usp=sharing)**

## Fitur Utama

| Fitur | Deskripsi |
| ----- | --------- |
| **32-bit Protected Mode** | GDT dengan segmen kernel, segmen user (DPL 3), dan Task State Segment |
| **Higher-Half Kernel** | Kernel di-link di `0xC0100000`, dimuat GRUB di alamat fisik `0x100000` |
| **Paging** | Page 4 KiB, recursive mapping, page frame allocator, dan address space terpisah untuk setiap process |
| **User Mode** | Program berjalan di ring 3 dan hanya bisa mengakses kernel lewat `int 0x30` |
| **Multitasking** | Scheduler preemptive Round Robin dengan timer IRQ0 1000 Hz dan context switch |
| **Process Management** | `exec`, `exit`, `kill`, `ps`, `getpid`, process foreground dan background, Ctrl+C |
| **EXT2 File System** | Disk 4 MiB, block 512 byte, folder bersarang, direct, single, dan double indirect block |
| **Text Mode** | VGA 80x25 dengan scrolling dan hardware cursor |
| **Keyboard** | Driver PS/2 dengan Shift, Caps Lock, Ctrl+C, dan ring buffer 64 karakter |
| **Real-Time Clock** | Driver CMOS untuk jam yang ditampilkan program `clock` |
| **Fault Isolation** | Page fault atau GPF di user mode hanya menghentikan process yang bersangkutan |

## Struktur Repository

```text
mbOSok/
├── .kit/                       # Template dan referensi dari modul praktikum
├── bin/                        # Output build (kernel, ISO, disk image, user program)
├── other/
│   └── grub1                   # Bootloader GRUB legacy untuk ISO
├── src/
│   ├── cpu/                    # GDT, IDT, PIC, interrupt handler, dispatcher syscall
│   │   ├── gdt.c
│   │   ├── idt.c
│   │   ├── interrupt.c
│   │   ├── intsetup.s
│   │   └── portio.c
│   ├── driver/                 # Driver perangkat
│   │   ├── cmos.c
│   │   └── keyboard.c
│   ├── external/
│   │   └── external-inserter.c # Tool di host untuk memasukkan file ke storage.bin
│   ├── filesystem/             # Driver disk ATA PIO dan EXT2
│   │   ├── disk.c
│   │   └── ext2.c
│   ├── header/                 # Seluruh header (.h), mengikuti struktur folder src/
│   ├── memory/
│   │   └── paging.c            # Paging dan page frame allocator
│   ├── process/                # Process, scheduler, dan context switch
│   │   ├── context-switch.s
│   │   ├── process.c
│   │   └── scheduler.c
│   ├── stdlib/
│   │   └── string.c            # memcpy, memset, memcmp, memmove freestanding
│   ├── text/
│   │   └── framebuffer.c       # Driver VGA text mode
│   ├── user/                   # Program user mode
│   │   ├── clock.c
│   │   ├── crt0.s
│   │   ├── echo.c
│   │   ├── hello.c
│   │   ├── readme.txt
│   │   ├── shell.c
│   │   └── user-linker.ld
│   ├── kernel-entrypoint.s     # Entry point, bootstrap paging, lompat ke higher half
│   ├── kernel.c                # Urutan inisialisasi kernel
│   ├── linker.ld
│   └── menu.lst                # Konfigurasi menu GRUB
├── JURNAL.md                   # Jurnal pengerjaan per milestone
├── Makefile
├── README.md
└── mascot.jpg
```

## Requirements

Program ini dikompilasi dan dijalankan di Linux atau **Windows Subsystem for Linux (WSL 2)** dengan paket berikut.

| Paket | Kegunaan |
| ----- | -------- |
| `nasm` | Assembler untuk file `.s` |
| `gcc`, `gcc-multilib` | Compiler C untuk target 32-bit (`-m32`) |
| `make` | Otomasi build |
| `genisoimage` | Membuat ISO bootable |
| `qemu-system-x86` | Emulator untuk menjalankan OS |
| `gdb` | Debugger (opsional) |

```bash
sudo apt update
sudo apt install -y nasm gcc gcc-multilib make genisoimage qemu-system-x86 gdb
```

## Instalasi dan Cara Menjalankan

1. Clone repository lalu masuk ke folder project.

   ```bash
   git clone https://github.com/ghaniyul/mbOSok.git
   cd mbOSok
   ```

2. Build kernel, buat disk baru, lalu masukkan semua user program ke disk.

   ```bash
   make prepare
   ```

   `make prepare` membuat ulang `bin/storage.bin`, sehingga isi disk sebelumnya hilang. Untuk build ulang kernel tanpa menyentuh disk, pakai `make build`.

3. Jalankan OS di QEMU.

   ```bash
   make run
   ```

   `make run` menjalankan QEMU dengan flag `-s -S`, sehingga QEMU menunggu debugger sebelum boot. Lanjutkan dengan konfigurasi `Kernel` di `.vscode/launch.json`, atau dari terminal lain:

   ```bash
   gdb -ex "target remote :1234" -ex "continue"
   ```

## Shell Commands

Shell adalah *init process* yang dimuat dari root directory EXT2. Ketik `help` untuk melihat daftar command.

### Navigasi dan Manajemen File

| Command | Syntax | Deskripsi |
| ------- | ------ | --------- |
| `pwd` | `pwd` | Menampilkan working directory |
| `cd` | `cd [dir\|..\|/]` | Pindah directory, mendukung path absolut dan relatif (`../a`, `/bin`) |
| `ls` | `ls [dir]` | Menampilkan isi directory saat ini atau directory lain |
| `mkdir` | `mkdir <name>` | Membuat folder baru |
| `cat` | `cat <file>` | Menampilkan isi file |
| `cp` | `cp <src> <dst>` | Menyalin file atau folder, bisa ke dalam folder tujuan |
| `mv` | `mv <src> <dst>` | Memindahkan atau mengganti nama file atau folder |
| `rm` | `rm <name>` | Menghapus file atau folder beserta isinya |

### Pencarian

| Command | Syntax | Deskripsi |
| ------- | ------ | --------- |
| `find` | `find <name>` | Mencari file atau folder di seluruh file system |
| `grep` | `grep <pattern> [file]` | Mencari pola di file, atau di output command lain lewat pipeline |

Pipeline didukung, contohnya `ls | grep txt`.

### Process Management

| Command | Syntax | Deskripsi |
| ------- | ------ | --------- |
| `exec` | `exec <file> [args] [&]` | Menjalankan program, `&` menjalankannya di background |
| `ps` | `ps` | Menampilkan PID, parent PID, state, dan nama semua process |
| `kill` | `kill <pid>` | Menghentikan process berdasarkan PID |
| Ctrl+C | | Menghentikan process yang sedang berjalan di foreground |

### Environment dan Utilitas

| Command | Syntax | Deskripsi |
| ------- | ------ | --------- |
| `env` | `env` | Menampilkan variabel `PATH` |
| `export` | `export <dir[:dir...]>` atau `export PATH=<dirs>` | Mengubah `PATH`, disimpan ke `/path.cfg` |
| `clear` | `clear` | Membersihkan layar |
| `help` | `help` | Menampilkan daftar command |

Program yang ada di `PATH` (bawaan: `/bin`) bisa dijalankan langsung dengan namanya dari folder mana pun.

```text
mbOSok/$ hello
Hello World!
mbOSok/$ clock &
[3] clock started in background
mbOSok/$ ps
PID   PPID  STATE    NAME
1     0     RUNNING  shell
3     1     READY    clock
total 2 process(es)
mbOSok/$ kill 3
killed pid 3
```

## User Programs

| Program | Lokasi | Deskripsi |
| ------- | ------ | --------- |
| `shell` | `/shell` | Init process. Dijalankan otomatis saat boot dan dihidupkan lagi oleh scheduler jika berhenti |
| `clock` | `/bin/clock` | Menampilkan jam dari CMOS di pojok kanan bawah layar, cocok dijalankan dengan `clock &` |
| `hello` | `/bin/hello` | Mencetak `Hello World!`, lalu menyapa setiap nama yang diberikan sebagai argumen |
| `echo` | `/bin/echo` | Mencetak argumen yang diberikan, contoh dari *separate shell command* |

Program tanpa `&` berjalan di foreground: shell menunggu sampai program selesai atau dihentikan dengan Ctrl+C.

## File System

Spesifikasi EXT2 (IF2130 Edition) yang dipakai mbOSok:

| Parameter | Nilai |
| --------- | ----- |
| Ukuran disk | 4 MiB (`bin/storage.bin`) |
| Ukuran block | 512 byte (8192 block) |
| Block group | 8 group, masing-masing 1024 block |
| Inode | 128 byte, 64 inode per group (total 512) |
| Root directory | inode 1 |
| Panjang nama maksimum | 255 karakter |
| Pointer data | 12 direct, 1 single indirect, 1 double indirect |

- File dan folder dibuat lewat `write()`. Folder dibuat bila `is_directory` bernilai `true` atau `buffer_size` bernilai 0, sesuai spesifikasi kit.
- Setiap folder diawali entri `.` dan `..`.
- Folder hanya bisa dihapus lewat `delete()` jika kosong. `rm` di shell menghapus isinya secara rekursif terlebih dahulu.
- Disk yang belum berisi signature mbOSok otomatis diformat saat boot.

## Build Commands

| Command | Deskripsi |
| ------- | --------- |
| `make prepare` | Clean, build ISO, buat disk baru, lalu masukkan `shell`, `readme.txt`, dan program ke `/bin` |
| `make build` | Build kernel dan ISO (`bin/OS2025.iso`) |
| `make run` | Build lalu jalankan QEMU dengan GDB server di `localhost:1234` |
| `make disk` | Membuat ulang `bin/storage.bin` kosong berukuran 4 MiB |
| `make inserter` | Build tool `inserter` untuk host |
| `make user-programs` | Build semua user program menjadi flat binary |
| `make insert-programs` | Memasukkan user program ke disk yang sudah ada |
| `make clean` | Menghapus hasil build |

## Catatan Teknis

### Memory Layout

| Alamat virtual | Isi |
| -------------- | --- |
| `0x00000000` | Awal program user (entry point `_start`) |
| `0x003F0000` - `0x003FFFFF` | Stack user (16 page), argumen `argc`/`argv` ada di puncak stack |
| `0xC0000000` - `0xC03FFFFF` | 4 MiB pertama memori fisik, berisi kernel |
| `0xC00B8000` | VGA text buffer |
| `0xC0100000` | Awal kernel (alamat fisik `0x100000`) |
| `0xFFC00000` | Akses page table lewat recursive mapping (PDE 1023) |

- Memori fisik yang dikelola: 128 MiB. 4 MiB pertama dicadangkan untuk kernel.
- Setiap process punya page directory sendiri yang berbagi mapping kernel di PDE `0x300`.
- Maksimum 16 process aktif, dengan ukuran program maksimum 1 MiB.

### Interrupt

| Vektor | Sumber | Penanganan |
| ------ | ------ | ---------- |
| `0x00` - `0x1F` | Exception CPU | Process user yang menyebabkan fault dihentikan, fault di kernel menghentikan sistem |
| `0x20` (IRQ0) | PIT timer 1000 Hz | Simpan context dan pindah ke process berikutnya |
| `0x21` (IRQ1) | Keyboard | Masukkan karakter ke ring buffer, tangani Ctrl+C |
| `0x30` | `int 0x30` (DPL 3) | System call |

### System Calls

Nomor layanan di `eax`, argumen di `ebx`, `ecx`, dan `edx`.

| No | Layanan | Argumen |
| -- | ------- | ------- |
| 0 | `read` file | `ebx` = `EXT2DriverRequest*`, `ecx` = `int8_t*` return code |
| 1 | `read_directory` | `ebx` = `EXT2DriverRequest*`, `ecx` = `int8_t*` return code |
| 2 | `write` file atau folder | `ebx` = `EXT2DriverRequest*`, `ecx` = `int8_t*` return code |
| 3 | `delete` | `ebx` = `EXT2DriverRequest*`, `ecx` = `int8_t*` return code |
| 4 | Ambil karakter keyboard | `ebx` = `char*` |
| 5 | `putchar` | `ebx` = karakter, `ecx` = warna |
| 6 | `puts` | `ebx` = string, `ecx` = panjang, `edx` = warna |
| 7 | Aktifkan keyboard | - |
| 8 | Ukuran file | `ebx` = `EXT2DriverRequest*`, `ecx` = `uint32_t*`, `edx` = `int8_t*` |
| 9 | Clear screen | - |
| 10 | `exit` | `ebx` = exit code |
| 11 | `exec` | `ebx` = path absolut, `ecx` = `argv`, `edx` = 1 untuk background. Mengembalikan PID atau -1 |
| 12 | `kill` | `ebx` = PID. Mengembalikan 1 jika berhasil |
| 13 | `ps` | `ebx` = buffer teks, `ecx` = ukuran buffer. Mengembalikan jumlah process |
| 14 | Waktu CMOS | `ebx` = `struct Time*` |
| 15 | Set posisi cursor | `ebx` = baris, `ecx` = kolom |
| 16 | Ambil posisi cursor | `ebx` = `uint8_t*` baris, `ecx` = `uint8_t*` kolom |
| 17 | `getpid` | Mengembalikan PID pemanggil |
| 18 | Tulis teks di posisi tertentu | `ebx` = teks, `ecx` = panjang, `edx` = baris \| kolom << 8 \| warna << 16 |

Semua pointer dari user dicek dengan `is_user_range()` sehingga seluruh rentangnya berada di bawah `0xC0000000`. Layanan yang tidak dikenal mengembalikan -1.

## Progres Milestone

| Milestone | Cakupan | Tag |
| --------- | ------- | --- |
| Milestone 0 | Toolchain, C kernel, linker, Makefile, GDT | `v0.1.0` |
| Milestone 1 | Framebuffer, interrupt (PIC remap, IDT, ISR), keyboard | `v0.1.1` |
| Milestone 2 | Driver disk ATA PIO dan file system EXT2 | `v0.2.0` |
| Milestone 3 | Paging, user mode, system call, shell | `v0.3.0` |
| Milestone 4 | Process, scheduler, multitasking | - |
| Bonus | `PATH` persisten dan separate shell command (`echo`) | - |

Catatan pengerjaan setiap milestone ada di [JURNAL.md](JURNAL.md).

## Maskot

![Maskot Kelompok](mascot.jpg)

Credit: 朧月
