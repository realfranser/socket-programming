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

int iov_init(const char *cola, char op, struct iovec *iov)
{
    int s, c_size;

    if ((s = connect_socket()) < 0)
        return -1;

    /* Set the option value */
    char *op_str = (char *)malloc(2 * sizeof(char));
    op_str[0] = op;
    op_str[1] = '\0';
    iov[0].iov_base = op_str;
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

    return s;
}

int createMQ(const char *cola)
{
    int s, c_size;
    struct iovec iov[3];

    if ((s = connect_socket()) < 0)
        return -1;

    /* Set the option value */
    char *op_str = (char *)malloc(2 * sizeof(char));
    op_str[0] = 'C';
    op_str[1] = '\0';
    iov[0].iov_base = op_str;
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

int destroyMQ(const char *cola)
{
    int s, c_size;
    struct iovec iov[3];

    if ((s = connect_socket()) < 0)
        return -1;

    /* Set the option value */
    char *op_str = (char *)malloc(2 * sizeof(char));
    op_str[0] = 'D';
    op_str[1] = '\0';
    iov[0].iov_base = op_str;
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

int put(const char *cola, const void *mensaje, uint32_t tam)
{
    int s, c_size;
    //ssize_t nwritten;
    struct iovec iov[5];
    char *msg;
    int msg_size = tam + 1;

    if ((s = connect_socket()) < 0)
        return -1;

    /* Set the option value */
    char *op_str = (char *)malloc(2 * sizeof(char));
    op_str[0] = 'P';
    op_str[1] = '\0';
    iov[0].iov_base = op_str;
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

    /* Check that the size of the message is not greater than the max allowed */
    if (tam > MSG_SIZE)
    {
        perror("Mensaje demasiado largo");
        return -1;
    }
    /* Size of the message and the message itself */
    iov[3].iov_base = &tam;
    iov[3].iov_len = sizeof(tam);

    if (tam == 0)
    {
        msg = (char *)malloc(msg_size * sizeof(char));
        *msg = (char *)mensaje;
        msg[tam] = '\0';
        iov[4].iov_base = msg;
        iov[4].iov_len = msg_size;
    }
    else
    {
        iov[4].iov_base = mensaje;
        iov[4].iov_len = tam;
    }

    writev(s, iov, 5);

    return wait_response(s);
}

int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking)
{
    int s, msg_size, leido, c_size;
    struct iovec iov[3];
    char buf[16];

    if ((s = connect_socket()) < 0)
        return -1;

    /* Set the option value */
    char *op_str = (char *)malloc(2 * sizeof(char));
    op_str[0] = 'G';
    op_str[1] = '\0';
    iov[0].iov_base = op_str;
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

    /* Note: hacer switch para printear el motivo del error */
    //if ((leido = read(s, &msg_size, sizeof(msg_size))) < 0)
    if ((leido = read(s, buf, 16)) < 0)
    {
        perror("error en read");
        close(s);
        return -1;
    }

    msg_size = atoi(buf);

    switch (msg_size)
    {
    case -1:
        printf("Error lectura de la cola (nombre incorrecto)\n");
        return -1;

    case 0:
        printf("Cola vacia\n");
        *tam = msg_size;
        return 0;

    default:
        printf("El mensaje tiene un size de: %s\n", buf);
        *mensaje = malloc(msg_size);
        *tam = msg_size;
        /* Hacemos la lectura del mensage */
        if ((leido = recv(s, *mensaje, msg_size, MSG_WAITALL)) < 0)
        {
            perror("error en el read");
            close(s);
            return 1;
        }

        printf("tam: %d\n", *tam);
        close(s);
        break;
    }

    return 0;
}
