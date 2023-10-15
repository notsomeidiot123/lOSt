# Overview 

The storage system in lOSt is designed to be pretty modular at compile-time, with few changes needed to be made in order to allow the changing or addition of new drivers. Unfortunately, at this time, this modularity does not continue to post-compilation, so while it's a great toy OS for the technical type (those who use Arch Linux, for example), others will need to either learn just a bit of C to be able to import the correct drivers, or will just need to roll with the default drivers. 

Side note, kind of bragging here, but my code is pretty bug-less afaik, but if you do find a bug, please let me know, by submitting an issue on [github](https://github.com/notsomeidiot123/lost)!
## Modularity

### Changing a driver

All that's needed to change a driver, is to follow a few simple steps:

1. Ensure that the drive is being detected sometime after the initial boot process (during or after the main() in kernel/kernel.c is called)
2. Replace the function calls for the respective driver in read_from_drive and write_to_drive with the drivers you want to use, or read_file/write_file if you are exchanging filesystem drivers

### Adding a new driver

Adding a new driver is almost as simple as changing out drivers

1. In the Filesystem_Types or Drive_Types enums in kernel/drivers/storage.h, add in the identifier for your driver (none of the types are hard-coded and depend on their specific position)
2. Only if your driver relates to hardware, not filesystems, add in your 6-character string identifer in the drive_type_strs array, and your two-letter shorthand identifier in the disk_types_ids array, to avoid (representational, not logical) bugs, add them at the same position as in step 2
3. Import the library or file that contains your code, or the predefs for your code.
4. Add in a case for each new driver in read_from_drive, and write_from_drive, or alternatively, read_file/write_file if you are adding filesystem drivers
5. profit???

## Warnings

Currently planning on replacing the VFS layer sometime after 1.0.0 launch from the DOS style drive lettering to a linux style mount point. This WILL break compatibility with almost all software that uses absolute addressing. To help minimize issues, I'll at least temporarily support BOTH indexed drive accessing and mount points.

```c
// I plan to allow something like this, either accessing the filesystem through the mountpoint OR through the index
//Index:
fread(fopen("A:/Directory/File.txt", "w+"), malloc(512), 512);
//on the actual driver layer, this file path will be translated to 
// /Directory/File.txt
//Now, let's say we have a second drive mounted at /mnt/b and we try to access a file in a directory inside of there
//we could access it with
fread(fopen("B:/Directory/File.txt", "w+"), malloc(512), 512);
//which will translate to /mnt/b/Directory/File.txt
//OR we can access it as
fread(fopen("/mnt/b/Directory/File.txt", "w+"), malloc(512), 512);
//And both will access the same file
```