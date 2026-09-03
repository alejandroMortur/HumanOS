#include "ata.h"
#include "vga.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("in %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("out %%al, %%dx" : : "a"(val), "d"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("in %%dx, %%ax" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("out %%ax, %%dx" : : "a"(val), "d"(port));
}

static uint32_t disk_sector_count = 0;
static char disk_model[41];
static int disk_present = 0;

static int ata_wait_bsy(void) {
    uint32_t timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
        if (status == 0xFF) return 0; // Floating bus (No disk)
        if (!(status & ATA_STAT_BSY)) return 1; // BSY cleared
    }
    return 0; // Timeout
}

static int ata_wait_drq(void) {
    uint32_t timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
        if (status == 0xFF) return 0;
        if (status & ATA_STAT_DRQ) return 1; // DRQ ready
    }
    return 0; // Timeout
}

int ata_identify(void) {
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LO, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HI, 0);
    outb(ATA_PRIMARY_COMM_STAT, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
    if (status == 0 || status == 0xFF) {
        disk_present = 0;
        return 0; // Disco no presente
    }

    if (!ata_wait_bsy()) {
        disk_present = 0;
        return 0;
    }

    if (inb(ATA_PRIMARY_LBA_MID) != 0 || inb(ATA_PRIMARY_LBA_HI) != 0) {
        disk_present = 0;
        return 0; // Dispositivo no ATA (ATAPI u otro)
    }

    if (!ata_wait_drq()) {
        disk_present = 0;
        return 0;
    }

    // Leer 256 palabras (512 bytes) de datos IDENTIFY
    uint16_t identify_buf[256];
    for (int i = 0; i < 256; i++) {
        identify_buf[i] = inw(ATA_PRIMARY_DATA);
    }

    // Extraer modelo del disco (Palabras 27-46)
    for (int i = 0; i < 20; i++) {
        uint16_t val = identify_buf[27 + i];
        disk_model[i * 2]     = (char)(val >> 8);
        disk_model[i * 2 + 1] = (char)(val & 0xFF);
    }
    disk_model[40] = '\0';

    // Extraer recuento de sectores LBA28 (Palabras 60-61)
    disk_sector_count = ((uint32_t)identify_buf[61] << 16) | identify_buf[60];
    if (disk_sector_count == 0) disk_sector_count = 10485760; // 5 GB por defecto si sin formatear
    disk_present = 1;

    return 1;
}

void ata_init(void) {
    if (ata_identify()) {
        vga_puts("[ATA] Hard Disk Detected: ", COLOR_LIGHT_GREEN);
        vga_puts(disk_model, COLOR_WHITE);
        vga_puts(" (", COLOR_WHITE);
        
        char buf[16];
        uint32_t disk_mb = (disk_sector_count / 2048);
        int count = disk_mb;
        int j = 0;
        if (count == 0) buf[j++] = '0';
        while (count > 0 && j < 15) {
            buf[j++] = '0' + (count % 10);
            count /= 10;
        }
        for (int k = j - 1; k >= 0; k--) vga_putc(buf[k], COLOR_WHITE);
        vga_puts(" MB)\n", COLOR_WHITE);
    } else {
        vga_puts("[ATA] No Primary ATA Disk Connected\n", COLOR_YELLOW);
        disk_present = 0;
    }
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!disk_present || buffer == ((void*)0)) return -1;

    if (!ata_wait_bsy()) return -1;

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMM_STAT, ATA_CMD_READ_SECTORS);

    if (!ata_wait_bsy() || !ata_wait_drq()) return -1;

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PRIMARY_DATA);
    }

    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (!disk_present || buffer == ((void*)0)) return -1;

    if (!ata_wait_bsy()) return -1;

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMM_STAT, ATA_CMD_WRITE_SECTORS);

    if (!ata_wait_bsy() || !ata_wait_drq()) return -1;

    const uint16_t* ptr = (const uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_DATA, ptr[i]);
    }

    outb(ATA_PRIMARY_COMM_STAT, 0xE7); // Cache Flush
    (void)ata_wait_bsy();

    return 0;
}

uint32_t ata_get_sector_count(void) {
    return disk_sector_count;
}

const char* ata_get_model(void) {
    return disk_model;
}
