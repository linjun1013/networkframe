#include<stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>


#ifdef CONFIG_SSL
#include <openssl/ssl.h>
#endif

int main(void)
{

#ifdef CONFIG_SSL
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    const SSL_METHOD *meth = SSLv23_client_method();
    SSL_CTX* ctx = SSL_CTX_new(meth);
#endif
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
    int fd[1000] = {0};
    int ret = 0;
    for(int i = 0; i < 200; i++)
    {   
        fd[i] = socket(AF_INET, SOCK_STREAM, 0);
        if(connect(fd[i], (sockaddr*)&sin, sizeof(sin)) == -1 )
        {
            printf("fd test --- :%d failed \n", i);
        }else{
#ifdef CONFIG_SSL
            SSL*ssl = SSL_new(ctx);
            SSL_set_fd(ssl, fd[i]);
            SSL_connect(ssl);
            SSL_write(ssl, "111", 3);
            SSL_free(ssl);
#else
            write(fd[i], "111", 3);
#endif
            ret++;
        }
        
    
    }

#ifdef CONFIG_SSL    
    SSL_CTX_free(ctx);
#endif

    printf("fd test cnt:%d\n", ret);
    sleep(10);


    for(int i = 0; i < ret; i++)
    {
        shutdown(fd[i], SHUT_RDWR);
        close(fd[i]);
    }

    while(1);

}
