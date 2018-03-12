#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/*
 * struct about domain
 */
typedef struct URI{
    char hostname[MAXLINE];
    char path[MAXLINE];
    char port[MAXLINE];
    char buffer[MAXLINE];
} uri_t;

typedef struct cache_t {
    char hostname[MAXLINE];
    char path[MAXLINE];
    
    char data[MAX_OBJECT_SIZE];
    
    struct cache_t* next;
    int Latest_use;
    int size;
} cache_t;
/*end of struct*/

// cache list
cache_t* root_list;
int use_num = 0;
int counter_cache = 0;

/*function declaration*/
int uri_parser(uri_t* server_addr, char* uri);
void forward_req(int fd);
void header_changer(char* buf);
void* thread(void* vargp);
cache_t* findCacheby(char* hostname, char* path);
void free_cache(cache_t* cc);
/*end of function dec*/

int main(int argc, char* argv[])
{
    int listenfd;
    int* connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    strcpy(port, argv[1]);
    //Listen to allocated port
    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);     
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);   
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}

/*
 * uri parser
 * extract host name, path, port number
 */
int uri_parser(uri_t* server_addr, char* uri) {
    if (!strncmp(uri, "http://", 7)) {
        //host name
        char* uri_start = uri + 7;      //start point except http://
        char* uri_end;
        uri_end = strpbrk(uri_start, "/:");
        int len = uri_end - uri_start;
        strncpy(server_addr->hostname, uri_start, len);
        
        //port
        strcmp(server_addr->port, "80");
        if (!strncmp(uri_end, ":", 1)) {
            int portnum = atoi(uri_end + 1);
            sprintf(server_addr->port, "%d", portnum);
        }
        
        //path
        char* path_start = strchr(uri_start, '/');
        if (path_start == NULL) {
            server_addr->path[0] = '\n';
        } else {
            path_start ++;
            strcpy(server_addr->path, path_start);
        }

        return 0;
    }
    else if (!strncmp(uri, "https://", 8)) {
        //host name
        char* uri_start = uri + 8;      //start point except http://
        char* uri_end;
        uri_end = strpbrk(uri_start, "/:");
        int len = uri_end - uri_start;
        strncpy(server_addr->hostname, uri_start, len);
        
        //port
        strcmp(server_addr->port, "80");
        if (!strncmp(uri_end, ":", 1)) {
            int portnum = atoi(uri_end + 1);
            sprintf(server_addr->port, "%d", portnum);
        }
        
        //path
        char* path_start = strchr(uri_start, '/');
        if (path_start == NULL) {
            server_addr->path[0] = '\n';
        } else {
            path_start ++;
            strcpy(server_addr->path, path_start);
        }        

        return 0;
    }


    printf("URI is not invalid\n");
    return -1;
}

/*
 * Forward client's request to server if valid
 * and forward server's response and content to client
 */
