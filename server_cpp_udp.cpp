/* References: http://www.linuxhowtos.org/C_C++/socket.htm
 * The above website has example code and explanations about how sockets
 * are made and used. It has both TCP and UDP examples.
 *
 * UDP ACK Explanation: Both the Server and Client have: char m_count.
 * When the sender sends a packet, the last character or 4 bytes of the packet
 * contain this m_count. m_count begins as '0'. Once the packet is sent, 
 * m_count is '1'. The receiver will begin with m_count = '0' so the first time
 * receiving the packet both will have m_count = '0'. Once received and stored,
 * the receiver changes m_count to '1' and sends ACK1. 
 * If a packet is resent by the sender, the last character will be a 1.
 * If the receiver's m_count is 1 then the packet is disregarded and the ACK1
 * is resent. All other combinations of the m_counts imply a new packet has
 * been sent and must be stored. This is due to my implementation inside a 
 * for loop when handling packets. This method is used for all packet sending.
 * 
 *
 * Testing Note: I find that sometimes the server terminates due to a dropped packet
 * or my own code. I cannot be certain as when I repeat the test I will get a
 * successful output. Please not this when testing :D
 */


#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <list>
#include <sstream> 

using namespace std;

struct keypair{
	string key;
	string value;
};

int main(int argc, char *argv[]){
	int sock, length_adr, n, pos, pos2, tries, i_contnum;
	unsigned int length_msg[2];
	socklen_t fromlen;
	struct sockaddr_in server;
	struct sockaddr_in from;
	char buffer[2048], m_count, tmp;
	string input, output, packet, ret_output, substr, key, value,
		   num, contnum;
	ostringstream convert;
	std::clock_t start;
	double duration;
	list<struct keypair>::iterator it;
	struct keypair *temp;
	list<struct keypair> keypair_list;

	if (argc < 2) {
		fprintf(stderr, "ERROR: Invalid number of args. Terminating.\n");
		return 0;
	}
	sock=socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0){
		fprintf(stderr,"ERROR: Counld not bind port. Terminating.\n");
		return 0;
	}
	length_adr = sizeof(server);
	bzero(&server,length_adr);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[1]));
	if (bind(sock,(struct sockaddr *)&server,length_adr)<0){
		fprintf(stderr,"ERROR: Could not bind port. Terminating.\n");
		return 0;
	}
	fromlen = sizeof(struct sockaddr_in);
	m_count = '0';
	while(1){
		input.clear();
		output.clear();
		bzero(buffer,2048);
		m_count='0';
		length_msg[0]=0;
		length_msg[1]=0;
		duration=0;
		
//=========Receiving Packet Length and ACK Client========//
		n = recvfrom(sock,length_msg,sizeof(length_msg),
					0,(struct sockaddr *)&from,&fromlen);
		m_count='1';
		start = std::clock();
		output.append("ACK");
		output.append("1");
		n = sendto(sock,output.c_str(),(output.size()),
				0,(struct sockaddr *)&from,fromlen);
		while((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) < 2){
			n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
					(struct sockaddr *)&from,&fromlen);
			if(n > 0){
				if(buffer[n-1] == '\0'){ //case when length sent twice
					if(buffer[4] != m_count){
						n = sendto(sock,output.c_str(),output.size(),
								0,(struct sockaddr *)&from,fromlen);
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

//=========Append Packets to Input=========//
		output.clear();
		m_count = '0';
		duration=0;
		for(long int i=length_msg[0];i>0;i=i-2046){
			if(duration >= 2 && i > 0){
				break;
			}
			tmp = buffer[n-1];
			if(tmp == '1' && m_count == '1'){ //Repeat Message ->  resend ACK
				//====send ACK1====//
				output.clear();
				start = std::clock();
				output.append("ACK");
				output.append("1");
				n = sendto(sock,output.c_str(),output.size(),
						0,(struct sockaddr *)&from,fromlen);
				while((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) < 2){
					n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
							(struct sockaddr *)&from,&fromlen);
					if(n > 0)
						break;
				}
				if(duration >= 2){
					break;
				}
			}else{ //All other cases are a new packet to be recorded.
				input.append(buffer);
				input.pop_back();
				bzero(buffer,2048);
				//====send ACK1====//
				m_count='1';
				start = std::clock();
				output.append("ACK");
				output.append("1");
				n = sendto(sock,output.c_str(),output.size(),
						0,(struct sockaddr *)&from,fromlen);
				while((duration = (std::clock() - start )/(double)CLOCKS_PER_SEC) < 2){
					n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
							(struct sockaddr *)&from,&fromlen);
					if(n > 0)
						break;
				}
			}
		}
		if(duration >= 2 && input.size() != length_msg[0]){ //Timed out and message was not done.
			fprintf(stderr,"ERROR: Failed to receive message. Terminating.\n");
			break;
		}


//=========Build Response from list========//
		ret_output.clear();
		if((pos = input.find_first_of("?")) == 0 ){ //Print ?key
			key = input.substr(pos+1);
			it = keypair_list.begin();
			while(it != keypair_list.end()){
				if(it->key.compare(input.substr(1,input.size())) == 0){
					ret_output = it->key;
					ret_output.append("=");
					ret_output.append(it->value);
				}
				++it;
			}
			if(it == keypair_list.end()){
				ret_output = key;
				ret_output.append("=");
			}
		}else if(input.compare("list") == 0){ //List Command
			pos = 0;
			it = keypair_list.begin();
			while(it != keypair_list.end()){
				ret_output.append(it->key);
				ret_output.append("=");
				ret_output.append(it->value);
				ret_output.append("\n");
				++it;
			}
			if(it == keypair_list.end()){
				ret_output.pop_back();
			}
			if(keypair_list.size() == 0)
				ret_output.append("empty");
		}else if((pos = input.find("listc")) == 0){
			num.clear();
			contnum.clear();
			ret_output.clear();
			it = keypair_list.begin();
			pos = input.find_first_of(" ");
			num = input.substr(pos+1);
			if((pos2 = num.find_first_of(" ")) != string::npos){
				contnum = num.substr(pos2+1);
				if(atoi(contnum.c_str()) == i_contnum && atoi(contnum.c_str()) !=0){
					num = num.substr(0,pos2);
					for(int i=0; i<atoi(contnum.c_str()); i++)
						++it;
					for(int i=0; i< atoi(num.c_str()); i++){
						if((i+atoi(contnum.c_str())) >= keypair_list.size())
							break;
						ret_output.append(it->key);
						ret_output.append("=");
						ret_output.append(it->value);
						ret_output.append("\n");
						++it;
						i_contnum++;
					}
					if((atoi(num.c_str())+atoi(contnum.c_str())) >= keypair_list.size())
						ret_output.append("END");
					else{
						convert.clear();
						convert.str("");
						convert << i_contnum;
						ret_output.append(convert.str());
					}
				}else{
					ret_output.append("ERROR: Invalid continuation key.");
				}
			}else{
				for(int i=0; i< atoi(num.c_str()); i++){
					if(i >= keypair_list.size())
						break;
					ret_output.append(it->key);
					ret_output.append("=");
					ret_output.append(it->value);
					ret_output.append("\n");
					++it;
					i_contnum = i+1;
				}
				if(atoi(num.c_str()) < keypair_list.size()){
					convert.clear();
					convert.str("");
					convert << i_contnum;
					ret_output.append(convert.str());
				}else{
					i_contnum=0;
					ret_output.append("END");
				}
			}
		}else if((pos = input.find_first_of("=")) != string::npos){ //Assignment Command
			it = keypair_list.begin();
			while(it != keypair_list.end()){
				if(it->key.compare(input.substr(0,pos)) == 0){
					it->value = input.substr(pos+1);
					break;
				}
				++it;
			}
			if(it == keypair_list.end()){
				temp = new struct keypair;
				keypair_list.insert(it,*temp);
				--it;
				it->key = input.substr(0,pos);
				it->value = input.substr(pos+1);
				delete temp;
			}
			ret_output.clear();
			ret_output.append("OK");
		}else{
			ret_output.append("ERROR: Server: Command not recognized");
		}
		

//=========Send Length of Output========//
		m_count = '0';
		length_msg[0] = ret_output.size();
		length_msg[1] = m_count;
		n=sendto(sock,&length_msg,
				sizeof(length_msg),0,(const struct sockaddr *)&from,fromlen);
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
						(const struct sockaddr *)&from,fromlen);
				tries++;
			}
			n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
					(struct sockaddr *)&from,&fromlen);
			if(n > 0){ //Successfully received packet
				output.clear();
				output.append(buffer);
				//cout <<"OUtput is: " <<  output << "\n";
				//output.pop_back();
				//cout << "input.back() is:.." << output.back() << "..\n";
				
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


//=========Send Response in Packets========//

		m_count='0';
		pos=0;
		for(long int i=length_msg[0];i>0;i=i-2046){
			packet.clear();
			packet.append(ret_output.substr(pos,2046));
			//packet.append(&m_count);
			packet.append("0");
			//cout << packet.c_str() << "\n";
			pos+=2046;
			n=sendto(sock,packet.c_str(),packet.size(),0,
					(const struct sockaddr *)&from,fromlen);
			//cout << n << "\n";
			tries=1;
			//m_count='1';
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
							(const struct sockaddr *)&from,fromlen);
					tries++;
				}
				n = recvfrom(sock,buffer,2048,MSG_DONTWAIT,
						(struct sockaddr *)&from,&fromlen);
				if(n > 0){ //Successfully received packet
					output.append(buffer);
					//cout << "input.back() is:.." << output.back() << "..\n";
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















		//cout << input << "\n";
		//cout << "Received a datagram: " << n << "\n";

	}
}

