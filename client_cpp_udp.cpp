/* References and methodology are inside server_cpp_udp.cpp*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
#include <climits>

using namespace std;

int main(int argc, char *argv[]){
	int sock, n, tries, pos, pos2, i_contnum;
	char m_count, tmp;
	unsigned int length_adr, length_msg[2];
	string input, output, packet, num, contnum, substr;
	struct sockaddr_in server, from;
	struct hostent *hp;
	std::clock_t start;
	double duration;
	char buffer[2048];
	

	if (argc != 3) { fprintf(stderr,"ERROR: Invalid number of args. Terminating.\n");
		return 0;
	}
	sock= socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
		fprintf(stderr,"ERROR: Could not bind port. Terminating\n");
		return 0;
	}
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp==0){
		fprintf(stderr,"ERROR: Unknown Host. Terminating.\n");
		return 0;
	}
	if(atoi(argv[2]) > 65535 || atoi(argv[2]) < 1024){
        fprintf(stderr,"ERROR: Invalid port. Terminating\n");
		return 0;
	}

	bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	length_adr=sizeof(struct sockaddr_in);
	m_count = 0;
	while(1){
		input.clear();
		output.clear();
		bzero(buffer,2048);
		m_count='0';
		std::getline(cin,input);

//========Validation of Input=========//
		length_msg[0] = input.size();
		if(length_msg[0] > 4094000000){
			fprintf(stderr,"ERROR: Invalid command.\n");
			continue;
		}
		if(input.compare("help")==0){
			cout << "Usage Instructions:\n?key --> returns 'key=value'.\nkey=value --> stores key and value, returns OK.\nlist --> returns all 'key=value' pairs stored.\nlistc num --> returns first 'num' of 'key=value' pairs and a contnum\nlistc num contnum --> returns 'num' of 'key=value' pairs continuing from previous listc call;\n      requires previously returned contnum. Ends with a new contnum or END.\n";
			continue;
		}else if((pos = input.find_first_of("?")) == 0 ){ // ?key Command
			if((pos2 = input.find_first_of("=")) == 1){
				fprintf(stderr,"ERROR: Invalid command.\n");
				continue;
			}
		}else if(input.compare("list") == 0){ //List Command
			;
		}else if((pos = input.find("listc")) == 0){
			if((pos2 = input.find_first_of("=")) != string::npos){
				fprintf(stderr,"Error: Invalid command.\n");
				continue;
			}
			if(input.size() > 6){ //more than just listc
				pos = input.find_first_of(" ");
				num = input.substr(pos+1);
				if((pos2 = num.find_first_of(" ")) != string::npos){
					contnum = num.substr(pos2+1);
					if(contnum.find_first_of("=") == string::npos){
						;
					}
				}
			}else{
				fprintf(stderr,"ERROR: Invalid command.\n");
				continue;
			}
		}else if((pos = input.find_first_of("=")) != string::npos){
			substr = input.substr(pos+1);
			if((pos = substr.find("=")) == 0){
				fprintf(stderr,"Error: Invalid command.\n");
				continue;
			}
		}else if(input.compare("exit") == 0){
			break;
		}else{
			fprintf(stderr,"ERROR: Invalid command.\n");
			continue;
		}


//=========Sending Length of Input=========//
		length_msg[1] = m_count;
		n=sendto(sock,&length_msg,
				sizeof(length_msg),0,(const struct sockaddr *)&server,length_adr);
		tries=1;
		m_count='1';
		length_msg[1] = m_count;
		n=0;
		start = std::clock();
		while(1){
			if(n > 0 || tries > 3)
				break;
			if((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) > (0.5)){
				n=sendto(sock,&length_msg,
						sizeof(length_msg),0,
						(const struct sockaddr *)&server,length_adr);
				tries++;
			}
			n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
					(struct sockaddr *)&from,&length_adr);
			if(n > 0){ //Successfully received packet
				output.clear();
				output.append(buffer);
				if(output.back() == m_count) //ACK was true and can continue
					break;
				else //resend length
					continue;
			}
		}
		if(tries > 3){
			fprintf(stderr,"ERROR: Failed to send message. Terminating.\n");
			break;
		}

//=========Sending Packets========//
		m_count='0';
		pos=0;
		for(long int i=length_msg[0];i>0;i=i-2046){
			packet.clear();
			packet.append(input.substr(pos,2046));
			packet.append("0");
			pos+=2046;
			n=sendto(sock,packet.c_str(),packet.size(),0,
					(const struct sockaddr *)&server,length_adr);
			tries=1;
			n=0;
			packet.pop_back();
			packet.append("1");
			start = std::clock();
			bzero(buffer,2048);
			while(1){
				if(n > 0 || tries > 3)
					break;
				if((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) > (0.5)){
					n=sendto(sock,packet.c_str(),packet.size(),0,
							(const struct sockaddr *)&server,length_adr);
					tries++;
				}
				n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
						(struct sockaddr *)&from,&length_adr);
				if(n > 0){ //Successfully received packet
					output.append(buffer);
					if(output.back() == m_count) //ACK was true and can continue
						break;
					else //resend length
						continue;
				}
			}
			if(tries > 3){
				fprintf(stderr,"ERROR: Failed to send message. Terminating.\n");
				break;
			}
		}
		if(tries > 3){
			break;
		}
//========Receive Packet Lengths and ACK Server========//
		input.clear();
		output.clear();
		bzero(buffer,2048);
		m_count='0';
		length_msg[0]=0;
		length_msg[1]=0;
		duration=0;
		n = recvfrom(sock,length_msg,sizeof(length_msg),
					0,(struct sockaddr *)&from,&length_adr);
		m_count='1';
		start = std::clock();
		output.append("ACK");
		output.append("1");
		n = sendto(sock,output.c_str(),(output.size()),
				0,(struct sockaddr *)&server,length_adr);
		while((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) < 2){
			n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
					(struct sockaddr *)&from,&length_adr);
			if(n > 0){
				if(buffer[n-1] == '\0'){ //case when length sent twice
					if(buffer[4] != m_count){
						n = sendto(sock,output.c_str(),output.size(),
								0,(struct sockaddr *)&server,length_adr);
						start = std::clock();
						continue;
					}
					else
						cout << "Should Never Come here!!!!\n";
				}
				else 
					break;
			}
		}
		if(duration >= 2){
			fprintf(stderr,"ERROR: Failed to receive message. Terminating.\n");
			break;
		}




//=========Receive Packets from Server as response=========//
		output.clear();
		m_count = '0';
		duration=0;
		for(long int i=length_msg[0];i>0;i=i-2046){
			if(duration >= 2 && i > 0){
				break;
			}
			tmp = buffer[n-1];
			if(tmp == '1' && m_count == '1'){
				//====send ACK1====//
				output.clear();
				start = std::clock();
				output.append("ACK");
				output.append("1");
				n = sendto(sock,output.c_str(),output.size(),
						0,(struct sockaddr *)&server,length_adr);
				while((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) < 2){
					n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
							(struct sockaddr *)&from,&length_adr);
					if(n > 0)
						break;
				}
				if(duration >= 2){
					break;
				}
			}else{
				input.append(buffer);
				input.pop_back();
				bzero(buffer,2048);
				
				//====send ACK1====//
				m_count='1';
				start = std::clock();
				output.append("ACK");
				output.append("1");
				n = sendto(sock,output.c_str(),output.size(),
						0,(struct sockaddr *)&server,length_adr);
				while((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) < 2){
					n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
							(struct sockaddr *)&from,&length_adr);
					if(n > 0)
						break;
				}
			}
		}
		if(duration >= 2 && input.size() != length_msg[0]){
			fprintf(stderr,"ERROR: Failed to receive message. Terminating.\n");
			break;
		}
		if(input.compare("empty") == 0)
			input.clear();
		cout << input.c_str() << "\n";
	}
	close(sock);
	return 0;
}
