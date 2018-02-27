# Operativssytem
1. gcc netlinkUser.c -o user
2. make 
3. insmod netlinkKernel.o
4. run ./user
5 dmesg || tail -f /var/log/kern.log

Remove with rmmod 
