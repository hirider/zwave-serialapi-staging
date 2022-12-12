#include "SerialapiProcess.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/uip-udp-packet.h"
#include "net/uip-debug.h"
#include "sys/ctimer.h"
#include "SerialapiEventHandler.h"
#include <mqueue.h>
#include <sys/mt.h>
#include <pthread.h>
#include <stdlib.h>
#include <serial-line.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define MAX_MESSAGE 5
#define MAX_MSG_SIZE 256
#define BUFFER_SIZE (MAX_MSG_SIZE + 10)
#define QUEUE_PERSMISION 0660
unsigned long long buff = 0;

#define SINGLE_CHANNEL_LEN  17
#define MULTICHANNEL_LEN 23
#define LOCAL_MSG_SIZE 50
#define MIN_SOCK_MSG_SIZE 14

struct sockaddr_un addr, from;
int ret;
int len;
int fd;

#define NONBLOCKING
#define MULTICMD_SUPPORT

#ifdef PROD
#define CLIENT_SOCK_FILE "/home/leo/Device/tmp/Sapi2zw_cl.sock"
#define SERVER_SOCK_FILE "/home/leo/Device/tmp/Sapi2zw_se.sock"
char *QueueName = "/msg-queueTest";
#endif

#ifndef PROD
#define CLIENT_SOCK_FILE "/home/aura/dev/SerialGateway/socket_tester/server.sock"
#define SERVER_SOCK_FILE "/home/aura/dev/SerialGateway/socket_tester/client.sock"
char *QueueName = "/msg-queueTest";
#endif


#ifdef NONBLOCKING
struct sockaddr_un nb_addr;
struct sockaddr_un nb_from;
socklen_t nb_fromlen = sizeof(nb_from);
char nb_buf[LOCAL_MSG_SIZE];//[8192];
int nb_ret;
int nb_len;
int nb_fd;
#endif

enum Curtain_data{
    OPEN = 1,
    CLOSE = 2,
    PAUSE = 3,
};

pthread_mutex_t Wait_mtx = PTHREAD_MUTEX_INITIALIZER, Send_mtx = PTHREAD_MUTEX_INITIALIZER; 

volatile bool GW_open = true;

PROCESS(SAPI_Process, "SAPI Process");

//unsigned int atoh(ap)char *ap; : definition given by the api provider
unsigned int atoh(char *ap)
{ 
    register char *p;
    register unsigned int n;
    register int digit,lcase;

    p = ap;
    n = 0;
    while(*p == ' ' || *p == '	')
        p++;

    if(*p == '0' && ((*(p+1) == 'x') || (*(p+1) == 'X')))
        p+=2;

    while ((digit = (*p >= '0' && *p <= '9')) ||
        (lcase = (*p >= 'a' && *p <= 'f')) ||
        (*p >= 'A' && *p <= 'F')) {
        n *= 16;
        if (digit)	n += *p++ - '0';
        else if (lcase)	n += 10 + (*p++ - 'a');
        else		n += 10 + (*p++ - 'A');
    }
    return(n);
}

bool RequireNodeId(int function)
{
    //All the below enums of functions in the array need node Id to work properly 
    //hence adding this validation step to prevent unwanted edge cases

    int functionlist[] =    {SAPI_SEND_SWITCH,SAPI_SEND_LEVEL,SAPI_GET_NODE_INFO,
                            SAPI_BROADCAST_NODE_INFO,SAPI_CONFIGURE_PARAMETER,SAPI_PRIORITY_ROUTE,
                            SAPI_GET_ROUTE};

    len = sizeof(functionlist)/sizeof(functionlist[0]);

    for(int i = 0; i < len; i++){
        if(functionlist[i] == function){
         return true;
        }
    }
    return false;
}

