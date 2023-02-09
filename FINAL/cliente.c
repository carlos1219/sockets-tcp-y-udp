#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PUERTO 3800
#define TAM_BUFFER 512
#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define TIMEOUT 6
#define MAXHOST 512

extern int errno;
void clienteTCP(int argc,char *argv[]);
void clienteUDP(int argc,char *argv[]);
void handler()
{
 printf("Alarma recibida \n");
}


int main(argc, argv)
int argc;
char *argv[];
{
	if (argc != 4) {
		fprintf(stderr, "Usage:  %s <remote host> <protocolo> <file.txt>\n", argv[0]);
		exit(1);
	}

	if(!strcmp(argv[2],"TCP")){
		clienteTCP(argc,argv);
	}else if(!strcmp(argv[2],"UDP")){
		clienteUDP(argc,argv);
	}else{
		exit(1);
	}
	return 0;
}

void clienteTCP(int argc,char *argv[]){
	int s;				/* connected socket descriptor */
   	struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    int sal=1;
    FILE *f;
    FILE *ext;
    if(NULL==(f=fopen(argv[3],"r"))) exit(1);
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
    /* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];
	char resp[TAM_BUFFER];

	if (argc != 4) {
		fprintf(stderr, "Usage:  %s <remote host> <file.txt>\n", argv[0]);
		exit(1);
	}
	
	/* Create the socket. */
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;
	
	/* Get the host information for the hostname that the
	 * user passed in. */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
        }
    else {
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	    }
    freeaddrinfo(res);

    /* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

		/* Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
	
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
		exit(1);
	}
		/* Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
	
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	 }

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	char ficheroname[20];
	sprintf(ficheroname,"%u",ntohs(myaddr_in.sin_port));
    strcat(ficheroname,".txt");
    if(NULL==(ext=fopen(ficheroname,"w+"))){
    	fprintf(stderr,"ERROR CREANDO FICHERO");
    	exit(1);
    }
	printf("Connected to %s on port %u at %s",argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));
	fprintf(ext,"Connected to %s on port %u at %s",argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));
	do{	
		fgets(buf,TAM_BUFFER,f);
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
			fprintf(stderr, "%s: Connection aborted on error ",	argv[0]);
			fprintf(stderr, "on send number %d\n", i);
			exit(1);
		}
		fprintf(ext,"Envio >>>> %s",buf);
		i = recv(s, resp, TAM_BUFFER, 0);
		if (i == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
			exit(1);
		}
		if(!strcmp(resp,"500\r\n")){
			fprintf(ext,"Recibo error >>>> %s",resp);
		}else{
			fprintf(ext,"Recibo >>>> %s",resp);
		}
			/* Print out message indicating the identity of this reply. */
	}while(strcmp(resp,"221\r\n"));
		fclose(ext);
		fclose(f);
		/* Now, shutdown the connection for further sends.
		 * This will cause the server to receive an end-of-file
		 * condition after it has received all the requests that
		 * have just been sent, indicating that we will not be
		 * sending any further requests.
		 */
	
	if (shutdown(s, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
		exit(1);
	}

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
			 * the begining of the next reply.
			 */
		
	
    /* Print message indicating completion of task. */
	time(&timevar);
	
	printf("All done at %s", (char *)ctime(&timevar));
	
}

