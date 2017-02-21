#ifndef DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <malloc.h>
#ifndef WIN32 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <linux/if_ether.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#endif

#define msleep(M) usleep(1000*(M))
#define min(a,b) ((a) < (b) ? (a):(b))
#define max(a,b) ((a) > (b) ? (a):(b))

#if 1 //def DEBUG
	#define LOGD(fmt, ...) do{printf("%s (%4d) DEBUG: ", __FILE__, __LINE__);printf(fmt, ## __VA_ARGS__);printf("\n");}while(0) 
	#define LOGI(fmt, ...) do{printf("%s (%4d) INFO: ", __FILE__, __LINE__);printf(fmt, ## __VA_ARGS__);printf("\n");}while(0) 
	#define LOGW(fmt, ...) do{printf("%s (%4d) WARN: ", __FILE__, __LINE__);printf(fmt, ## __VA_ARGS__);printf("\n");}while(0) 
	#define LOGE(fmt, ...) do{printf("%s (%4d) ERROR: ", __FILE__, __LINE__);printf(fmt, ## __VA_ARGS__);printf("\n");}while(0) 
#else
	#define LOGI(fmt, ...) 
	#define LOGD(fmt, ...) 
	#define LOGW(fmt, ...) 
	#define LOGE(fmt, ...) do{printf("%s (%4d) ERROR: ", __FILE__, __LINE__);printf(fmt, ## __VA_ARGS__);printf("\n");}while(0) 
#endif

#endif