void fetchcmdclass(SerApi_ctx_t* input_ptr){
    SAPI_datastore_supported_cmdclass_t * tmp;
    //cmd_class_t ret;

    switch(input_ptr->function){
        case SAPI_SEND_SWITCH:
        {
            tmp = SAPI_DbQuerySupportedCmdClass(input_ptr->desNodeId,COMMAND_CLASS_SWITCH_BINARY);
            if(tmp->cmd_class == COMMAND_CLASS_SWITCH_BINARY && tmp->nodeid == input_ptr->desNodeId)
            {
                input_ptr->cmd_classes.cmd_class = tmp->cmd_class;
                input_ptr->cmd_classes.cmd_function = SWITCH_BINARY_SET;

                tmp = SAPI_DbQuerySupportedCmdClass(input_ptr->desNodeId,COMMAND_CLASS_MULTI_CHANNEL_V2);
                if(tmp->cmd_class == COMMAND_CLASS_MULTI_CHANNEL_V2 && tmp->nodeid == input_ptr->desNodeId)
                {
                    input_ptr->cmd_classes.encap_cmd_class = tmp->cmd_class;
                    input_ptr->cmd_classes.encap_cmd_function = MULTI_CHANNEL_CMD_ENCAP_V2;
                    input_ptr->cmd_classes.ismultichannel = true;
                }
                else{
                    input_ptr->cmd_classes.encap_cmd_class = 0;
                    input_ptr->cmd_classes.encap_cmd_function = 0;
                    input_ptr->cmd_classes.ismultichannel = false;
                }
            }
            else{
                LOG_PRINTF("command class not supported\n");
            }

        }
        break;

        case SAPI_SEND_LEVEL:
        {
            tmp = SAPI_DbQuerySupportedCmdClass(input_ptr->desNodeId,COMMAND_CLASS_SWITCH_MULTILEVEL);
            if(tmp->cmd_class == COMMAND_CLASS_SWITCH_MULTILEVEL && tmp->nodeid == input_ptr->desNodeId)
            {
                input_ptr->cmd_classes.cmd_class = tmp->cmd_class;
                if(input_ptr->zw_data == OPEN)
                {   
                    input_ptr->cmd_classes.cmd_function= SWITCH_MULTILEVEL_SET;
                    input_ptr->state=0xff;


                }else if(input_ptr->zw_data == PAUSE)
                {
                    input_ptr->cmd_classes.cmd_function= SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE;
                    
                }else if(input_ptr->zw_data == CLOSE)
                {   
                    input_ptr->cmd_classes.cmd_function= SWITCH_MULTILEVEL_SET;
                    
                    input_ptr->state = 0x00;

                }
                //ret.cmd_function = SWITCH_MULTILEVEL_SET;

                tmp = SAPI_DbQuerySupportedCmdClass(input_ptr->desNodeId,COMMAND_CLASS_MULTI_CHANNEL_V2);
                if(tmp->cmd_class == COMMAND_CLASS_MULTI_CHANNEL_V2 && tmp->nodeid == input_ptr->desNodeId)
                {
                    input_ptr->cmd_classes.encap_cmd_class = tmp->cmd_class;
                    input_ptr->cmd_classes.encap_cmd_function = MULTI_CHANNEL_CMD_ENCAP_V2;
                    input_ptr->cmd_classes.ismultichannel = true;
                }
                else
                {   input_ptr->cmd_classes.encap_cmd_class = 0;
                    input_ptr->cmd_classes.encap_cmd_function =0;
                    input_ptr->cmd_classes.ismultichannel=false;

                }
            }
            else{
                LOG_PRINTF("command class not supported\n");
            }
        }
        break;

        default :
        {
            LOG_PRINTF("vaue function seem to be corrupted!\n");
        }
    }
}

#ifdef MULTICMD_SUPPORT
bool SAPI_ProcessIncoming(char * buff)
{
    char *p;
    char clone[LOCAL_MSG_SIZE];
    SerApi_ctx_t input = {};

    bool IsNodeValid = false;

    strncpy(clone, buff,LOCAL_MSG_SIZE);
    if (clone[0] != '\0')
    {
        p = strtok(clone, ".");
        input.function = atoh(p);

        p = strtok(NULL, ".");
        if (p != NULL)
        {
            input.desNodeId = atoh(p);
            p = strtok(NULL, ".");
            if (p != NULL)
            {
                input.desEpid = atoh(p);
                p = strtok(NULL, ".");
                if (p != NULL)
                {
                    if(input.state > 0xff){
                        ERR_PRINTF("Invalid state!\n");
                        return false;
                    }
                    input.state = atoh(p);

                    p = strtok(NULL, ".");
                    if (p != NULL)
                    {
                        if(strlen(p) > 8){
                            ERR_PRINTF("data frame cannot be larger than 4 bytes\n");
                            return false;
                        }
                        input.zw_data = atoh(p);
                    }
                }
            }
        }
    }
    else
    {
        printf("No Input\n");
        return false;
    }

    for(int i = 0; i < info.NodeCount; i++){
        if(input.desNodeId == info.NetworkInfo_Nodes[i]){
            IsNodeValid = true;
            break;
        }
    }

    if(IsNodeValid == true && (input.function == SAPI_SEND_SWITCH || input.function == SAPI_SEND_LEVEL)){

        // input.cmd_classes 
        fetchcmdclass(&input);
        SAPI_ApplicationHandler(input);
    }
    else if(IsNodeValid == false && RequireNodeId(input.function)){
        ERR_PRINTF("Cannot send command to invalid Node Id!\n");
        return false;
    }
    else{
        SAPI_ApplicationHandler(input);
    }
    return true;
}
#endif


