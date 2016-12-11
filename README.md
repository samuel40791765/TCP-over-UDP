Use Linux to run
We used a window size of 4 to transmit our packets. 
Packets each contain 3 random numbers from 1~999. When finished transmitting, both the server and client will show all the numbers in each packet transmitted.

1. Compile both the server and client cpp file
	g++ server.cpp -o server
	g++ client.cpp -o client

2. Make sure the port numbers are the same and which IP address you are on.
	

3. Run both of the Unix Executable in two different windows.
	(Important!: Number of packets cannot exceed 100, because of the packet array size we set. If you want to send more, you can change it.)
	./server <port num> <number of packets to send(1~100)>

	./client <IP address> <port num(1~100)>