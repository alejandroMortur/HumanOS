#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Puertos I/O del bus ATA primario
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMM_STAT    0x1F7

// Comandos ATA
#define ATA_CMD_READ_SECTORS     0x20
#define ATA_CMD_WRITE_SECTORS    0x30
#define ATA_CMD_IDENTIFY         0xEC

// Flags de estado ATA
#define ATA_STAT_ERR  0x01
#define ATA_STAT_DRQ  0x08
#define ATA_STAT_SRV  0x10
#define ATA_STAT_DF   0x20
#define ATA_STAT_RDY  0x40
#define ATA_STAT_BSY  0x80

// Funciones del driver ATA PIO
void ata_init(void);
int ata_identify(void);
int ata_read_sector(uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint32_t lba, const uint8_t* buffer);
uint32_t ata_get_sector_count(void);
const char* ata_get_model(void);

#endif
