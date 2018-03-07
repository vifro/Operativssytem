# Operativssytem
How to test this simple implementation

1. gcc [flags] netlinkUser.c TLV/tlv.c -o testrun
2. make 
3. insmod nl_kw_modKO.ko
4. run ./user
5. dmesg || tail -f /var/log/kern.log

Remove with rmmod 
