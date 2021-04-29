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

/* Check for removal */
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
    int s, c_size;
    struct iovec iov[3];

    if ((s = connect_socket()) < 0)
        return -1;

    char *opcion = (char *)malloc(2 * sizeof(char));

    /* Set the create or destroy option value */
    opcion[0] = opc;
    opcion[1] = '\0';
    iov[0].iov_base = opcion;
    iov[0].iov_len = 2;
    /* Get the size of the queue */
    c_size = strlen(cola) + 1;
    iov[1].iov_base = &c_size;
    iov[1].iov_len = sizeof(c_size);
    /* Check that the queue name is not greater than the max allowed */
    if (c_size > NAME_SIZE)
    {
        perror("Nombre de la cola demasiado largo");
        return -1;
    }
    /* Set up the queue */
    iov[2].iov_base = cola;
    iov[2].iov_len = c_size;

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
    int s, c_size;
    struct iovec iov[5];

    /* Set the put option value */
    char opc = 'P';
    char *opcion = (char *)malloc(2 * sizeof(char));
    opcion[0] = opc;
    opcion[1] = '\0';
    iov[0].iov_base = opcion;
    iov[0].iov_len = 2;

    /* Size of the queue and the queue itself*/
    c_size = strlen(cola) + 1;
    iov[1].iov_base = &c_size;
    iov[1].iov_len = sizeof(c_size);
    /* Check that the queue name is not greater than the max allowed */
    if (c_size > NAME_SIZE)
    {
        perror("Nombre de la cola demasiado largo");
        return -1;
    }
    /* Set up the queue */
    iov[2].iov_base = cola;
    iov[2].iov_len = c_size;

    /* Size of the message and the message itself */

    return 0;
}
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking)
{
    return 0;
}
