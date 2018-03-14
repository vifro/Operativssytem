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
 *   
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

#ifndef FILENAME_D
#define FILENAME_D "kw_data.txt" //  Filename for storing key word data
#endif

#ifndef SYSLOG_FILE
#define SYSLOG_FILE "daemonlog" // Name for syslog file
#endif
  

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
 * Handling system signals
 */
void signal_handler(int sig){
    switch(sig) {
        case SIGHUP:
            log_message(FILENAME_D, "Signal - hangup");
            break;
        case SIGTERM:
            log_message(FILENAME_D, "Signal - Terminate"),
            exit(EXIT_SUCCESS);
            break;
    }
}

void write_to_file(char message[]){
	
	FILE *fp;
	int err = 0;
	char temp[6] = "hello";
	
	strcpy(temp, message);
	

	fp = fopen(FILENAME_D, "wb");
	if(fp < 0){
		log_message(SYSLOG_FILE, "Could not open file");
		exit(EXIT_FAILURE);
	}
	log_message(SYSLOG_FILE, "Before Write");
	err = fwrite(temp, sizeof(char), sizeof(temp) - 1, fp);
	if(err != sizeof(temp)){
		log_message(SYSLOG_FILE, "An error occured while writing to file");
		exit(EXIT_FAILURE);
	}
	log_message(SYSLOG_FILE, "After write");
	if(fclose(fp) != 0) {
		log_message(SYSLOG_FILE, "An error occured while closing the file");
		exit(EXIT_FAILURE);
	}
}

int i = 0; //Counter for test, needs to be removed

pid_t pid, sid; //pid and session id

/*
 * Creates a daemon
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
    
    /* Change the working directory*/
    if(chdir("/") < 0) {
        log_message(SYSLOG_FILE, "Something went wrong changing directory");
        exit(EXIT_FAILURE);
    }
    
    /* Since a terminal cant use terminal its good practice to close --> */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    /*
    for (fd = 0; fd < _NFILE; fd++)
    close(fd);  /* close all file descriptors */
    
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

int main(void){
    /*take a name for syslog or give it a default value*/
    
    create_daemon();
    
    /* Run task*/
    while(1) {
        
        if(i > 10){
            log_message(SYSLOG_FILE, "Daemon exiting from within");
            exit(EXIT_SUCCESS);
        }
        
        log_message(SYSLOG_FILE, "This message comes from daemon");
       	write_to_file("Daemon speaking");
        sleep(1);
        i++;
    }
    
    
    log_message(SYSLOG_FILE,"Daemon exiting from within, wtf");
    exit(EXIT_FAILURE);
}




