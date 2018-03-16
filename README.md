# Operativssytem
How to test this simple implementation

1. in /user gcc [flags] netlinkUser.c TLV/tlv.c -o testrun
2. in /kernel , make 
3. in /kernel, insmod nl_kw_modKO.ko
4. in /file gcc file_deamon.c -o daemon 
5. in /file ./daemon
6. cat /var/log/syslog to check that daemon is successfully mounted. 
7. in /user ./testrun [name] [sequencenumber] -  {char*, integer} 
8. dmesg || tail -f /var/log/kern.log  to cehck
9. Check the in /file/temp to see if the values stored is saved to file. 

After run:
Remove module and kill the daemon by name. 