void clienteUDP(int argc,char *argv[]){
	int i, errcode,nc;
	int retry = RETRIES;		/* holds the retry count */
    int s;				/* socket descriptor */
    long timevar;                       /* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
    struct in_addr reqaddr;		/* for returned internet address */
    int	addrlen, n_retry;
    struct sigaction vec;
   	char hostname[MAXHOST];
   	struct addrinfo hints, *res;
   	char buf[TAM_BUFFER];
	char resp[TAM_BUFFER];

	if (argc != 4) {
		fprintf(stderr, "Usage:  %s <remote host> <file.txt>\n", argv[0]);
		exit(1);
	}
	FILE *f;
    if(NULL==(f=fopen(argv[3],"r"))) exit(1);
    FILE *ext;


		/* Create the socket. */
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
    /* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	
			/* Bind socket to some local address so that the
		 * server can send the reply back.  A port number
		 * of zero will be used so that the system will
		 * assign any available port number.  An address
		 * of INADDR_ANY will be used so we do not have to
		 * look up the internet address of the local host.
		 */
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
		exit(1);
	   }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
            perror(argv[0]);
            fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
            exit(1);
    }

            /* Print out a startup message for the user. */
    time(&timevar);
            /* The port number must be converted first to host byte
             * order before printing.  On most hosts, this is not
             * necessary, but the ntohs() call is included here so
             * that this program could easily be ported to a host
             * that does require it.
             */
    char ficheroname[20];
	sprintf(ficheroname,"%u",ntohs(myaddr_in.sin_port));
    strcat(ficheroname,".txt");
    if(NULL==(ext=fopen(ficheroname,"w+"))){
    	fprintf(stderr,"ERROR CREANDO FICHERO");
    	exit(1);
    }

	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		 * user passed in.
		 */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
      }
    else {
			/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	 }
     freeaddrinfo(res);
     /* puerto del servidor en orden de red*/
	 servaddr_in.sin_port = htons(PUERTO);

   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    vec.sa_handler = (void *) handler;
    vec.sa_flags = 0;
    if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGALRM)");
            fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
            exit(1);
        }
	
    n_retry=RETRIES;
	while (n_retry > 0) {
	if (sendto (s, "\r\n", strlen("\r\n"), 0, (struct sockaddr *)&servaddr_in,
				sizeof(struct sockaddr_in)) == -1) {
        		perror(argv[0]);
        		fprintf(stderr, "%s: unable to send request\n", argv[0]);
        		exit(1);
        	}
        	fprintf(ext,"Envio peticion de conexion ");
    alarm(TIMEOUT);
		/* Wait for the reply to come in. */
		addrlen = sizeof(servaddr_in);
        if ((nc=recvfrom (s, resp, TAM_BUFFER-1, 0,
						(struct sockaddr *)&servaddr_in, &addrlen)) == -1) {
    		if (errno == EINTR) {
    				/* Alarm went off and aborted the receive.
    				 * Need to retry the request if we have
    				 * not already exceeded the retry limit.
    				 */
 		         printf("attempt %d (retries %d).\n", n_retry, RETRIES);
  	 		     n_retry--; 
                    } 
            else  {
				printf("Unable to get response from");
				exit(1); 
                }
              } 
        else {
            alarm(0);
            /* Print out response. */
            if (reqaddr.s_addr == ADDRNOTFOUND) 
               printf("Host %s unknown by nameserver %s\n", "YO", argv[1]);
            else {
                /* inet_ntop para interoperatividad con IPv6 */
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                   perror(" inet_ntop \n");
                }	
            break;	
            }
        }
    fprintf(ext,"---> He establecido conexion\n");
    fflush(stdout);
    resp[nc]='\0';
    fprintf(stdout,"Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));
    fprintf(ext,"Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

	do{
		fgets(buf,TAM_BUFFER,f);
		/* Send the request to the nameserver. */
        if (sendto (s, buf, strlen(buf), 0, (struct sockaddr *)&servaddr_in,
				sizeof(struct sockaddr_in)) == -1) {
        		perror(argv[0]);
        		fprintf(stderr, "%s: unable to send request\n", argv[0]);
        		exit(1);
        	}
        fprintf(ext,"Envio >>> %s",buf);
        n_retry=RETRIES;
	  while (n_retry > 0) {
	    alarm(TIMEOUT);
		/* Wait for the reply to come in. */
		addrlen = sizeof(servaddr_in);
        if ((nc=recvfrom (s, resp, TAM_BUFFER-1, 0,
						(struct sockaddr *)&servaddr_in, &addrlen)) == -1) {
    		if (errno == EINTR) {
    				/* Alarm went off and aborted the receive.
    				 * Need to retry the request if we have
    				 * not already exceeded the retry limit.
    				 */
 		         printf("attempt %d (retries %d).\n", n_retry, RETRIES);
  	 		     n_retry--; 
                    } 
            else  {
				printf("Unable to get response from");
				exit(1); 
                }
              } 
        else {
            alarm(0);
            /* Print out response. */
            if (reqaddr.s_addr == ADDRNOTFOUND) 
               printf("Host %s unknown by nameserver %s\n", "YO", argv[1]);
            else {
                /* inet_ntop para interoperatividad con IPv6 */
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                   perror(" inet_ntop \n");
                }	
            break;	
            }
  		}
  		resp[nc]='\0';
  		if(!strcmp(resp,"500\r\n")){
			fprintf(ext,"Recibo error >>>> %s",resp);
		}else{
			fprintf(ext,"Recibo >>>> %s",resp);
		}
	}while(strcmp(resp,"221\r\n"));
		/* Set up a timeout so I don't hang in case the packet
		 * gets lost.  After all, UDP does not guarantee
		 * delivery.
		 */
	  

    if (n_retry == 0) {
       printf("Unable to get response from");
       printf(" %s after %d attempts.\n", argv[1], RETRIES);
       }
    printf("All done at %s", (char *)ctime(&timevar));
}
