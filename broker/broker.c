#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "comun.h"
#include "comun.c"
#include "diccionario.h"
#include "cola.h"

#define TAM 1024

struct iovec iov_ret[1];
int ret_err = -1;
int ret_ok = 0;

void read_error(int s, int s_conec)
{
    /* Enviamos iovec de error al cliente */
    iov_ret[0].iov_base = &ret_err;
    iov_ret[0].iov_len = sizeof(ret_err);
    writev(s_conec, iov_ret, 1);
    /* Cerramos descriptores */
    close(s);
    close(s_conec);
}

void return_ok(int s_conec)
{
    iov_ret[0].iov_base = &ret_ok;
    iov_ret[0].iov_len = sizeof(ret_ok);
    writev(s_conec, iov_ret, 1);
}

void return_err(int s_conec)
{
    iov_ret[0].iov_base = &ret_err;
    iov_ret[0].iov_len = sizeof(ret_err);
    writev(s_conec, iov_ret, 1);
}

void free_cola(char *clave, void *valor)
{
    free(clave);
    struct cola *aux_queue = valor;
    cola_destroy(aux_queue, NULL);
}

struct message
{
    void *content;
    int size;
};

void print_queue(void *value)
{
    struct message *msg = value;
    printf("Contenido de la cola:\n");
    printf("%s\n", (char *)msg->content);
}

int main(int argc, char *argv[])
{
    int s, s_conec, leido, api_err;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    int opcion = 1;

    int c_size, op;
    char *cola_name;
    char op_str[2];

    struct diccionario *dict;
    struct cola *queue;

    struct message *msg;
    void *msg_p;

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
        return 1;
    }
    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("error creando socket");
        return 1;
    }
    /* Para reutilizar puerto inmediatamente */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion)) < 0)
    {
        perror("error en setsockopt");
        return 1;
    }
    dir.sin_addr.s_addr = INADDR_ANY;
    dir.sin_port = htons(atoi(argv[1]));
    dir.sin_family = PF_INET;
    printf("Servidor conectado al puerto: %d\n", dir.sin_port);

    if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0)
    {
        perror("error en bind");
        close(s);
        return 1;
    }
    if (listen(s, 5) < 0)
    {
        perror("error en listen");
        close(s);
        return 1;
    }

    /* Creacion del diccionario */
    dict = dic_create();
    if (dict == NULL)
    {
        perror("Error en la creacion del diccionario");
        close(s);
        return 1;
    }

    while (1)
    {
        tam_dir = sizeof(dir_cliente);
        if ((s_conec = accept(s, (struct sockaddr *)&dir_cliente, &tam_dir)) < 0)
        {
            perror("error en accept");
            close(s);
            /* Return 1 means: error en accept */
            return 1;
        }

        /* Leer tipo de operacion */
        if ((leido = read(s_conec, op_str, 2)) < 0)
        {
            perror("error en read");
            read_error(s, s_conec);
            /* Return 2 means: error en read */
            return 2;
        }
        op = op_str[0];

        /* Leer size del nombre de la cola */
        if ((leido = read(s_conec, &c_size, sizeof(int))) < 0)
        {
            perror("error en read");
            read_error(s, s_conec);
            /* Return 2 means: error en read */
            return 2;
        }

        /* Reservar espacio para el nombre de la cola */
        cola_name = malloc(c_size);
        /* Leer nombre de la cola */
        if (recv(s_conec, cola_name, c_size, MSG_WAITALL) < 0)
        {
            perror("error en recv");
            read_error(s, s_conec);
            /* Return 2 means: error en read */
            return 2;
        }

        printf("Ha seleccionado la operacion: %c\n", op);

        switch (op)
        {
            /* Create a new queue */
        case 'C':
            /* Creacion de la cola y add al diccionario*/
            queue = cola_create();
            if (queue == NULL)
            {
                perror("Error en la creacion de la cola");
                read_error(s, s_conec);
                return 3;
            }
            /* Insert queue en diccionario */
            if (dic_put(dict, cola_name, queue) < 0)
            {
                perror("Error insert (nombre no disponible)");
                return_err(s_conec);
                break;
            }

            printf("Cola \'%s\' creada satisfactoriamente\n\n", cola_name);
            return_ok(s_conec);
            break;

        /* Destroy an existing queue */
        case 'D':
            if (dic_remove_entry(dict, cola_name, free_cola) < 0)
            {
                perror("Error destroy (nombre not found)");
                return_err(s_conec);
                break;
            }

            printf("Cola \'%s\' destruida correctamente\n\n", cola_name);
            return_ok(s_conec);
            break;

        /* Insert string to an existing queue */
        case 'P':
            msg = malloc(sizeof(struct message));
            /* Get message size */
            if ((leido = read(s_conec, &msg->size, sizeof(int))) < 0)
            {
                perror("error en read");
                read_error(s, s_conec);
                /* Return 2 means: error en read */
                return 2;
            }
            if (msg->size == 0)
            {
                printf("El mensaje no tiene contenido, la cola no cambia\n");
                return_ok(s_conec);
                break;
            }
            /* Get queue from dict */
            queue = dic_get(dict, cola_name, &api_err);
            if (api_err < 0)
            {
                perror("Error encontrando la cola (nombre incorrecto)");
                return_err(s_conec);
                break;
            }

            /* Get message */
            msg_p = malloc(msg->size);
            if (recv(s_conec, msg_p, msg->size, MSG_WAITALL) < 0)
            {
                perror("error en recv");
                read_error(s, s_conec);
                return 2;
            }
            msg->content = msg_p;
            /* Introduce message in the selected queue */
            if (cola_push_back(queue, msg) < 0)
            {
                perror("Error introduciendo mensaje en la cola");
                return_err(s_conec);
                break;
            }
            printf("La cola \'%s\' ha sido actualizada correctamente\n\n", cola_name);
            cola_visit(queue, print_queue);
            return_ok(s_conec);
            break;

        /* Get value of an existing queue */
        default:
            msg = malloc(sizeof(struct message));
            /* Get pointer to the queue */
            queue = dic_get(dict, cola_name, &api_err);
            if (api_err < 0)
            {
                perror("Error encontrando la cola (nombre incorrecto)");
                return_err(s_conec);
                break;
            }
            /* Get pointer to the first element in the queue */
            msg = cola_pop_front(queue, &api_err);
            if (api_err < 0)
            {
                /* No element found (empty queue) */
                printf("La cola esta vacia\n");
                return_ok(s_conec);
                break;
            }
            /* Element found (pointer to message) */
            printf("contenido: %s, size: %d\n", (char *)msg->content, msg->size);
            struct iovec iov_get[2];
            int msg_size = msg->size;

            iov_get[0].iov_base = &msg_size;
            iov_get[0].iov_len = sizeof(msg_size);
            iov_get[1].iov_base = msg->content;
            iov_get[1].iov_len = msg_size;

            printf("Elemento extraido de la cola \'%s\'correctamente\n\n", cola_name);
            writev(s_conec, iov_get, 2);
            break;
        }

        close(s_conec);
    }
    close(s);
    return 0;
}
