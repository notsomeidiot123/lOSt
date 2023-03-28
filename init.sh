mkdir bin
mkdir kernel/obj
mkdir drives 
cd drives
touch testfd.fd
touch testhd.hd
qemu-img resize testfd.fd 1.44M
qemu-img resize testhd.hd 512M