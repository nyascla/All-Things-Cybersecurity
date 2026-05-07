#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")

#define PORT    "9000"
#define BUFSIZE 5 

/* =========================================================
   FASE 0 — Inicializar y limpiar Winsock
   ---------------------------------------------------------
   En Linux los sockets son parte del SO y no necesitan init.
   En Windows hay que arrancar la librería Winsock antes de
   usar cualquier función de red, y apagarla al terminar.
   ========================================================= */
int winsock_init(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[-] WSAStartup fallo\n");
        return 0;
    }
    return 1;
}

void winsock_cleanup(void) {
    WSACleanup();
}

/* =========================================================
   FASE 1 — Crear el socket de escucha
   ---------------------------------------------------------
   Un socket es como un "enchufe" que aún no está conectado
   a nadie. Aquí solo lo creamos y lo configuramos para TCP.

   getaddrinfo() traduce ("cualquier IP", "puerto 9000")
   a una estructura que socket()/bind() entienden, sin tener
   que rellenar structs a mano.  Soporta IPv4 e IPv6.
   ========================================================= */
SOCKET crear_socket_escucha(void) {
    struct addrinfo hints, *result;
    SOCKET sock;

    // Decimos qué tipo de conexión queremos
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;       // IPv4  (AF_INET6 para IPv6)
    hints.ai_socktype = SOCK_STREAM;   // TCP   (SOCK_DGRAM para UDP)
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;    // NULL en getaddrinfo → escuchar en todas las IPs del equipo

    // Resolvemos la dirección local: IP = cualquiera, puerto = PORT
    if (getaddrinfo(NULL, PORT, &hints, &result) != 0) {
        printf("[-] getaddrinfo fallo\n");
        return INVALID_SOCKET;
    }

    // Creamos el socket con los parámetros que nos devolvió getaddrinfo
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        printf("[-] socket() fallo: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    /* FASE 2 — Bind
       Asociamos el socket al puerto.
       Sin bind el SO no sabe en qué puerto debe escuchar. */
    if (bind(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        printf("[-] bind() fallo: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    freeaddrinfo(result); // ya no necesitamos la estructura de direcciones

    /* FASE 3 — Listen
       Ponemos el socket en modo "espera de conexiones".
       SOMAXCONN = tamaño máximo de la cola de clientes pendientes. */
    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        printf("[-] listen() fallo: %d\n", WSAGetLastError());
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

/* =========================================================
   UTILIDAD — Leer una línea completa del cliente
   ---------------------------------------------------------
   recv() devuelve los datos según llegan por la red,
   puede ser byte a byte. Esta función acumula hasta '\n'
   para que handle_client trabaje con frases completas.
   ========================================================= */
int recv_linea(SOCKET client, char *linea, int maxlen) {
    int total = 0;
    char c;
    int n;

    // while (total < maxlen - 1) {
    while (1) {
        n = recv(client, &c, 1, 0);

        if (n == 0)           return 0;  // cliente cerró la conexión
        if (n == SOCKET_ERROR) return -1; // error de red

        if (c == '\n') break;            // Enter → fin de mensaje
        if (c == '\r') continue;         // \r de Windows/telnet → ignorar

        linea[total++] = c;
    }

    linea[total] = '\0';
    return 1;
}

/* =========================================================
   Lógica de negocio — Conversación con un cliente
   ========================================================= */
void handle_client(SOCKET client) {
    char buffer[BUFSIZE];

    while (1) {
        int resultado = recv_linea(client, buffer, BUFSIZE);

        if (resultado == 0) { printf("[+] Cliente cerro conexion\n");  break; }
        if (resultado == -1){ printf("[-] Error en recv()\n");         break; }
        if (strlen(buffer) == 0) continue; // línea vacía (solo Enter)

        printf("[+] Recibido: \"%s\"\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            send(client, "Hasta luego!\n", 13, 0);
            printf("[+] Cliente envio 'exit'\n");
            break;
        }

        char respuesta[BUFSIZE];
        snprintf(respuesta, sizeof(respuesta), "Recibido: %s\n", buffer);
        send(client, respuesta, (int)strlen(respuesta), 0);
    }
}

/* =========================================================
   MAIN — Ciclo de vida del servidor
   =========================================================
   El flujo siempre es el mismo en cualquier servidor TCP:

       WSAStartup → socket → bind → listen → accept → recv/send

                              ┌─────────────────────────┐
   Arranque (una vez):        │  init → socket → bind   │
                              │       → listen           │
                              └────────────┬────────────┘
                                           │ espera
                              ┌────────────▼────────────┐
   Bucle (un cliente a la vez)│       accept()          │◄─┐
                              │    handle_client()      │  │
                              │    closesocket()        │──┘
                              └─────────────────────────┘
   ========================================================= */
int main(void) {
    if (!winsock_init()) return 1;

    // Crear socket, hacer bind y ponerlo en escucha
    SOCKET ListenSocket = crear_socket_escucha();
    if (ListenSocket == INVALID_SOCKET) {
        winsock_cleanup();
        return 1;
    }

    printf("[+] Escuchando en puerto %s...\n", PORT);

    // Bucle principal: acepta un cliente, lo atiende, repite
    while (1) {
        // accept() BLOQUEA aquí hasta que alguien se conecte
        // Devuelve un socket NUEVO exclusivo para ese cliente
        // ListenSocket sigue libre para aceptar el siguiente
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("[-] accept() fallo: %d\n", WSAGetLastError());
            break;
        }

        printf("[+] Cliente conectado.\n");
        handle_client(ClientSocket);  // toda la conversación ocurre aquí
        closesocket(ClientSocket);    // cerramos ESTE cliente (no el de escucha)
        printf("[+] Cliente desconectado, esperando otro...\n");
    }

    // Limpieza final
    closesocket(ListenSocket);
    winsock_cleanup();
    printf("[+] Servidor cerrado.\n");
    return 0;
}