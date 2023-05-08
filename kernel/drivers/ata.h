extern int ata_identify();
//100% refactor later please for the love of god ata.c is so messy, this is what i get from using code i wrote 6+ months ago
extern int ata_read28(unsigned short *dest, unsigned int lba_addr, unsigned char drive_no, char sector_count);
extern int ata_write28(unsigned short *src, unsigned int lba_addr, unsigned char drive_no, char sector_count);