inline void lba_to_chs_floppy(int lba, int *cyl, int *head, int *sector){
    *cyl = lba/ (2 * 18);
    *head = ((lba % ( 2 * 18)) / 18);
    *sector = ((lba % (2 * 18)) % 18 + 1);
}