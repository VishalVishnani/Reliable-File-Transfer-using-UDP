//udp_client.c
//Includes
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>



//Buffer Size
#define BUFFER_SIZE 512



//Function for encryption
void encryption(char* buffer,int len,char* key){
  int i=0;
  int key1;
  for(i=0;i<len;i++){
    key1=i%4;
    buffer[i]=buffer[i]^key[key1];
  }
}




/*Error function which takes message as arguement*/
void error(char* msg){
  perror(msg);
  exit(0);
}



int main(int argc , char *argv[]){
  int sock,length,n;
  struct sockaddr_in server,from;
  struct hostent *hp;
  char key[4]="ABCD";
  char buffer_temp[BUFFER_SIZE-1];
  char buffer[BUFFER_SIZE];

  if(argc<3){
    fprintf(stderr,"usage %s hostname port\n",argv[0]);
    exit(1);
  }


  /*Create socket*/
  sock = socket(AF_INET,SOCK_DGRAM, 0);
  
  /*If value returned is -1, display error*/
  if(sock<0){
    error("socket failed");
  }

  hp=gethostbyname(argv[1]);

  if(hp==0){
    fprintf(stderr,"Error, no such host\n");
  }

  bzero((char*)&server, sizeof(server));

  server.sin_family=AF_INET;
  
  bcopy((char*)hp->h_addr, (char*)&server.sin_addr,hp->h_length);

  server.sin_port=htons(atoi(argv[2]));
  length=sizeof(struct sockaddr_in);
  char command;

  int ACK_i=1;
  char p_no='1';
  struct timeval tv;
  tv.tv_sec=3;
  tv.tv_usec=0;
  

  while(1){

    //set timeout for recvfrom
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

    ACK_i=1;
       
    //zero out both the buffers
    bzero(buffer,BUFFER_SIZE);
    bzero(buffer_temp,BUFFER_SIZE-1);


    //wait for command from user    
    printf("\nEnter command:\n");
    char command1[25];
    gets (command1);
    char* filename;
    filename=strtok(command1," ");

    
    //check which command is received
    int ret1=strcmp(command1,"get");
    int ret2=strcmp(command1,"put");
    int ret3=strcmp(command1,"ls");
    int ret4=strcmp(command1,"del");
    int ret5=strcmp(command1,"exit");

        
    //Seperate the file name
    if (ret1==0){
      command='1';
      filename=strtok(NULL," ");
    }

    else if (ret2==0){
      command='2';
      filename=strtok(NULL," ");
      //file open to read
      FILE *picture;
      picture=fopen(filename,"r");
      if(picture==NULL){
        printf("\nIncorrect file name\n");
        continue;
      }
      fclose(picture);

    }

    else if (ret3==0){
      command='3';
      strcpy(filename,"ls");
    }
  
    else if (ret4==0){
      command='4';
      filename=strtok(NULL," ");
    }

    else{
      command='5';
      strcpy(filename,"exit");
    }


    p_no='1'; 
    buffer[0]=p_no;
    buffer[1]=command;
  
    int i;
    for(i=0;i<25;i++){
      buffer[i+2]=*(filename+ i);
    }  
  
    //	printf("Buffer Value: %s\n",buffer);
    
    int ACK_1=0;

    //send the command to server and wait for ACK
    while(ACK_1!=1){
      
      n=sendto(sock,buffer,26,0,(struct sockaddr*)&server,length);

      if(n<0)
        error("Send to");

      bzero(buffer,BUFFER_SIZE);

      n=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr*)&server,&length);
    
      if(n<0)
        error("recv from");
      
      int ret= strcmp(buffer,"ACK");
      if(ret==0){
        ACK_1=1;
      }
      bzero(buffer,BUFFER_SIZE);  

    }
    ACK_1=0;



    //put command
    if(command=='2'){

      //file open to read
      FILE *picture;
      picture=fopen(filename,"r");

      if(picture==NULL)
        continue;

      
      int size;
      int size_cmp=0;
      int ACK_len=1;
      	

      //calculate size of file
      fseek(picture,0,SEEK_END);
      size=ftell(picture);
      fseek(picture,0,SEEK_SET);
  
      //send size of file 
      printf("\nSending file size\n");
      n=sendto(sock,&size,sizeof(int),0,(struct sockaddr*)&server,length);    
      n=sendto(sock,&size,sizeof(int),0,(struct sockaddr*)&server,length);

      printf("\nSize: %d\n",size);
		
				
      if(n<0)
        error("sendto");
      		
      sleep(1);
     

      int num=0;
      int size_to_send=0;
      int file_size=0;
      int size1=size;
          

      printf("\nSending file\n");

      //wait until entire file is send
      while(file_size<size1){
        ACK_i=1;
        p_no=p_no+1;
      
        //Calculate the size to send
        num=size/(BUFFER_SIZE-1);
        
        if(num>=1){
          size_to_send=BUFFER_SIZE-1;
        }
        else{
          size_to_send=size%(BUFFER_SIZE-1);
        }

        //read the file
        fread(buffer_temp,1,size_to_send,picture);

        //wait until ACK is received for a particular packet
        while(ACK_i!=0){
          buffer[0]=p_no;
          int k;
          for(k=0;k<BUFFER_SIZE-1;k++){
            buffer[k+1]=buffer_temp[k];
          }
          //printf("\nbuffer[0] = %d\n",buffer[0]);
          encryption(buffer,size_to_send+1,key);
          n=sendto(sock,buffer,size_to_send+1,0,(struct sockaddr*)&server,length);
          //printf("\n n= %d\n ",n);
        
          bzero(buffer,BUFFER_SIZE);
       
        
          n=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr*)&server,&length);
          char ACK_recv[4]="ACK";
          char temp=p_no+1;
         
          ACK_recv[3]=temp;
          ACK_i=strcmp(buffer,ACK_recv);
          //printf("\n p_no send = %d and ack no. receveived= %d\n",p_no,buffer[3]);
          //printf("\nACK_recv : %s\n",ACK_recv);          
          bzero(buffer,BUFFER_SIZE);
        }

        bzero(buffer_temp,BUFFER_SIZE-1);
        size=size-size_to_send;
        file_size=file_size+size_to_send;
       
      }
      printf("\nFile transfer completed\n");
      fclose(picture);  
    }





    //ls command 
    else if(command=='3'){
      n=recvfrom(sock,buffer,BUFFER_SIZE,0,(struct sockaddr*)&server,&length);
      encryption(buffer,BUFFER_SIZE,key);

      if(n<0)
        error("Recv from");
     
      printf("%s\n",buffer); 
    }






    //Get file command
    else if (command=='1'){
      int m;
      printf("\nReceiving file size\n");
      int size;
      int ACK_len;
      ACK_len=1;
      int loss=0;

      //Get file size 
     

      while(1){
        n=recvfrom(sock,&size,sizeof(int),0,(struct sockaddr*)&server,&length);
	if(n>0 && size > 0){
	  break;
	}
      }
      sleep(2);
      
      if(n<0)
        error("recvfrom");
        
      printf("\nFile size : %d\n",size);
      printf("\nReceiving File now\n");

      //Open file to write
      int file_size=0;
      FILE* image;
      image=fopen(filename,"w");

      //Wait until entire file is received
      while(file_size < size){
        p_no=p_no+1;

        //wait until ack is received for a particular packet
        while(ACK_i!=0){
          n=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr*)&server,&length);
          //printf("\n n= %d\n",n);

          encryption(buffer,n,key);
          char ACK_send[4]="ACK";

	  //when right packed is received, send ACk of expected packet
          if(buffer[0]==p_no){
            char temp=p_no+1;
            ACK_send[3]=temp;
            
            m=sendto(sock,ACK_send,sizeof(ACK_send),0,(struct sockaddr*)&server,length);
            ACK_i=0;
            //printf("\nRecieved : %d  and Expected: %d \n",buffer[0],p_no);
          }

	  //when out of order packet is received, send ACK of expected packet
          else{
            ACK_i=1;
            ACK_send[3]=p_no;
            m=sendto(sock,ACK_send,sizeof(ACK_send),0,(struct sockaddr*)&server,length);
            printf("\nPACKET LOSS: %d\n",loss);
            printf("\nRecieved : %d  and Expected: %d \n",buffer[0],p_no);
            loss=loss+1;
            bzero(buffer,BUFFER_SIZE);
          }
        //printf("\nACK_send= %s\n",ACK_send);
        }
        ACK_i=1;
        int l;

        //copy to buffer temp to remove the packet number
        for(l=0;l<BUFFER_SIZE-1;l++){
          buffer_temp[l]=buffer[l+1];
        }


        //write to a file
        fwrite(buffer_temp,1,n-1,image);
        file_size=file_size+n-1;
        printf("\nFilesize : %d\n",file_size); 
      }
      fclose(image);
      printf("\nFile received of size: %d\n",file_size);
      printf("\nLoss: %d\n",loss);
    }




    //Delete command
    else if (command=='4'){
      n=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr*)&server,&length);
      printf("\nReceived: %s\n",buffer);
    }

	

    //Exit command
    else if(command=='5'){
      exit(1);
    }
     

  }

}  

