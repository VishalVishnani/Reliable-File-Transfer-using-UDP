//udp_server.c
//Includes
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/time.h>


//buffer size
#define BUFFER_SIZE 512


//Encryption function
void encryption(char* buffer,int len,char* key){
  int i=0;
  int key1;
  for(i=0;i<len;i++){
    key1=i%4;
    buffer[i]=buffer[i]^key[key1];
  }
}



//Function to display error which takes message as an arguement
void error(char* msg){
  perror(msg);
  exit(1);
}



int main( int argc, char* argv[]){

  int sock,length,fromlen,n;

  //key for encryption/decryption
  char key[4]="ABCD";

  char buf[BUFFER_SIZE];
  char buf_temp[BUFFER_SIZE-1];
  struct sockaddr_in server,from;
  struct stat st;  
 
  if(argc<2){
    fprintf(stderr,"Error, no port provided\n");
    exit(1);
  }
  

  //Create socket
  //AF_INET is used for ip concept
  //SOCK_DGRAM- Socekt is to be created for udp
  sock= socket(AF_INET, SOCK_DGRAM, 0);
  

  //If sock returns -1, there was some error
  if(sock<0){
    error("Failed to create socket");
  }

  length=sizeof(server);

  //Clearing the structure variable
  bzero((char*)&server, length);

  server.sin_family = AF_INET;

  //INADDR_ANY is to get the ip address of PC automatically
  server.sin_addr.s_addr = INADDR_ANY;

  //Port no. as command line argument
  //htons converts into network understandable format
  server.sin_port=htons(atoi(argv[1]));


  //bind
  if(bind(sock, (struct sockaddr*)&server, length)<0){
    error("bind failed");
  }
  
  fromlen=sizeof(struct sockaddr_in);
  char p_no='1';
  
  struct timeval tv,tv1;
  tv.tv_sec=3;
  tv.tv_usec=0;
	 

  while(1){

    //Time out for recvfrom
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
 
    p_no='1';
    int ACK_i=1;

   
    printf("\nWaiting for command: \n");
   	
    //zero out the buffers
    bzero(buf,BUFFER_SIZE);
    bzero(buf_temp,BUFFER_SIZE-1);    
   

    //wait until the command packet is received
    while(1){
      n=recvfrom(sock,buf,25,0,(struct sockaddr*)&from,&fromlen);
      if(n>=0){
        break;
      }
    }
	

    printf("%s\n",buf); 
    int ACK_1=0;

    if(buf[0]=='1'){
      printf("\nCommand packet received\n");
      n=sendto(sock,"ACK",3,0,(struct sockaddr*)&from,fromlen);
      ACK_1=1;
      p_no=p_no+1;
    }
    else{
      n=sendto(sock,"NAK",3,0,(struct sockaddr*)&from,fromlen);
    }
   
    //if right command packet received
    if(ACK_1==1){

      char filename[24];
      int j;

      //seperate file name
      for(j=0;j<24;j++){
        filename[j]=buf[j+2];
      }

      //printf("Filename: %s\n",filename);



      //ls command
      if(buf[1]=='3'){
        printf("\nSendling ls output\n");
        FILE* proc=popen("ls","r");
        int c,i=0;
        while((c=fgetc(proc))!=EOF && i<BUFFER_SIZE-1)
          buf[i++]=c;
        buf[i]=0;
        pclose(proc);
        encryption(buf,BUFFER_SIZE,key);
        n=sendto(sock,buf,BUFFER_SIZE,0,(struct sockaddr*)&from,fromlen);
        if(n<0)
          error("Sendto");
      }		





      //put command
      else if(buf[1]=='2'){
        int m;

        printf("\nReceiving file size\n");
        int size;
        int ACK_len;
			
        ACK_len=1;
        int loss=0;

	//wait for filesize 

	while(1){
				
          n=recvfrom(sock,&size,sizeof(int),0,(struct sockaddr*)&from,&fromlen);

	  if(n>0 && size >0){
	    break;
	  }
	}
	sleep(2);
					
		
        if(n<0)
	  error("recvfrom");

        printf("\nFile size : %d\n",size);

        printf("\nReceiving File now\n");


	//open file to write
        int file_size=0;
        FILE* image;
        image=fopen(filename,"w");
     

	
    	//wait until entire file is received  
        while(file_size < size){
      	

	  //wait until ACK is received for a particular packet
          while(ACK_i!=0){
          
            n=recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr*)&from,&fromlen);            
            //printf("\n n= %d\n",n); 
            encryption(buf,n,key);
            char ACK_send[4]="ACK";
            
	    //expected packet arrives, send ACK of next expecting packet
            if(buf[0]==p_no){
              char temp=p_no+1;
       
              ACK_send[3]=temp;
              m=sendto(sock,ACK_send,sizeof(ACK_send),0,(struct sockaddr*)&from,fromlen);
              ACK_i=0;
              //printf("\nRecieved : %d  and Expected: %d \n",buf[0],p_no);
             
            }

	    //unexpected packet, send ACK of expected packet
            else{
              ACK_i=1;           
         
              ACK_send[3]=p_no;
              m=sendto(sock,ACK_send,sizeof(ACK_send),0,(struct sockaddr*)&from,fromlen);
              printf("\nPACKET LOSS : %d\n",loss);
              printf("\nRecieved : %d  and Expected: %d \n",buf[0],p_no);
              loss=loss+1;
              bzero(buf,BUFFER_SIZE);
            }
          
	    //printf("\nACK_send= %s\n",ACK_send);
          }
         
          p_no=p_no+1;
          ACK_i=1;
          
          //seperate packet from its sequence number	
          for(j=0;j<BUFFER_SIZE-1;j++){
            buf_temp[j]=buf[j+1];
          }

					
          //write to file
          fwrite(buf_temp,1,n-1,image);
          file_size=file_size+n-1;
          printf("\nFilesize : %d\n",file_size); 
        }
        fclose(image);
        printf("\nFile received of size: %d\n",file_size);
        printf("\nTotal Loss: %d\n",loss);
//        sleep(2);
      }





      //get command
      else if(buf[1]=='1'){

        //open file	
        FILE *picture;
        picture=fopen(filename,"r");
        int size;
        int size_cmp=0;
        int ACK_len=1;

	//find size of file
        fseek(picture,0,SEEK_END);
        size=ftell(picture);
        fseek(picture,0,SEEK_SET);

				
	//send size of picture
        printf("\nSending filesize\n");

        //sending length of picture
        n=sendto(sock,&size,sizeof(int),0,(struct sockaddr*)&from,fromlen);
	n=sendto(sock,&size,sizeof(int),0,(struct sockaddr*)&from,fromlen);
			
        printf("Size: %d\n",size);
        if(n<0)
          error("sendto");
        sleep(1);
          	

        int num=0;
        int size_to_send=0;
        int file_size=0;
        int size1=size;


	//wait until entire file is received 
	printf("\nSending file\n");
        while(file_size<size1){
          ACK_i=1;

	  //calculate no. of bytes to send
          num=size/(BUFFER_SIZE-1);
         
          if(num>=1)
            size_to_send=BUFFER_SIZE-1;
          else
            size_to_send=size % (BUFFER_SIZE-1);

	  //read the file
          fread(buf_temp,1,size_to_send,picture);

	  //wait for ack for a particular packet
          while(ACK_i!=0){

	    //create packet to send with packet number
            buf[0]=p_no;
          
            int k;
						
            for(k=0;k<BUFFER_SIZE-1;k++){
              buf[k+1]=buf_temp[k];
            }
            encryption(buf,size_to_send+1,key);
            n=sendto(sock,buf,size_to_send+1,0,(struct sockaddr*)&from,fromlen);
            //printf("\n n= %d\n ",n);

            bzero(buf,BUFFER_SIZE);
           
            n=recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr*)&from,&fromlen);
           
            char ACK_recv[4]="ACK";
            char temp=p_no+1;
            ACK_recv[3]=temp;
            ACK_i=strcmp(buf,ACK_recv);

            //printf("\n p_no send = %d and ack no. receveived= %d\n",p_no,buf[3]);

            //printf("\nACK_recv : %s\n",ACK_recv);

            bzero(buf,BUFFER_SIZE);
          }
          bzero(buf_temp,BUFFER_SIZE-1);
          size=size-size_to_send;
          file_size=file_size+size_to_send;
          p_no=p_no+1;
        
        }
        printf("\nFile Send Completed\n");
        fclose(picture);
      }



		
      //remove command
      else if(buf[1]=='4'){
        int  ret=remove(filename);
        if (ret==0){
          printf("\nFile deleted successfully\n");
          n=sendto(sock,"File Deleted",12,0,(struct sockaddr*)&from,fromlen);
        }
        else{
          printf("\nFile Deletion Failed\n");
          n=sendto(sock,"File Deletion Failed",20,0,(struct sockaddr*)&from,fromlen);

        } 

      }
		


      //exit
      else if(buf[1]=='5'){
        printf("\nExiting\n");
        exit(1);
      }
    }
    ACK_1=0;
    
  }

 
}