void forward_req(int fd) {

        cache_t* look = root_list;
        printf("\nCache list~\n");
        while (look != NULL) {
            printf("cached!! host : %s\n", look->hostname);
            look = look->next;
        }
        printf("count : %d\n", counter_cache);
        printf("Cache list~\n");

    rio_t rio, rio_recv_server;
    uri_t server_addr;
    char buf[MAXBUF], method[MAXLINE], uri[MAXLINE], version[MAXLINE], errMessage[MAXLINE];
    int clientfd, tot_size, length;
    char* hostname;

    //initialize rio and associate fd
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) {
        return;
    }

    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    //check method
    if (strcmp(method, "GET")) {
        printf("Proxy does not implement this method\n");
        strcmp(errMessage, "Please input valid header");
        Rio_writen(fd, errMessage, (size_t)strlen(errMessage));
        return;
    }

    //check if uri is valid
    if (!uri) {
        printf("cannot find host\n");
        strcmp(errMessage, "Please input valid header");
        Rio_writen(fd, errMessage, (size_t)strlen(errMessage));
        return;
    }
    
    //parse uri
    if (uri_parser(&server_addr, uri)) {
        printf("error\n");
        strcmp(errMessage, "Please input valid header");
        Rio_writen(fd, errMessage, (size_t)strlen(errMessage));
        return;
    }

    //if there is cached obj?
    cache_t* temp_cache;
    if (temp_cache = findCacheby(server_addr.hostname, server_addr.path)) {
        Rio_writen(fd, temp_cache->data, temp_cache->size);

        printf("cache call!!\n");

        return;
    }

    clientfd = Open_clientfd(server_addr.hostname, server_addr.port);
    
    // //for test
    // clientfd = Open_clientfd("localhost", "8089");    
    
    //header setting
    sprintf(buf, "GET /%s HTTP/1.0\r\n", server_addr.path);
    sprintf(buf, "%sHost: www.cmu.edu\r\n", buf);
    sprintf(buf, "%sUser-Agent: %s", buf, user_agent_hdr);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);

    // header_changer(buf);

    //send header
    Rio_writen(clientfd, buf, strlen(buf));

    //c1ache generate
    if (!findCacheby(server_addr.hostname, server_addr.path)) {
        if (counter_cache < 11) {
            temp_cache = (cache_t *)malloc(sizeof(cache_t));

            temp_cache->next = root_list;
            strcpy(temp_cache->hostname, server_addr.hostname);
            strcpy(temp_cache->path, server_addr.path);

            root_list = temp_cache;
            counter_cache++;
        } else {
            cache_t* cur = root_list;
            cache_t* temp = cur;
            int small = cur->Latest_use;

            while (cur != NULL) {
                if (cur->Latest_use < small) {
                    temp = cur;
                }
                cur = cur->next;
            }

            free_cache(temp);

            temp_cache = (cache_t *)malloc(sizeof(cache_t));

            temp_cache->next = root_list;
            strcpy(temp_cache->hostname, server_addr.hostname);
            strcpy(temp_cache->path, server_addr.path);   
            root_list = temp_cache;
        }
    }
    //read server's response
    Rio_readinitb(&rio_recv_server, clientfd);

    tot_size = 0;
    //Read header and send to client
    printf("proxy send client : \n");
    char obj[MAX_OBJECT_SIZE];
    Rio_readlineb(&rio_recv_server, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        if (!strncmp(buf, "Content-length", 14)) {
            length = atoi(buf + 16);
        }
        Rio_writen(fd, buf, strlen(buf));
        strcat(temp_cache->data, buf);
        tot_size += strlen(buf);
        printf("%s", buf);
        Rio_readlineb(&rio_recv_server, buf, MAXLINE);
    }
    Rio_writen(fd, buf, strlen(buf));
    strcat(temp_cache->data, buf);
    tot_size += strlen(buf);
    printf("%s\n", buf);

    //read body(content) and send to client
    int n = Rio_readnb(&rio_recv_server, obj, MAX_OBJECT_SIZE);
    //printf("strlength is : %d, length is : %d, rio read : %d\n", strlen(obj), length, n);
    Rio_writen(fd, obj, length);
    strcat(temp_cache->data, obj);
    tot_size += length;

    // /* simply sending line..but too long problem in jpg transfer */
    // int n;
    // while (n = Rio_readlineb(&rio_recv_server, buf, MAXLINE) != 0) {
    //     Rio_writen(fd, buf, strlen(buf));
    // }
    temp_cache->size = tot_size;

    Close(clientfd);

    return;
}

/* 
 * thread process each request
 */
void* thread(void* vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    
    forward_req(connfd);
    Close(connfd);
    return;
}

void free_cache(cache_t* cc) {
    cache_t* cur = root_list;
    
    while (cur != NULL) {
        if (cur->next == cc) {
            break;
        }
        cur = cur->next;
    }

    cur->next = cc->next;
    
    free(cc);

    return;
}

cache_t* findCacheby(char* hostname, char* path) {
    cache_t* cur = root_list;
    
    while (cur != NULL) {
        if (!strcmp(cur->hostname, hostname) && !strcmp(cur->path, path)) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

/* 여유가 되면 구현합시다.. 
 * 안하는 것으로 하겠습니다...
 */
// void header_changer(char* buf) {
//     char* cp, * head;
//     size_t size;
//     size = strlen(buf) > strlen(user_agent_hdr) ? strlen(buf)
//                                                 : strlen(user_agent_hdr);
//     cp = (char*) Malloc(size+1);
//     head = (char*) Malloc(51);
//     strcpy(cp, buf);

//     strcpy(head, strtok(buf, ":"));
//     if(strcmp(head, "User-Agent") == 0)
//         strcpy(cp, user_agent_hdr); 
//     if(strcmp(head, "Host") == 0) 
//         strcpy(cp, "www.cmu.edu");
//     if(strcmp(head, "Connection") == 0) 
//         strcpy(cp, "close");
//     if(strcmp(head, "Proxy-Connection") == 0) 
//         strcpy(cp, "close");

//     strcpy(buf, cp);
//     Free(head);
//     Free(cp);
//     return;
// }