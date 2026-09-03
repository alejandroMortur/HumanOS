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
static int use_secondary = 0; // 0 = primary, 1 = secondary
static int use_slave = 0;     // 0 = master, 1 = slave

// Helper functions to get correct port based on channel
static inline uint16_t ata_data_port(void) {
    return use_secondary ? ATA_SECONDARY_DATA : ATA_PRIMARY_DATA;
}

static inline uint16_t ata_comm_stat_port(void) {
    return use_secondary ? ATA_SECONDARY_COMM_STAT : ATA_PRIMARY_COMM_STAT;
}

static inline uint16_t ata_drive_head_port(void) {
    return use_secondary ? ATA_SECONDARY_DRIVE_HEAD : ATA_PRIMARY_DRIVE_HEAD;
}

static inline uint16_t ata_seccount_port(void) {
    return use_secondary ? ATA_SECONDARY_SECCOUNT : ATA_PRIMARY_SECCOUNT;
}

static inline uint16_t ata_lba_lo_port(void) {
    return use_secondary ? ATA_SECONDARY_LBA_LO : ATA_PRIMARY_LBA_LO;
}

static inline uint16_t ata_lba_mid_port(void) {
    return use_secondary ? ATA_SECONDARY_LBA_MID : ATA_PRIMARY_LBA_MID;
}

static inline uint16_t ata_lba_hi_port(void) {
    return use_secondary ? ATA_SECONDARY_LBA_HI : ATA_PRIMARY_LBA_HI;
}

static int ata_wait_bsy(void) {
    uint32_t timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(ata_comm_stat_port());
        if (status == 0xFF) return 0; // Floating bus (No disk)
        if (!(status & ATA_STAT_BSY)) return 1; // BSY cleared
    }
    return 0; // Timeout
}

static int ata_wait_drq(void) {
    uint32_t timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(ata_comm_stat_port());
        if (status == 0xFF) return 0;
        if (status & ATA_STAT_DRQ) return 1; // DRQ ready
    }
    return 0; // Timeout
}

static int ata_probe_drive(int bus, int drive) {
    use_secondary = bus;
    use_slave = drive;

    uint16_t dh_port = ata_drive_head_port();
    uint16_t cs_port = ata_comm_stat_port();

    // Seleccionar unidad (0xA0 = Master, 0xB0 = Slave)
    outb(dh_port, (drive == 0) ? 0xA0 : 0xB0);

    // Retardo de 400ns (4 lecturas al puerto de estado)
    for (int i = 0; i < 4; i++) {
        (void)inb(cs_port);
    }

    outb(ata_seccount_port(), 0);
    outb(ata_lba_lo_port(), 0);
    outb(ata_lba_mid_port(), 0);
    outb(ata_lba_hi_port(), 0);
    outb(cs_port, ATA_CMD_IDENTIFY);

    uint8_t status = inb(cs_port);
    if (status == 0 || status == 0xFF) {
        return 0; // Sin controlador o bus flotante
    }

    if (!ata_wait_bsy()) {
        return 0;
    }

    // Verificar si es un dispositivo ATAPI (LBA_MID != 0 o LBA_HI != 0)
    if (inb(ata_lba_mid_port()) != 0 || inb(ata_lba_hi_port()) != 0) {
        return 0; // Dispositivo no ATA (ATAPI u otro)
    }

    // Esperar a que se active DRQ o ocurra un error ERR
    uint32_t timeout = 100000;
    while (timeout-- > 0) {
        status = inb(cs_port);
        if (status & ATA_STAT_ERR) {
            return 0; // El dispositivo reportó un error
        }
        if (status & ATA_STAT_DRQ) {
            break; // Listo para leer datos de IDENTIFY
        }
    }
    if (timeout == 0 || !(status & ATA_STAT_DRQ)) {
        return 0;
    }

    // Leer 256 palabras (512 bytes) de datos IDENTIFY
    uint16_t identify_buf[256];
    for (int i = 0; i < 256; i++) {
        identify_buf[i] = inw(ata_data_port());
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
    if (disk_sector_count == 0) {
        disk_sector_count = 10485760; // 5 GB por defecto
    }

    disk_present = 1;
    return 1;
}

int ata_identify(void) {
    for (int bus = 0; bus < 2; bus++) {
        for (int drive = 0; drive < 2; drive++) {
            if (ata_probe_drive(bus, drive)) {
                return 1;
            }
        }
    }
    disk_present = 0;
    return 0;
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
        vga_puts("[ATA] No ATA Hard Disk Connected\n", COLOR_YELLOW);
        disk_present = 0;
    }
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!disk_present || buffer == ((void*)0)) return -1;

    if (!ata_wait_bsy()) return -1;

    uint8_t head_val = (use_slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
    outb(ata_drive_head_port(), head_val);
    outb(ata_seccount_port(), 1);
    outb(ata_lba_lo_port(), (uint8_t)lba);
    outb(ata_lba_mid_port(), (uint8_t)(lba >> 8));
    outb(ata_lba_hi_port(), (uint8_t)(lba >> 16));
    outb(ata_comm_stat_port(), ATA_CMD_READ_SECTORS);

    if (!ata_wait_bsy() || !ata_wait_drq()) return -1;

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ata_data_port());
    }

    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (!disk_present || buffer == ((void*)0)) return -1;

    if (!ata_wait_bsy()) return -1;

    uint8_t head_val = (use_slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
    outb(ata_drive_head_port(), head_val);
    outb(ata_seccount_port(), 1);
    outb(ata_lba_lo_port(), (uint8_t)lba);
    outb(ata_lba_mid_port(), (uint8_t)(lba >> 8));
    outb(ata_lba_hi_port(), (uint8_t)(lba >> 16));
    outb(ata_comm_stat_port(), ATA_CMD_WRITE_SECTORS);

    if (!ata_wait_bsy() || !ata_wait_drq()) return -1;

    const uint16_t* ptr = (const uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(ata_data_port(), ptr[i]);
    }

    outb(ata_comm_stat_port(), 0xE7); // Cache Flush
    (void)ata_wait_bsy();

    return 0;
}

uint32_t ata_get_sector_count(void) {
    return disk_sector_count;
}

const char* ata_get_model(void) {
    return disk_model;
}

void ata_flush(void) {
    if (!disk_present) return;
    if (!ata_wait_bsy()) return;
    outb(ata_comm_stat_port(), 0xE7); // ATA Cache Flush command
    (void)ata_wait_bsy();
}
