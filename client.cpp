/* a client in the unix domain */
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>

using namespace std;

struct packet{
    bool syn;
    bool ack;
    bool fin;
    int sequence_number;
    int ack_number;
    int data[3];
};

void error(const char *);


int main(int argc, char *argv[])
{
    bool sendDuplicate=true;
    bool Timeoutsend=true;
    int sockfd;
    int seqnum=1;
    int servseq=0;
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in serv,client;
    
    serv.sin_family = AF_INET;
    serv.sin_port = htons(stoi(argv[2]));
    serv.sin_addr.s_addr = inet_addr(argv[1]);
    
    char buffer[256];
    socklen_t l = sizeof(client);
    socklen_t m = sizeof(serv);
    //socklen_t m = client;
    bool handshaked=false;
    
    while (!handshaked) {
        cout<<"Initiating three way handshake\n";
        packet firsthandshake;
        
        firsthandshake.syn=true;
        firsthandshake.ack=false;
        firsthandshake.ack_number=0;
        firsthandshake.fin=false;
        firsthandshake.sequence_number=seqnum;
        
        //buffer[0]=seqnum;
        if (firsthandshake.syn==true) {
            buffer[0]=1;
        }
        if (firsthandshake.ack==false) {
            buffer[1]=0;
            buffer[4]=0;
        }
        if (firsthandshake.fin==false) {
            buffer[2]=0;
        }
        buffer[3]=firsthandshake.sequence_number;
        sendto(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&serv,m);
        bzero(buffer,sizeof(buffer));
        recvfrom(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&l);
        packet recvsecondhandshake;
        if (buffer[0]==1) {
            recvsecondhandshake.syn=true;
        }
        else{
            cout<<"Received packet is not SYN packet"<<endl;
            continue;
        }
        if (buffer[1]==1) {
            recvsecondhandshake.ack=true;
            recvsecondhandshake.ack_number=buffer[4];
        }
        if (buffer[2]==0) {
            recvsecondhandshake.fin=false;
        }
        recvsecondhandshake.sequence_number=buffer[3];
        bzero(buffer,sizeof(buffer));
        //printf("%d %d\n",buffer[0],buffer[1]);
        if(recvsecondhandshake.ack_number!=seqnum+1){
            cout<<"Ack doesn't equal sequance number\n"<<endl;
            continue;
        }
        else{
            packet thirdhandshake;
            thirdhandshake.syn=false;
            thirdhandshake.ack=true;
            thirdhandshake.ack_number=recvsecondhandshake.sequence_number+1;
            thirdhandshake.fin=false;
            
            if (thirdhandshake.syn==false) {
                buffer[0]=0;
            }
            if (thirdhandshake.ack==true) {
                buffer[1]=1;
                buffer[4]=thirdhandshake.ack_number;
            }
            if (thirdhandshake.fin==false) {
                buffer[2]=0;
            }
            sendto(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&serv,m);
            cout<<"Three Way handshake established"<<endl;
            servseq=thirdhandshake.ack_number;
            seqnum++;
            handshaked=true;
        }
        bzero(buffer,sizeof(buffer));
    }
    
    
    
    
    
    bool finish=false;
    packet recvpackets[500];
    int packetsend[2000];
    int windowstart=0; //另一端也要存著window開始的位置
    const int windowsize=4;
    bool show=true;
    int finishpacketnum=0;//儲存finish packet在recvpackets的位置
    int temp2=0;
    
    while (!finish) {
        int tempstart=windowstart;
        memset(packetsend, 0, sizeof(packetsend));//傳完把packet整個清空歸0,下次就能使用
        //cout<<"kk"<<endl;
        int initrc=recvfrom(sockfd,packetsend,sizeof(packetsend),0,(struct sockaddr *)&client,&l);
        //cout<<"received"<<endl;
        if(initrc<0)
        {
            cout<<"ERROR READING FROM SOCKET";
        }
        //cout<<"dd"<<endl;
        //把收到的packet跟原本一樣的做出來
        for (int i=windowstart; i<windowstart+windowsize; i++) {
            recvpackets[i].sequence_number=packetsend[(i%windowsize)*8+3];
            if (packetsend[(i%windowsize)*8]==0) {
                recvpackets[i].syn=false;
            }
            if (packetsend[(i%windowsize)*8+1]==0) {
                recvpackets[i].ack=false;
                recvpackets[i].ack_number=NULL;//ack number
            }
            else if (packetsend[(i%windowsize)*8+1]==1) {
                recvpackets[i].ack=true;
                recvpackets[i].ack_number=packetsend[i*8+4];//ack number
            }
            if (packetsend[(i%windowsize)*8+2]==0) {
                recvpackets[i].fin=false;
            }
            else if(packetsend[2]==1) {//在這裡偵測有沒有收到finish packet
                if (temp2%windowsize!=0) {
                    windowstart=temp2;
                }
                recvpackets[i].fin=true;
                finishpacketnum=i; //儲存finish packet位置
                //cout<<"test1"<<endl;
                finish=true;//讓while結束
                break;
            }
            
            for (int j=0; j<3; j++) {
                recvpackets[i].data[j]=packetsend[(i%windowsize)*8+5+j];
            }
        }
        memset(packetsend, 0, sizeof(packetsend));//傳完把packet整個清空歸0,下次就能使用
        if (finish) {
            break;
        }
        
        
        
        packet sendpackets[10];
        int temp=servseq;
        //依照寄來的資訊制做回傳給server的ack packet,如果有錯誤windowstart不變,等待server回傳那些值過來
        for (int i=0; i<windowsize; i++) {
            sendpackets[i].syn=false;
            sendpackets[i].ack=true;
            sendpackets[i].fin=false;
            if (recvpackets[windowstart].sequence_number==servseq) { //主要偵測封包有無錯誤的地方
                //cout<<recvpackets[windowstart].sequence_number<<" "<<servseq<<endl;
                sendpackets[i].ack_number=servseq+1;
                windowstart++;
                servseq++;
            }
            //else if(windowstart!=10 || recvpackets[windowstart].fin==true )
            else {
                if (show) {
                    cout<<recvpackets[windowstart].sequence_number<<" "<<servseq<<" "<<windowstart<<endl;
                    //show=false;
                }
                temp2=windowstart;
                servseq=temp;
                sendpackets[i].ack_number=NULL;
                windowstart=tempstart;
                break;
            }
            sendpackets[i].sequence_number=seqnum;
            seqnum++;
        }
        
        
        //把ack packet塞進寄過去的buffer array
        for (int i=0; i<windowsize; i++) {
            if (sendpackets[i].syn==false) {
                packetsend[i*8]=0;
            }
            if (sendpackets[i].ack==false) {
                packetsend[i*8+1]=0;
                packetsend[i*8+4]=NULL;//ack number
            }
            else if(sendpackets[i].ack==true){
                packetsend[i*8+1]=1;
                packetsend[i*8+4]=sendpackets[i].ack_number;
            }
            if (sendpackets[i].fin==false) {
                packetsend[i*8+2]=0;
            }
            packetsend[i*8+3]=sendpackets[i].sequence_number;
            for (int j=0; j<3; j++) {
                packetsend[i*8+5+j]=NULL;
            }
        }
        
        //傳dublicate ACK在這
        if (sendDuplicate && windowstart==20) {
            cout<<"Sending a Dublicate packet"<<endl;
            windowstart=tempstart;
            servseq=temp;
            packetsend[3*8+4]=packetsend[2*8+4]; //設定跟前一個packet的ack一樣
            sendDuplicate=false;
        }
        //cout<<windowstart<<endl;
        
        //傳timeout在這
        if(Timeoutsend==true && windowstart==24){
            cout<<"Not sending ack packet on purpose, to make server timeout..."<<endl;
            windowstart=tempstart;
            servseq=temp;
            Timeoutsend=false;
            sleep(2);
            continue;
        }
        else{
            sendto(sockfd,packetsend,sizeof(packetsend),0,(struct sockaddr *)&serv,m);
        }
        
        //debug,之後是要收到finpacket才結束!!!!!
        if (windowstart==11) {
            cout<<"test2"<<endl;
            finish=true;//
        }
    }

    if (finish) {
        //cout<<"test"<<endl;
        //int initrc = recvfrom(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&client, &l);
        //if (initrc<0)
        //{
         //   cout << "ERROR READING FROM SOCKET";
        //}
        
        packet recvTerminate;
        if (packetsend[0] == 0) {
            recvTerminate.syn = false;
        }
        if (packetsend[1] == 0) {
            recvTerminate.ack = false;
            recvTerminate.ack_number = 0;
        }
        if (packetsend[2] == 1) {
            recvTerminate.fin = true;
        }
        recvTerminate.sequence_number = packetsend[3];
        //cout<<"test"<<endl;
        packet sendTerminateAck;
        sendTerminateAck.syn = false;
        sendTerminateAck.ack = true;
        sendTerminateAck.fin = true;
        sendTerminateAck.ack_number = recvpackets[finishpacketnum].sequence_number + 1;
        if (sendTerminateAck.syn == false) {
            packetsend[0] = 0;
        }
        if (sendTerminateAck.ack == true) {
            packetsend[1] = 1;
            packetsend[4] = sendTerminateAck.ack_number;
        }
        if (sendTerminateAck.fin == true) {
            packetsend[2] = 1;
        }
        
        int initrp = sendto(sockfd,packetsend, sizeof(packetsend), 0, (struct sockaddr *)&serv, m);
        
        packet sendFin;
        sendFin.syn = false;
        sendFin.ack = true;
        sendFin.fin = true;
        sendFin.sequence_number = seqnum;
        sendFin.ack_number = recvTerminate.sequence_number + 1;
        if (sendFin.syn == false) {
            packetsend[0] = 0;
        }
        if (sendFin.ack == true) {
            packetsend[1] = 1;
            packetsend[4] = sendFin.ack_number;
        }
        if (sendFin.fin == true) {
            packetsend[2] = 1;
        }
        packetsend[3] = sendFin.sequence_number;
        initrp = sendto(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&serv, m);
        //seqnum++;
        packet recvFinal;
        int initrc = recvfrom(sockfd, packetsend, sizeof(packetsend), 0, (struct sockaddr *)&client, &l);
        if (initrc<0)
        {
            cout << "ERROR READING FROM SOCKET";
        }
        //cout<<buffer<<endl;
        if (packetsend[0] == 0) {
            recvFinal.syn = false;
        }
        if (packetsend[1] == 1) {
            recvFinal.ack = true;
            recvFinal.ack_number = packetsend[4];
        }
        if (packetsend[2] == 0) {
            recvFinal.fin = true;
        }
        
        //cout<<recvFinal.ack_number<<" "<<seqnum<<endl;
        if (recvFinal.ack_number == seqnum + 1) {
            cout << "Fin correct" << endl;
        }
        //break;

    }
    
    //顯示packet
    for (int i=0; i<windowstart; i++) {
        cout<<i+1<<": ";
        for (int j=0; j<3; j++) {
            cout<<recvpackets[i].data[j]<<" ";
        }
        cout<<endl;
    }
    
    
    
    
    
    //cout<<"\ngoing to send\n";
    //cout<<"\npls enter the mssg to be sent\n";
    //fgets(buffer,256,stdin);
    //sendto(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *)&serv,m);
    bzero(buffer,sizeof(buffer));
    //recvfrom(sockfd,buffer,256,0,(struct sockaddr *)&client,&l);
    cout<<buffer<<endl;
    
    
    close(sockfd);
    return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
