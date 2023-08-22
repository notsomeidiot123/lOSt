#pragma once
//returns lenght of string
extern int kstrlen(char *s);
//returns 0 on success
extern int kstrcmp(char *s, char *d);
//sets bytes amount of shorts in d to val
extern void kmemset(short *d, int bytes, int val);
//copies bytes memory from s to d
extern void kmemcpy(char *s, char *d, int bytes);
//converts a number to string representation using base base, with padding padding
extern char *ltostr(int number, int base, int padding);
//sets count memory to 0 in ptr
extern void clearmem(void *ptr, int count);
//concatenates two strings together in dest
extern void kstrcat(char *dest, char *first, char *second);
//returns 0 on success
extern int kmemcmp(char *s, char *c, int size);
//Return 0 when there are no more tokens
//Otherwise, returns string split by delimiters
extern char *kstrtok(char *to_tok, char *delim);
/*
Replaces formatted string {format} with provided arguments
WARNING: THE CALLER IS RESPONSIBLE FOR FREEING MEMORY ALLOCATED BY THIS 
FUCNTION.
*/
extern char *ksprintf(char *format, ...);