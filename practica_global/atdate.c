#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define SERVER_MODE 0
#define CLIENT_TCP_MODE 1
#define CLIENT_UDP_MODE 2
#define DEFAULT_PORT 37
#define DEFAULT_MODE "cu"
#define DEBUG(d) if(debug_mode==1) {printf("[DEBUG %d] ", getpid()); printf(d); printf("\n");}
#define ERROR(e) printf("[ERROR FATAL] "); printf(e); exit(-1);

int sockfd;
int acceptfd;
char debug_mode;

void signal_handler(int signal) {
    printf("[CTRL+C] Shut down...\n");
    close(sockfd);
    DEBUG("Closing socket..\n");
    close(acceptfd);
    DEBUG("Socket closed.\n");
    exit(1);

}

void sigpipe_handler(int signal) {
    int pid = getpid();
    printf("[%d] Conexion close.\n", pid);
    close(sockfd);
    exit(1);
}
int help(int argc, char *argv[]) {
    printf("\nattime can be use in three modes: client tcp, client udp and server\n");
    printf("How to use: atdate [-h serverhost] [-p port] [-m cu | ct | s ] [-d]\n");
    printf("\t[-h serverhost] select the server to connect.Just for clients.\n");
    printf("\t[-p port] select the port to connect.Port 37 by default.Just for clients.\n");
    printf("\t[-m modo] select the mode \n");
    printf("\t  ├── cu: client UDP\n");
    printf("\t  ├── ct: client TCP\n");
    printf("\t  └── s:  server\n");
    return 0;
}
void new_client (char* serverhost_str, int type_of_connection, int port){

    if (type_of_connection == CLIENT_UDP_MODE) {
        printf("Iniciando modo cliente UDP a servidor %s\n", serverhost_str);
    } else {
        printf("Iniciando modo cliente TCP a servidor %s\n", serverhost_str);
    }

    struct hostent *he;
    struct in_addr a;
    he = gethostbyname(serverhost_str);
    if (he) {
        printf("Preguntando a %s ", he->h_name);
        bcopy(he->h_addr, (char *) &a, sizeof(a));
        printf("en %s\n", inet_ntoa(a));
    }

    time_t time_int;
    uint32_t recvt;
    struct sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_addr.s_addr = inet_addr(inet_ntoa(a));
    client_address.sin_port = htons(port);
    if(type_of_connection == CLIENT_UDP_MODE) {
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    } else {
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    memset(&(client_address.sin_zero), '\0', 8);

    int result = connect(sockfd, (struct sockaddr *)&client_address, sizeof(struct sockaddr));
    if(type_of_connection == CLIENT_TCP_MODE && result == -1) {ERROR("Error al abrir conexión.")};
    if(type_of_connection == CLIENT_UDP_MODE) {
        DEBUG("Enviando datagrama vacio...");
        send(sockfd, NULL, 0, 0); //Enviamos datagrama vacío
        if(recv(sockfd, &recvt, 4, 0)<4){ERROR("Respuesta UDP correcta no recibida")};
        DEBUG("Recibida fecha..");
        time_int = ntohl(recvt) - 2208988800; //Timestamp correspondiente al 0 para Linux (70 años)
        printf("%s", ctime(&time_int));
    }else{
        while(1) {
            if(recv(sockfd, &recvt, 4, 0)!=0){
            time_int = ntohl(recvt) - 2208988800; //Timestamp correspondiente al 0 para Linux (70 años)
            printf("%s", ctime(&time_int));
            }
        }
    }
}
void server(int port) {

    struct sockaddr_in serveraddr;

    unsigned int clientaddrlen = 20; //FIXME
    time_t current_time;

    if(port==37)port=6804;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); //Escuchamos en cualquier dirección
    serveraddr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {ERROR("Error en bind")};
    if (listen(sockfd, 10) < 0) {ERROR("Error en listen")};
    printf("Servidor iniciado y escuchando en el puerto %d.\n", port);
    uint32_t send_buffer;
    while (1) {
        struct sockaddr_in clientaddr;
        acceptfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if(fork() == 0) {
            int pid = getpid();
            int ret;
            printf("[%d] [%s] Conexión abierta. Enviando informacion.\n", pid, inet_ntoa((struct in_addr)clientaddr.sin_addr));
            do{
                time(&current_time);
                send_buffer = htonl(current_time + 2208988800);
                sleep(1);
                printf("[%d] [%s] Enviando fecha.\n", pid, inet_ntoa((struct in_addr)clientaddr.sin_addr));
                ret = send(acceptfd, &send_buffer, sizeof(uint32_t), 0);
                if(debug_mode==1) printf("[DEBUG] %d bytes enviados\n", ret);
            }while( ret >0 );
            printf("[%d] Conexion cerrada!\n", pid);
            return;
        }
    }
}
int main(int argc, char *argv[]) {
    /* Check the argument */
    if (argc == 1) {
        printf("Keep Calm ¿Has mirado la wiki?.\n");
        printf("Usage: %s [-h serverhost] [-p port] [-m cu | ct | s ] [-d]\n", argv[0]);
        printf("Use --help for more info\n");
        return -1;
    }
    signal(SIGPIPE, sigpipe_handler);
    signal(SIGINT, signal_handler);

    int param_temp;
    char* server_str;
    char* port_str;
    char* mode_str = DEFAULT_MODE;
    debug_mode = 0;
    int port = DEFAULT_PORT;

    for (param_temp = 1; param_temp < argc; param_temp+=2) {
        if (memcmp(argv[param_temp], "--help",6) == 0) {
            return help(argc, argv);
        }
        if (memcmp(argv[param_temp],"-d",2) == 0) {
            debug_mode= 1;
        } else {
            if (param_temp + 2 > argc) {
                printf("ERROR: Bad parameters.\n");
                return -1;
            }
            if (memcmp(argv[param_temp],"-h",2) == 0) {
                server_str = strdup(argv[param_temp + 1]);
            } else if (memcmp(argv[param_temp],"-m",2) == 0) {
                mode_str = strdup(argv[param_temp + 1]);
            } else if (memcmp(argv[param_temp],"-p",2) == 0) {
                port_str = strdup(argv[param_temp + 1]);
                port = atoi(port_str);
            } else {
                printf("ERROR:  %s is not a correct parameter. Use --help.\n", argv[param_temp]);
                return -1;
            }
        }

    }

    if (memcmp(mode_str, "cu",2) == 0) {
        new_client(server_str, CLIENT_UDP_MODE, port);
    }else if (memcmp(mode_str, "ct",2) == 0) {
        new_client(server_str, CLIENT_TCP_MODE, port);
    }else if (memcmp(mode_str, "s",1) == 0) {
        server(port);
    }else{
        printf("Incorrect mode type.\n");
        help(argc, argv);
    }
return 0;
}
