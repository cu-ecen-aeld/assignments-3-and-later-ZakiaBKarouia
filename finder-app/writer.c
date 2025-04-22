#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>


int main(int argc, char *argv[]){

    openlog("writer", LOG_PID, LOG_USER);

    if(argc != 3){
        syslog(LOG_ERR, "Invalid number of arguments: expected 2, got %d", argc - 1);
        fprintf(stderr, "Usage: %s <writefile> <writestr>\n", argv[0]);
        closelog();
        return 1;
    }

    const char *filepath = argv[1];
    const char *text = argv[2];

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file '%s': %s", filepath, strerror(errno));
        closelog();
        return 1;
    }
    ssize_t bytes_written = write(fd, text, strlen(text));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to file '%s': %s", filepath, strerror(errno));
        close(fd);
        closelog();
        return 1;

    }
    close(fd);

    syslog(LOG_DEBUG, "writing '%s' to '%s'", text, filepath);
    closelog();
    return 0 ; 
}