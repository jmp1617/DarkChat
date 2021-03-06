#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <time.h>
#include <ncurses.h>
#include <sys/random.h>

#include "aes.h"

#define RPORT 8686
#define SPORT 8687
#define MAXCONN 50
#define MAXMSGLEN 256
#define MAXFILESIZE 8*1000*1000 // 100 MB

//------- Identifiers
#define ACTIVE_NODES_REQ 0xF0
#define DISCONNECT 0xF1
#define STD_MSG 0xF2
#define NODE_RES 0xF3
#define HELLO 0xF4
#define BL_UPD 0xF5
#define F_MSG 0xF6

//------- Structures
// User Input
struct arguments_s{
    char* key;
    char* node_ip;
    char* nickname;
};
typedef struct arguments_s* Arguments;

// Ip linked list
struct ip_list_s{
    uint32_t ip;
    char nick[20];
    struct ip_list_s* next;
};
typedef struct ip_list_s* IP_List;

void IPL_add(uint32_t ip, IP_List* root, char* nickname);
void IPL_print(IP_List root);
void IPL_destroy(IP_List root);
char* IPL_contains(uint32_t ip, IP_List root);
int IPL_remove(uint32_t ip, IP_List* root);

// Message List
struct message_list_s{
    char message[MAXMSGLEN];
    char nick[20];
    uint32_t time;
    struct message_list_s* next;
};
typedef struct message_list_s* MSG_List;

void MSG_add(char* message, char* nick, uint32_t time, MSG_List* messages);
void MSG_destroy(MSG_List messages);
void MSG_display(MSG_List messages);
void print_time(uint32_t* time);
void wprint_time(WINDOW* w, uint32_t* time);

// Metadata
struct metadata_s{
    int ip_count;
    int blacklist_count;
    uint32_t my_ip;
    int reciever_s;
    int sender_s;
    char nick[20];
    IP_List ip_list;
    IP_List blacklist;
    MSG_List messages;
    unsigned int lock: 2;
    unsigned int ipassive: 1;
    unsigned int emit_black: 1;
    unsigned int keyloaded: 1;
    uint8_t key[32];
    uint8_t iv[16];
    WINDOW* win;
    WINDOW* message_board;
    WINDOW* messenger;
    WINDOW* banner;
    WINDOW* status;
    WINDOW* message_sender;
    struct AES_ctx* encrypt_context;
};
typedef struct metadata_s* Metadata;

struct message_s{
    uint8_t identifier;
    int size;
    uint8_t* message;
};
typedef struct message_s* Message;

//------- encryption
void generate_key_256();
void load_key(char* key, Metadata meta);
int send_message_encrypted(Message m, int socket, Metadata meta);

//------- display
void display(Metadata meta);
void display_mb(MSG_List messages, WINDOW* mb);

//------- blacklist
void load_blacklist(IP_List* root, Metadata meta);
void dump_blacklist(IP_List root);

//------- voids
void print_usage(); // print the usage
void create_directories(); // create the .darkchat and keys if not present
void check_args(); // validate arguments
void print_ip(uint32_t ip); // print in human readable

//------- aux
uint32_t conv_ip(char* ip); // check and convert the ip

//------- socket
int init_socket();
uint32_t get_ip_of_interface(char* interface);
int send_message(Message m, int socket);

//------- threading
void* message_reciever_worker(void* arg);
void* message_sender_worker(void* arg);

//------- locks
void lock(Metadata meta);
void unlock(Metadata meta);

//------- destruction
void destructor(Arguments args, Metadata meta);
