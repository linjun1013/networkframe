

#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include"util.h"

int lockSingleInstance(const char* lock_path)
{   
    int fd = open(lock_path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        return 0;
     }
              
     struct flock lock;
     memset(&lock, 0, sizeof(lock));
     lock.l_type = F_WRLCK;
     lock.l_whence = SEEK_SET;
                                        
     if (fcntl(fd, F_SETLK, &lock) < 0){
        close(fd);
        return 0;
     }
                                                                           
     return 1;
}
