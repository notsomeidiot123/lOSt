mkdir bin
mkdir kernel/obj
mkdir drives 
cd drives
touch testfd.fd
touch testhd.hd
touch data.hd
qemu-img resize testfd.fd 1.44M
qemu-img resize testhd.hd 512M

chmod 777 make
chmod 777 run