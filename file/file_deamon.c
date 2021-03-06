/*
 *  Background process that listens to <insert file here> and 
 *  and write to a given file
 *
 *  Author: Victor From <c13vfm@cs.umu.se>
 * 
 *  Background processes are created in 7 steps. Information on this
 *  was read from http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html. 
 *  
 *  Writing to file:  http://www.enderunix.org/docs/eng/daemon.php.
 *
 *	IMPORTANT, RUN AS ROOT   
 */
 
#include <stdlib.h>     //  exit , EXIT_FAILURE AND EXIT_SUCCESS MACROS
#include <stdio.h>      //  File input and output
#include <unistd.h>     //  fork(), pipe and io primitives 
#include <errno.h>      //  Value of last error
#include <string.h>     //  size_t and NULL 
#include <syslog.h>     //  Use for printing logs
#include <fcntl.h>      //
#include <sys/types.h>  //  off_t and pid_t
#include <sys/stat.h>   //  File type and file mode
#include <signal.h>     //  Signals
#include <poll.h>		//  Poll

/*For working in both windows and unix */
#ifdef WINDOWS
#include <direct.h>
#define getDir _getcwd
#else
#define getDir getcwd
#endif

#ifndef FILENAME_D
#define FILENAME_D "/temp/kw_saved.txt" //  Filename for storing key word data
#endif

#ifndef SYSLOG_FILE
#define SYSLOG_FILE "daemonlog" // Name for syslog file
#endif
  
#ifndef MAX_FILELEN // Name for syslog file
#define MAX_FILELEN 4096
#endif

int i = 0; //Counter for test, needs to be removed

pid_t pid, sid; //pid and session id

/* file */
struct pollfd fds;
FILE *fp;
char path[255];

/*   
 * This function reports messages to log for daemon, 
 * Usefull for information and debugging as user. 
 */
void log_message(char *filename,  const char *message)
{
    openlog(filename, LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "%s", message);
    closelog();
}

/*
 * Handling system signals. 
 */
void signal_handler(int sig){
    switch(sig) {
        case SIGHUP:
            log_message(FILENAME_D, "Signal - hangup");
            break;
        case SIGTERM:
            log_message(FILENAME_D, "Signal - Terminate"),
            fclose(fp);
            close(fds.fd);
            exit(EXIT_SUCCESS);
            break;
    }
}

/*
 * Write to file. 
 * 		Saved the message to file . If an error occured, Exit the program.
 * 		
 *		For further development. Make the check then redo the try in a 
 *		fashionable manner.
 */
void write_to_file(char *message){
	
	FILE *temp_fp;
	int err;
	
	temp_fp = fopen(path,"w+"); //open in append mode, not existing == create
	if(temp_fp == NULL){
		log_message(SYSLOG_FILE, "Could not open file:");
		log_message(SYSLOG_FILE, path);
		exit(EXIT_FAILURE);
	}
	
	log_message(SYSLOG_FILE, "Before Write");
	err = fprintf(temp_fp, "%s\n", message);
	
	if(err < 0){
		log_message(SYSLOG_FILE, "An error occured while writing to file");
		exit(EXIT_FAILURE);
	
	}
	
	log_message(SYSLOG_FILE, "After write");
	if(fclose(temp_fp) != 0) {
		log_message(SYSLOG_FILE, "An error occured while closing the file");
		exit(EXIT_FAILURE);
	}
}

/*
 * Creates a daemon, each step of the process is described in the code.
 *
 */
void create_daemon(){

    /* Fork process to create our to be daemon */
    pid = fork();
    if(pid < 0) {
        printf("Failed to fork process");
        exit(EXIT_FAILURE);
    }
    
    /* Exit parent  */
    if(pid > 0) {
        printf("%d - Parent is now exiting ..\n", getpid());
        exit(EXIT_SUCCESS);
    }

    /* [UMASK] - change permission so files created by daemon can be used  */
    umask(0); // 0 = read, write and execute

    /* Create a unique session id. */
    sid = setsid();
    if(sid < 0) {
        log_message(SYSLOG_FILE, "Error creating a unique session id");
        exit(EXIT_FAILURE);
    }
    
    /* Get directory started from*/
    if (getcwd(path, sizeof(path)) == NULL) {
   		log_message(SYSLOG_FILE, "Could`nt get current directory");
   		exit(EXIT_FAILURE);
    }
    strcat(path, FILENAME_D);
    log_message(SYSLOG_FILE, path);
    
    /* Change the working directory*/
    if(chdir("/") < 0) {
        log_message(SYSLOG_FILE, "Something went wrong changing directory");
        exit(EXIT_FAILURE);
    }
    
    /* Since a terminal cant use daemon its good practice to close --> */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    /*
   
    /* CHECK MUTUAL EXCLUTION, MAYBE BY OPENING A FILE AND CHECK THAT! */
    
    /* Ignoring signals */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    
    /* Adding signals to handler */
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    
    log_message(SYSLOG_FILE, "Setup - SUCCESS");    
}

/*
 * Creata a daemon and wait for updates in the virtual filesystem 
 * located at /sys/kernel/kobject_kw/kw_info.
 * 
 * When a change in that files content is detected, call the function
 * write_to_file for saving to disk. 
 *
 * Important that you read the whole file before waiting for a change with poll!
 * Also close and reopen the pollfd after each update.
 * Read poll.h for more information!  
 */
int main(void){
    /*Create daemon first*/
    create_daemon();
    
    char tempbuf;
    int retval;
    short revents;
    char temp_read;
    //char temp[MAX_FILELEN];
    char *tempstring;
    
    log_message(SYSLOG_FILE, "opening file");
    fds.fd = open("/sys/kernel/kobject_kw/kw_info", O_RDONLY | O_NONBLOCK);
    fds.events = POLLPRI;
    
    while(read(fds.fd, &temp_read, 1) != 0){
		  log_message(SYSLOG_FILE, "Reading file"); 		
	}
	
    /* Run task*/
    while(1) {
        
       	if(fds.fd < 0){
       		log_message(SYSLOG_FILE, "Error open syslog file");
        	close(fds.fd);
        	exit(EXIT_FAILURE);
       	}
       	
        log_message(SYSLOG_FILE, "polling");
        retval = poll(&fds, 1, -1);
        if(retval < 0 ){
        	log_message(SYSLOG_FILE, "Error poll");
        	close(fds.fd);
        	exit(EXIT_FAILURE);
        	
        } else{
		    close(fds.fd);
		    fp = fopen("/sys/kernel/kobject_kw/kw_info", "r");
		    if(fp == NULL)
		    	break;
		    fseek(fp, 0, SEEK_END);
			long fsize = ftell(fp);
			fseek(fp, 0, SEEK_SET);  //same as rewind(f);

			tempstring = malloc(fsize + 1);
			fread(tempstring, fsize, 1, fp);
		    //temp = malloc(MAX_FILELEN);
		   	//log_message(SYSLOG_FILE, "Scanning\n");
		    //fscanf(fp, "%s", temp);
            log_message(SYSLOG_FILE, tempstring);
		   	write_to_file(tempstring);
		   	free(tempstring);
		   	fclose(fp);	
		   	
		   	fds.fd = open("/sys/kernel/kobject_kw/kw_info", O_RDONLY | O_NONBLOCK);
		   		
		   	while(read(fds.fd, &temp_read, 1) != 0){
		   		
		   	}
        }
    }
    
    close(fds.fd);
    log_message(SYSLOG_FILE,"Daemon exiting from within, wtf");
    exit(EXIT_FAILURE);
}