int Sapi2Zwave_send(char * buf){

    static char sock_buff[MAX_SOCK_MSG_LEN] = "";

    size_t out_len = strlen(buf);

    if (out_len >= MAX_SOCK_MSG_LEN)
    {
        fprintf(stderr, "%s() : strlen(\"%s\") = %u\n", __func__, buf, out_len);
        fprintf(stderr, "Sapi2Zwave_send() : Message length must be less than %d", MAX_SOCK_MSG_LEN);
        return 1;
    }

    memset(sock_buff, 0, sizeof(sock_buff));

    //Make a copy of the message
    strncpy(sock_buff, buf, out_len);
    // strcat(sock_buff, "\n");

    if ((fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        return 1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    //strcpy(addr.sun_path, CLIENT_SOCK_FILE);
    //unlink(CLIENT_SOCK_FILE);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        close(fd);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCK_FILE);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect()");
        close(fd);
        return 1;
    }

    pthread_mutex_lock(&Send_mtx);
    if (send(fd, sock_buff, strlen(buf) + 1, 0) == -1) {
        perror("send()");
        pthread_mutex_unlock(&Send_mtx);
        close(fd);
        return 1;
    }
    pthread_mutex_unlock(&Send_mtx);

    close(fd);
    return 0;
    //unlink(CLIENT_SOCK_FILE);
}

void sock_listner(int signum){

//refer to https://man7.org/linux/man-pages/man7/signal-safety.7.html for signal safe functions.
//printf cannot be used in signal handler, hence using "write" at 571,576 and 582

    if(signum == SIGUSR2){
        char temp[LOCAL_MSG_SIZE];
        memset(nb_buf,0,sizeof(nb_buf));
    	if((len = recvfrom(nb_fd, nb_buf, sizeof(nb_buf), 0, (struct sockaddr *)&nb_from, &nb_fromlen)) > 0){
            int buf_len = strlen(nb_buf);

            snprintf(temp,64,"downstream [%s]\n",nb_buf);
            write(1,temp,buf_len + MIN_SOCK_MSG_SIZE);
            SAPI_ProcessIncoming(nb_buf);
    	}
        else{
            char message[] = "failed to read socket\n";
            write(1,message,sizeof(message)+1);
        }
    }
    else{
        LOG_PRINTF("Signal not recognized!");
    }
}

#ifndef PROD
void writeppid(){
    FILE *fptr;
    fptr = fopen("/home/aura/dev/SerialGateway/socket_tester/ppid.txt", "w");
    fprintf(fptr, "%d",getppid());
    fclose(fptr);
}
#endif

PROCESS_THREAD(SAPI_Process, ev, data)
{
    struct sigaction sig;
    sig.sa_handler = sock_listner;
    sig.sa_flags = SA_RESTART;

    PROCESS_BEGIN()

     SAPI_data_store_init();

    LOG_PRINTF("Serial API Contiki process started\n");

    info = SAPI_GetAllNodes(false);

    //SAPI_ProcessIncoming("1");

    pthread_t listner_td;

#ifndef NONBLOCKING
    if(pthread_create(&listner_td,NULL,SAPI_Listner,"Listner")){
        perror("Listner thread error\n");
        exit(1);
    }
#endif

#ifdef NONBLOCKING

	if ((nb_fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0) {
		perror("socket()");
		return 1;
	}

	memset(&nb_addr, 0, sizeof(nb_addr));
	nb_addr.sun_family = AF_UNIX;
	strcpy(nb_addr.sun_path, CLIENT_SOCK_FILE);
	unlink(CLIENT_SOCK_FILE);
	if (bind(nb_fd, (struct sockaddr *)&nb_addr, sizeof(nb_addr)) < 0) {
		close(nb_fd);
		perror("bind()");
		return 1;
	}

    sigaction(SIGUSR2, &sig,NULL);
#endif

#ifndef PROD
    writeppid();
#endif

    while(1) 
    {
        SerialAPI_Poll();
        //PROCESS_WAIT_EVENT();
    }

    PROCESS_END()
}