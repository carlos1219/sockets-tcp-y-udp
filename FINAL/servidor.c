/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.  
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



#define PUERTO 3800
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 512
#define MAXHOST 128


extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
 struct UDPaddr_in {
 	short 		sin_family;
 	unsigned short 	sin_port;
 	struct in_addr sin_addr;
 	char 		sin_zero[8];
 	int 		perS; //socket personal
 };


void serverTCP(int s, struct sockaddr_in peeraddr_in,struct sockaddr_in server,FILE* f);
void serverUDP(int s, char * buffer, struct UDPaddr_in clientaddr_in,struct sockaddr_in server,FILE *f);
int protocoloSMTPTCP(int s,struct sockaddr_in peeraddr_in,char* hostname,struct sockaddr_in server,FILE* f);
int protocoloSMTPUDP(int s,struct UDPaddr_in peeraddr_in,char* hostname,struct sockaddr_in server,FILE* f,int addrlen,char* iPc);
void errout(char *);		/* declare error out routine */
int cmpArroba(char *buf,char comp,int infBuf);
int cmpFinal(char *buf,char comp,int indBuf);

int sal=0,from=0,to=0;
int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */
    
    int cc;				    /* contains the number of bytes read */
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
    struct UDPaddr_in clientUDP_in;
	int addrlen;
	long timevar;
	int ret;
	FILE* fichero;
    fichero = fopen("peticiones.log","w+");

    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
    struct sigaction vec;

		/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientUDP_in, 0, sizeof(struct UDPaddr_in));

    addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
            }
            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
            }
        
		while (!FIN) {
            /* Meter en el conjunto de sockets los sockets UDP y TCP */
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
            /* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */ 
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. SeÃal recibida en elect\n "); 
                }
            }
           else { 

                /* Comprobamos si el socket seleccionado es el socket TCP */
                if (FD_ISSET(ls_TCP, &readmask)) {
                    /* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
    				/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    	close(ls_TCP); /* Close the listen socket inherited from the daemon. */
        				serverTCP(s_TCP, clientaddr_in,myaddr_in,fichero);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_TCP);
        			}
             } /* De TCP*/
        	if (FD_ISSET(s_UDP, &readmask)) {
                /* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */
          	
                cc = recvfrom(s_UDP, buffer, strlen("\r\n"), 0,
                   (struct sockaddr *)&clientaddr_in, &addrlen);
                if ( cc == -1) {
                    perror(argv[0]);
                    printf("%s: recvfrom error\n", argv[0]);
                    exit (1);
                    }
                clientUDP_in.sin_family = clientaddr_in.sin_family;
                clientUDP_in.sin_port = clientaddr_in.sin_port;
                clientUDP_in.sin_addr = clientaddr_in.sin_addr;
                strcpy(clientUDP_in.sin_zero,clientaddr_in.sin_zero);
                clientUDP_in.perS = 0;

                clientUDP_in.perS=socket (AF_INET, SOCK_DGRAM, 0);
                if(clientUDP_in.perS==-1){
                	perror(argv[0]);
                    printf("%s: unable to create socket UDP\n", argv[0]);
                    exit (1);
                }
                myaddr_in.sin_port=0;
                if(-1==bind(clientUDP_in.perS, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in))){
                	perror(argv[0]);
                    printf("%s: unable to bind address UDP\n", argv[0]);
                    exit (1);
                }
                buffer[cc]='\0';
                if (clientUDP_in.perS == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    	close(s_UDP);/* Close the listen socket inherited from the daemon. */
        				serverUDP (clientUDP_in.perS, buffer, clientUDP_in,myaddr_in,fichero);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(clientUDP_in.perS);
        			}
                
                }
          }
		}   /* Fin del bucle infinito de atención a clientes */
        /* Cerramos los sockets UDP y TCP */
        close(ls_TCP);
        close(s_UDP);
        
    
        printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in,struct sockaddr_in server,FILE *f)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char hostname[MAXHOST];		/* remote host's name string */
	struct addrinfo hints, *res;
	int errcode;
	int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */
	
    				
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
             }
    /* Log a startup message. */
    time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
    char iPc[TAM_BUFFER];
    if(NULL==inet_ntop(AF_INET,&(clientaddr_in.sin_addr),iPc,INET_ADDRSTRLEN)){
    	fprintf(stderr,"Error con IPc");
    	exit(-1);
    }
    int i=strlen(iPc);
    iPc[i]='\0';
    fprintf(f,"\nConexion realizada a %s",(char *)ctime(&timevar));
	fprintf(f,"Nombre del servidor e IP y puerto: %s ***********************SERVER TCP*********************** IPCliente y puerto: %s/%u\n",hostname,iPc,ntohs(clientaddr_in.sin_port));
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */
	
	
	reqcnt = protocoloSMTPTCP(s,clientaddr_in,hostname,server,f);
	
	close(s);

		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
	fprintf(f,"Completed %s port %u, at %s------------------------------------\n",
		"hostname", ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
	fclose(f);
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char * buffer, struct UDPaddr_in clientaddr_in,struct sockaddr_in server,FILE *f)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode;
    long timevar;	
    int i=0;
    struct addrinfo hints, *res;
    char hostname[TAM_BUFFER];
    char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char resp[TAM_BUFFER];		/* Respuesta a mandar*/
	char temp[TAM_BUFFER];
    
	int addrlen;
    
   	addrlen = sizeof(struct sockaddr_in);

    if(1==getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),hostname,MAXHOST,NULL,0,0)){
        if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
         	perror(" inet_ntop \n");
    }

   	strcpy(buf,buffer);
    fprintf(f,"He recibido una solicitud de Conexion %s\n",buf);

	nc = sendto (s,"220", strlen("220"),
			0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
         perror("serverUDP");
         printf("%s: sendto error\n", "serverUDP");
         return;
         }
    time (&timevar);
    char iPc[TAM_BUFFER];
    if(NULL==inet_ntop(AF_INET,&(clientaddr_in.sin_addr),iPc,INET_ADDRSTRLEN)){
    	fprintf(stderr,"Error con IPc");
    	exit(-1);
    }
    i=strlen(iPc);
    iPc[i]='\0';
    fprintf(f,"\nConexion realizada a %s",(char *)ctime(&timevar));
    fprintf(f,"Nombre del servidor e IP: %s ***********************SERVER UDP*********************** IPCliente y puerto: %s/%u\n",hostname,iPc,ntohs(clientaddr_in.sin_port));
    
	i = protocoloSMTPUDP(s,clientaddr_in,hostname,server,f,addrlen,iPc);

	printf("Completed %s port %u, at %s\n",
		"hostname", ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
	fprintf(f,"Completed %s port %u, at %s------------------------------------\n",
		"hostname", ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
		fclose(f);  
		close(s); 
 }


int protocoloSMTPTCP(int s,struct sockaddr_in clientaddr_in,char *hostname,struct sockaddr_in server,FILE* f){

 	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char res[TAM_BUFFER];		/* Respuesta a mandar*/
	char saludo[TAM_BUFFER];
	char temp[TAM_BUFFER];
	int indBuf;
	int i;
	int ret=0,prot=0,len=0;	//ret=numero de request y prot=final de protocolo
	int sal=0,from=0,to=0,data=0;	/*Booleanos que marcan si ha recibido el saludo, si ha recibido
								el emisor y los destinatarios*/
	char iPc[TAM_BUFFER];
    if(NULL==inet_ntop(AF_INET,&(clientaddr_in.sin_addr),iPc,INET_ADDRSTRLEN)){
    	fprintf(stderr,"Error con IPc");
    	exit(-1);
    }
    i=strlen(iPc);
    iPc[i]='\0';
	
	if(!strcmp(hostname,"nogal.fis.usal.es")){
		strcpy(temp,"usal.es");
		i=strlen(temp);
		temp[i]='\r';
		temp[i+1]='\n';
		temp[i+2]='\0';
		strcpy(saludo,"HELO ");
		strcat(saludo,temp);
	}else{
		strcpy(temp,hostname);
		i=strlen(temp);
		temp[i]='\r';
		temp[i+1]='\n';
		temp[i+2]='\0';
		strcpy(saludo,"HELO ");
		strcat(saludo,temp);
	}
	while (prot==0) {
		len = recv(s, buf, BUFFERSIZE, 0);
		if (len == -1) errout(hostname); /* error from recv */
			/* The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAM_BUFFER bytes.  This is because a recv returns
			 * as soon as there is some data, and will not wait for
			 * all of the requested data to arrive.  Since TAM_BUFFER bytes
			 * is relatively small compared to the allowed TCP
			 * packet sizes, a partial receive is unlikely.  If
			 * this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAM_BUFFER bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next request.
			 */
		fprintf(f,"\n%s/%u envia a %s >>TCP>> %s",iPc,ntohs(clientaddr_in.sin_port),hostname,buf);
		if(0==strcmp(buf,"QUIT\r\n")){
			strcpy(res,"221\r\n");
			prot=1;
		} 
		if(prot==0){
			if(sal==0){ //TODAVIA NO HA RECIBIDO EL SALUDO
				if(!strcmp(buf,saludo)){
					strcpy(res,"200\r\n");
					sal=1;
				}else{
					strcpy(res,"500\r\n");
				}	
			}else if(from==0){ //UNA VEZ RECIBIDO EL SALUDO COMPROBARA EL MAIL FROM
					if(!strncmp(buf,"MAIL FROM: <",strlen("MAIL FROM: <"))){
						if(0<(indBuf=cmpArroba(buf,'@',12))){
							if(cmpFinal(buf,'>',indBuf)){
								strcpy(res,"200\r\n");
								from=1;
							}
						}else{
							strcpy(res,"500\r\n");
						}
					}else{
						strcpy(res,"500\r\n");
					}
			}else if(to==0){ //UNA VEZ RECIBIDO EL EMISOR RECIBE LOS RCPT TO O DATA
				if(!strncmp(buf,"RCPT TO: <",strlen("RCPT TO: <"))){
					if(0<(indBuf=cmpArroba(buf,'@',10))){
						if(cmpFinal(buf,'>',indBuf)){
							strcpy(res,"200\r\n");
							data=1;
						}
					}else{
						strcpy(res,"500\r\n");
					}
				}else if(data==1 && !strcmp(buf,"DATA\r\n")){ //COMPRUEBA SI YA SE VA A RECIBIR EL DATA
					to=1;
					strcpy(res,"200\r\n");
				}else{
					strcpy(res,"500\r\n");
				}
			}else if(to==1){
				if(!strcmp(buf,".\r\n")){
					from=0;
					to=0;
					strcpy(res,"200\r\n");
				}
				strcpy(res, "200\r\n");
				fprintf(f, "Recibiendo el mensaje");
			}
		}
		ret++;
		if (send(s, res, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		fprintf(f,"\n%s envia a %s/%u >>TCP>> %s",hostname,iPc,ntohs(clientaddr_in.sin_port),res);
	}
	return ret;
 }

int cmpArroba(char *buf,char comp,int indBuf){
	int i=0;
	while(buf[indBuf]!='\n'){
		if(buf[indBuf]=='@'){
			return i;
		}
		i++;
		indBuf++;
	}
	return -1;
}
int cmpFinal(char *buf,char comp,int indBuf){
	int i=0;
	int dot=0;
	while(buf[indBuf]!='\n'){
		if(buf[indBuf]=='.'){
			dot=1;
			break;
		}
		i++;
		indBuf++;
	}
	if(dot==1){
		while(buf[indBuf]!='\n'){
			if(buf[indBuf]=='>'){
				if(buf[indBuf+1]=='\r'){
					if(buf[indBuf+2]=='\n') return 1;
				}
			}
			i++;
			indBuf++;
		}
	}
	
	return -1;
}

int protocoloSMTPUDP(int s,struct UDPaddr_in clientaddr_in,char* hostname,struct sockaddr_in server,FILE* f,int addrlen,char* iPc){
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char resp[TAM_BUFFER];		/* Respuesta a mandar*/
	char temp[TAM_BUFFER];
	char saludo[TAM_BUFFER];
	int indBuf,i;
	int nc; //Recogemos los caracteres recogidos en el mensaje recibido recvfrom
	int ret=0,prot=0,len=0;	//ret=numero de request y prot=final de protocolo
	int sal=0,from=0,to=0,data=0;	/*Booleanos que marcan si ha recibido el saludo, si ha recibido
								el emisor y los destinatarios*/

	if(!strcmp(hostname,"nogal.fis.usal.es")){
		strcpy(temp,"usal.es");
		i=strlen(temp);
		temp[i]='\r';
		temp[i+1]='\n';
		temp[i+2]='\0';
		strcpy(saludo,"HELO ");
		strcat(saludo,temp);
	}else{
		strcpy(temp,hostname);
		i=strlen(temp);
		temp[i]='\r';
		temp[i+1]='\n';
		temp[i+2]='\0';
		strcpy(saludo,"HELO ");
		strcat(saludo,temp);
	}
	
	while (prot==0) {
		nc=recvfrom(s, buf, BUFFERSIZE - 1, 0,(struct sockaddr *)&clientaddr_in, &addrlen);
		if (len == -1) errout("hostname"); /* error from recv */
			/* The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAM_BUFFER bytes.  This is because a recv returns
			 * as soon as there is some data, and will not wait for
			 * all of the requested data to arrive.  Since TAM_BUFFER bytes
			 * is relatively small compared to the allowed TCP
			 * packet sizes, a partial receive is unlikely.  If
			 * this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAM_BUFFER bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next request.
			 */
		buf[nc]='\0';
		fprintf(f,"\n%s/%u envia a %s >>UDP>> %s",iPc,ntohs(clientaddr_in.sin_port),hostname,buf);
		if(0==strcmp(buf,"QUIT\r\n")){
			strcpy(resp,"221\r\n");
			prot=1;
		} 
		if(prot==0){
			if(sal==0){ //TODAVIA NO HA RECIBIDO EL SALUDO
				if(!strcmp(buf,saludo)){
					strcpy(resp,"200\r\n");
					sal=1;
				}else{
					strcpy(resp,"500\r\n");
				}	
			}else if(from==0){ //UNA VEZ RECIBIDO EL SALUDO COMPROBARA EL MAIL FROM
					if(!strncmp(buf,"MAIL FROM: <",strlen("MAIL FROM: <"))){
						if(0<(indBuf=cmpArroba(buf,'@',12))){
							if(cmpFinal(buf,'>',indBuf)){
								strcpy(resp,"200\r\n");
								from=1;
							}
						}else{
							strcpy(resp,"500\r\n");
						}
					}else{
						strcpy(resp,"500\r\n");
					}
			}else if(to==0){ //UNA VEZ RECIBIDO EL EMISOR RECIBE LOS RCPT TO O DATA
				if(!strncmp(buf,"RCPT TO: <",strlen("RCPT TO: <"))){
					if(0<(indBuf=cmpArroba(buf,'@',10))){
						if(cmpFinal(buf,'>',indBuf)){
							strcpy(resp,"200\r\n");
							data=1;
						}
					}else{
						strcpy(resp,"500\r\n");
					}
				}else if(data==1 && !strcmp(buf,"DATA\r\n")){ //COMPRUEBA SI YA SE VA A RECIBIR EL DATA
					to=1;
					strcpy(resp,"200\r\n");
				}else{
					strcpy(resp,"500\r\n");
				}
			}else if(to==1){
				if(!strcmp(buf,".\r\n")){
					from=0;
					to=0;
					strcpy(resp,"200\r\n");
				}
				strcpy(resp,"200\r\n");
				fprintf(f, "Recibiendo el mensaje");
			}
		}
		ret++;
		if (nc = sendto (s,resp, strlen(resp),0, (struct sockaddr *)&clientaddr_in, addrlen) == -1) errout("hree");
		fprintf(f,"\n%s envia a %s/%u >>UDP>> %s",hostname,iPc,ntohs(clientaddr_in.sin_port),resp);
		if(prot==1){
			break;
		}
	}
}
