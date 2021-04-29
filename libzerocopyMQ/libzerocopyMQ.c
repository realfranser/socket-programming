#include <stdint.h>
#include "zerocopyMQ.h"
#include "comun.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char *pchar;

/* Connects to socket using env variables (used by all 4 options) */
int connect_socket()
{

    int s;
    struct hostent *host_info;
    struct sockaddr_in dir;

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }
    host_info = gethostbyname(getenv("BROKER_HOST"));
    if (host_info == NULL)
    {
        perror("error en el nombre del host");
        return -1;
    }

    dir.sin_addr = *(struct in_addr *)host_info->h_addr_list[0];
    dir.sin_family = PF_INET;
    dir.sin_port = htons(atoi(getenv("BROKER_PORT")));

    if (connect(s, (struct sockaddr *)&dir, sizeof(dir)) < 0)
    {
        perror("error en conect");
        close(s);
        return -1;
    }

    return s;
}

char *reverseString(char *s, int l)
{
    int i;
    char *r;
    r = (char *)malloc((l + 2) * sizeof(char));
    for (i = 0; i < l; i++)
    {
        r[(l - (i + 1))] = s[i];
    }
    r[l] = '\0';
    return r;
}

char *intToString(long n)
{
    char *s;
    s = (char *)malloc(sizeof(char));
    int i = 0;
    s[i] = (char)((n % 10) + 48);
    while (n / 10 > 0)
    {
        i++;
        s = (char *)realloc(s, (i + 2) * sizeof(char));
        n /= 10;
        s[i] = (char)((n % 10) + 48);
    }
    s = reverseString(s, (i + 1));
    s[(i + 1)] = '\0';
    return s;
}

int wait_response(int fd)
{
    /* Note: hacer switch para printear el motivo del error */
    int res, leido;
    if ((leido = read(fd, &res, sizeof(res))) < 0)
    {
        perror("error en read");
        close(fd);
        return -1;
    }
    close(fd);
    return res;
}

int create_destroy(const char *cola, char opc)
{
    int s = connect_socket();
    int respuesta;
    char *opcion = (char *)malloc(2 * sizeof(char));
    char *sizeof_cola_s = intToString(strlen(cola));
    opcion[0] = opc;
    opcion[1] = '\0';
    ssize_t nwritten;
    struct iovec iov[3];
    iov[0].iov_base = opcion;
    iov[0].iov_len = 2;
    iov[1].iov_base = sizeof_cola_s;
    iov[1].iov_len = SIZE_INT;
    int c_size = atoi(sizeof_cola_s) + 1;
    /* Puede que falle ya que solo coge 10 primeros caracteres */
    if (c_size > NAME_SIZE)
    {
        perror("Nombre de la cola demasiado largo");
        return -1;
    }
    iov[2].iov_base = cola;
    iov[2].iov_len = atoi(sizeof_cola_s) + 1;

    writev(s, iov, 3);

    return wait_response(s);
}
int createMQ(const char *cola)
{
    return create_destroy(cola, 'C');
}

int destroyMQ(const char *cola)
{
    return create_destroy(cola, 'D');
}
int put(const char *cola, const void *mensaje, uint32_t tam)
{
    return 0;
}
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking)
{
    return 0;
}
