/* a server in the unix domain.  The pathname of 
   the socket address is passed as an argument */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <time.h>

using namespace std;

void error(const char *);

struct packet{
    bool syn;
    bool ack;
    bool fin;
    int sequence_number;
    int ack_number;
    int data[3];
};

int main(int argc, char *argv[])
{
    srand (time(NULL));
    
    int packnum=stoi(argv[2]);
    int sockfd;
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in serv,client;
    
    serv.sin_family = AF_INET;
    serv.sin_port = htons(stoi(argv[1]));
    serv.sin_addr.s_addr=INADDR_ANY;
    
    if(bind(sockfd,(struct sockaddr *)&serv,sizeof(serv))<0)
        error("binding socket");
    
    char buffer[256];
    socklen_t l = sizeof(client);
    //socklen_t m = client;
    
    
    int seqnum=30;
    bool handshaked=false;
    
    //Three way handshake
    while (!handshaked) {
        cout<<"Initiating three way handshake\n";
        int initrc=recvfrom(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&l);
        if(initrc<0)
        {
            cout<<"ERROR READING FROM SOCKET";
        }
        
        //received first packet
        packet recvfirsthandshake;
        if (buffer[0]==1) {
            recvfirsthandshake.syn=true;
        }
        else{
            cout<<"Received packet is not SYN packet"<<endl;
            continue;
        }
        if (buffer[1]==0) {
            recvfirsthandshake.ack=false;
            recvfirsthandshake.ack_number=0;
        }
        if (buffer[2]==0) {
            recvfirsthandshake.fin=false;
        }
        recvfirsthandshake.sequence_number=buffer[3];
        
        //create second packet based on first packet information
        packet secondhandshake;
        secondhandshake.syn=true;
        secondhandshake.ack=true;
        secondhandshake.fin=false;
        secondhandshake.sequence_number=seqnum;
        secondhandshake.ack_number=recvfirsthandshake.sequence_number+1;
        
        
        //put second pack info inside buffer
        if (secondhandshake.syn==true) {
            buffer[0]=1;
        }
        if (secondhandshake.ack==true) {
            buffer[1]=1;
            buffer[4]=secondhandshake.ack_number;
        }
        if (secondhandshake.fin==false) {
            buffer[2]=0;
        }
        buffer[3]=secondhandshake.sequence_number;
        int initrp=sendto(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&client,l);
        if(initrp<0)
        {
            cout<<"ERROR writing to SOCKET";
        }
        
        //receive third handshake packet
        packet recvthirdhandshake;
        initrc=recvfrom(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&l);
        if(initrc<0)
        {
            cout<<"ERROR READING FROM SOCKET";
        }
        //cout<<buffer<<endl;
        if (buffer[0]==0) {
            recvthirdhandshake.syn=false;
        }
        if (buffer[1]==1) {
            recvthirdhandshake.ack=true;
            recvthirdhandshake.ack_number=buffer[4];
        }
        if (buffer[2]==0) {
            recvthirdhandshake.fin=false;
        }
        
        
        if(recvthirdhandshake.ack_number==seqnum+1){
            cout<<"Three Way handshake established"<<endl;
            handshaked=true;
            seqnum++;
        }
        bzero(buffer,sizeof(buffer));
    }
    
    
    
    
    
    bool show=true;
    bool finish=false;
    packet packets[500];
    int packetsend[2000];
    //清空packet資料
    memset(packetsend, 0, sizeof(packetsend));
    
    //輸入資料給packet
    for (int i=0; i<packnum; i++) {
        packets[i].syn=false;
        packets[i].ack=false;
        packets[i].fin=false;
        packets[i].sequence_number=seqnum;
        seqnum++;
        for(int j=0;j<3;j++){
            packets[i].data[j]=rand()%999+1;
        }
    }
    
    int windowstart=0; //window開始的位置
    const int windowsize=4;
    int initrc;
    
    fd_set read_set;
    
    struct timeval tv;
    
    
    while (!finish) {
        FD_ZERO(&read_set);
        FD_SET(sockfd, &read_set);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        //從packet的structure資料,照順序輸入到array buffer(packetsend), client也依照一樣的順序讀出來,在client 端建立的packet structure輸入進去
        for (int i=windowstart; i<windowstart+windowsize; i++) {
            if (packets[i].syn==false) {
                packetsend[(i%windowsize)*8]=0;
            }
            if (packets[i].ack==false) {
                packetsend[(i%windowsize)*8+1]=0;
                packetsend[(i%windowsize)*8+4]=NULL;//ack number
            }
            if (packets[i].fin==false) {
                packetsend[(i%windowsize)*8+2]=0;
            }
            packetsend[(i%windowsize)*8+3]=packets[i].sequence_number;
            for (int j=0; j<3; j++) {
                packetsend[(i%windowsize)*8+5+j]=packets[i].data[j];
            }
        }
        int send=sendto(sockfd,packetsend,sizeof(packetsend),0,(struct sockaddr *)&client,l);
        if(send<0)
        {
            cout<<"ERROR writing to SOCKET";
        }
        memset(packetsend, 0, sizeof(packetsend));//傳完把packet整個清空歸0,下次就能使用
        int ret = select(sockfd+1, &read_set, NULL, NULL, &tv);//Timeout with select
        if(ret <= 0){
            cout<<"Timeout"<<endl;
            continue;
        }
        //cout<<"test"<<endl;
        initrc=recvfrom(sockfd,packetsend,sizeof(packetsend),0,(struct sockaddr *)&client,&l);
        //接受ACK回傳, ACK沒有data在裡面（都會是NULL）
        if(initrc<0)
        {
            cout<<"ERROR READING FROM SOCKET";
        }
        
        packet recvackpacket[20];
        for (int i=0; i<windowsize; i++) {
            if (packetsend[i*8]==0) {
                recvackpacket[i].syn=false;
            }
            if (packetsend[i*8+1]==0) {
                recvackpacket[i].ack=false;
                recvackpacket[i].ack_number=NULL;//ack number
            }
            else if (packetsend[i*8+1]==1) {
                recvackpacket[i].ack=true;
                recvackpacket[i].ack_number=packetsend[i*8+4];//ack number
            }
            if (packetsend[i*8+2]==0) {
                recvackpacket[i].fin=false;
            }
            recvackpacket[i].sequence_number=packetsend[i*8+3];
        }
        
        //檢查收到的ack,如果有ack錯就把回到原本位置再重傳一次
        int temp=windowstart;
        for (int i=0; i<windowsize; i++) {
            if (recvackpacket[i].ack_number==packets[windowstart].sequence_number+1) {
                //cout<<recvackpacket[i].ack_number<<" "<<packets[windowstart].sequence_number<<endl;
                windowstart++;
                
            }
            else if(windowstart!=packnum){
                
                //debug
                if(show) {
                    cout<<recvackpacket[i].ack_number<<" "<<packets[windowstart].sequence_number<<endl;
                    cout<<"Received a Duplicate ACK"<<endl;
                    show=false;
                }
                windowstart=temp;
                break;
            }
        }
        //cout<<windowstart<<endl;
        memset(packetsend, 0, sizeof(packetsend));//傳完把packet整個清空歸0,下次就能使用
        //如果最後一個ack=要傳的packet數量就停止
        int lastACK=windowstart+1;
        if(lastACK==packnum+1)
            finish=true;
    }
    
    
    //這裡開始傳送finish
    if(finish) {
        
        packet terminate;
		terminate.syn = false;
		terminate.ack = false;
        terminate.fin=true;
		terminate.ack_number = 0;
		terminate.sequence_number = seqnum;
		if (terminate.syn == false) {
			packetsend[0] = 0;
		}
		if (terminate.ack == false) {
			packetsend[1] = 0;
			packetsend[4] = 0;
		}
		if (terminate.fin == true) {
			packetsend[2] = 1;
		}
		packetsend[3] = terminate.sequence_number;
		sendto(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&client, l);
        //cout<<"test"<<endl;
		bzero(buffer, sizeof(buffer));
		recvfrom(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&client, &l);
		packet recvTerminateSynAck;
		if (packetsend[0] == 0) {
			recvTerminateSynAck.syn = false;
		}
		if (packetsend[1] == 1) {
			recvTerminateSynAck.ack = true;
			recvTerminateSynAck.ack_number = buffer[4];
		}
		if (packetsend[2] == 1) {
			recvTerminateSynAck.fin = true;
		}
		
		bzero(buffer, sizeof(buffer));
		recvfrom(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&client, &l);
        
		packet recvFin;
		if (packetsend[0] == 0) {
			recvFin.syn = false;
		}
		if (packetsend[1] == 1) {
			recvFin.ack = true;
			recvFin.ack_number = packetsend[4];
		}
		if (packetsend[2] == 1) {
			recvFin.fin = true;
		}
		recvFin.sequence_number = packetsend[3];

		packet sendFinal;
		sendFinal.syn = false;
		sendFinal.ack = true;
		sendFinal.fin = true;
		sendFinal.ack_number = recvFin.sequence_number + 1;
		if (sendFinal.syn == false) {
			packetsend[0] = 0;
		}
		if (sendFinal.ack == true) {
			packetsend[1] = 1;
			packetsend[4] = sendFinal.ack_number;
		}
		if (sendFinal.fin == true) {
			packetsend[2] = 1;
		}
		sendto(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&client, l);
    }
    
    for (int i=0; i<packnum; i++) {
        cout<<i+1<<": ";
        for (int j=0; j<3; j++) {
            cout<<packets[i].data[j]<<" ";
        }
        cout<<endl;
    }
    
    
    
    
    
    //cout<<"\ngoing to recv\n";
    /**int rc= recvfrom(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&l);
    if(rc<0)
    {
        cout<<"ERROR READING FROM SOCKET";
    }
    //cout<<"\n the message received is : "<<buffer<<endl;
    bzero(buffer,sizeof(buffer));
    int rp= sendto(sockfd,"hi",2,0,(struct sockaddr *)&client,l);
    
    /**if(rp<0)
    {
        cout<<"ERROR writing to SOCKET";
    }**/

   close(sockfd);
   return 0;
}




void error(const char *msg)
{
    perror(msg);
    exit(0);
}
