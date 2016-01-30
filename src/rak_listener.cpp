//
//  rak_listener.cpp
//  BallsWars
//
//  Created by Gabriele Di Bari on 14/08/15.
//  Copyright (c) 2015 Gabriele Di Bari. All rights reserved.
//
#include <rak_listener.h>
#include <iostream>
#include <string>
#define DEBUG_MESSAGE(x) std::cout << x << std::endl;
#define DEBUG_CODE(x) x
//using namespace
void server_listener::open(unsigned short port, double timeOut, int maximumIncomingConnections)
{
    close();
    m_peer = RakNet::RakPeerInterface::GetInstance();
    RakNet::SocketDescriptor sd(port, 0);
	m_peer->Startup(maximumIncomingConnections, &sd, 1);
	m_peer->SetMaximumIncomingConnections(maximumIncomingConnections);
}
//number of connections
int server_listener::get_maximum_incoming_connections() const
{
    return m_peer->GetMaximumIncomingConnections();
}
//get my andress
server_listener::system_address server_listener::get_my_address() const
{
    return m_peer->GetMyBoundAddress();
}
//close connection
void server_listener::close()
{
    if (m_peer)
    {
        //delete
		m_peer->CloseConnection(m_peer->GetMyGUID(), true);
        RakNet::RakPeerInterface::DestroyInstance(m_peer);
    }
    m_peer = nullptr;
}

//new id
server_listener::UID server_listener::new_uid()
{
    return ++m_uid_generator;
}
//new client
server_listener::UID server_listener::add_new_client(server_listener::system_address& address)
{
    //new uid
    UID uid = new_uid();
    //push new client
    m_clients[uid] = client{ uid, address, nullptr, nullptr };
    //return uid
    return uid;
}
//send uid to client
void server_listener::send_uid(server_listener::client& client)
{
    bit_stream stream;
    stream.Write< message_id >((message_id)ID_SERVER_SEND_UID);
    stream.Write< UID >( client.m_uid );
    m_peer->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, client.m_address, false);
}
//get id by address
server_listener::UID server_listener::get_uid_by_address(const server_listener::system_address& address)
{
    for(auto client : m_clients)
    {
        if( client.second.m_address == address )
            return client.second.m_uid;
    }
    return -1;
}
//get Client by address
server_listener::client* server_listener::get_client_by_address(const server_listener::system_address& address)
{
    return get(get_uid_by_address(address));
}
//get id client
server_listener::client* server_listener::get(server_listener::UID client)
{
    auto it = m_clients.find(client);
    if(it != m_clients.end()) return &it->second;
    return nullptr;
}
//client exist?
bool server_listener::exists(server_listener::UID client) const
{
    auto it = m_clients.find(client);
    return (it != m_clients.end());
}
//close a connection
void server_listener::close_client_connection(server_listener::UID uid)
{
    client* l_client=get(uid);
    if(l_client) close_client_connection(*l_client);
}
//close a connection
void server_listener::close_client_connection(server_listener::client& client)
{
    m_peer->CloseConnection(client.m_address, true);
    on_disconnected(client);
    m_clients.erase(client.m_uid);
}
//number of clients
unsigned int server_listener::count_clients() const
{
    return (unsigned int)m_clients.size();
}
//utility method
void server_listener::call_all_on_message(server_listener::client& client,
                                          server_listener::bit_stream& stream)
{
    on_message(client, stream);
    //find callback
    if(client.m_callback)
    {
        client.m_callback(client, stream);
    }
}
//send message to client (by uid)
void server_listener::send(server_listener::UID uid_client,const std::vector<unsigned char>& byte,PacketReliability type)
{
    client* ptr_client = get(uid_client);
    if(ptr_client) send(*ptr_client,byte,type);
}
//send message to client (by object)
void server_listener::send(server_listener::client& client,const std::vector<unsigned char>& byte,PacketReliability type)
{
    bit_stream stream;
    stream.Write((message_id)ID_SERVER_MSG);
    stream.Write(client.m_uid);
    stream.WriteBits((const unsigned char*)(byte.data()),((bit_size_t)byte.size()) * 8);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, client.m_address, false);
}
//send message to client
void server_listener::send(server_listener::UID uid_client,const byte_vector_stream& byte,PacketReliability type)
{
	client* ptr_client = get(uid_client);
    if(ptr_client) send(*ptr_client,byte,type);
}
//send message to client
void server_listener::send(server_listener::client& client,const byte_vector_stream& byte,PacketReliability type)
{
	bit_stream stream;
    stream.Write((message_id)ID_SERVER_MSG);
    stream.Write(client.m_uid);
    stream.WriteBits((const unsigned char*)(byte.data()),((bit_size_t)byte.size()) * 8);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, client.m_address, false);
}
//send "c" message to client (by uid)
void server_listener::send(server_listener::UID uid_client,const unsigned char* msg,size_t size,PacketReliability type)
{
    client* ptr_client = get(uid_client);
    if(ptr_client) send(*ptr_client,msg,size,type);
}
//send "c" message to client (by object)
void server_listener::send(server_listener::client& client,const unsigned char* msg,size_t size,PacketReliability type)
{
	bit_stream stream;
    stream.Write((message_id)ID_SERVER_MSG);
    stream.Write(client.m_uid);
    stream.WriteBits(msg,((bit_size_t)size) * 8);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, client.m_address, false);
}
//send Rak message to client (by uid)
void server_listener::send(server_listener::UID uid_client,const server_listener::bit_stream& stream,PacketReliability type)
{
    client* ptr_client = get(uid_client);
    if(ptr_client) send(*ptr_client,stream,type);
}
//send Rak message to client (by object)
void server_listener::send(server_listener::client& client,const server_listener::bit_stream& instream,PacketReliability type)
{
	bit_stream stream;
    stream.Write((message_id)ID_SERVER_MSG);
    stream.Write(client.m_uid);
    stream.Write(instream);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, client.m_address, false);
}
//update sever
void server_listener::update()
{
    if(m_peer)
    {
        packet* l_packet = nullptr;

        for (l_packet = m_peer->Receive(); 
		     l_packet && m_peer;
             m_peer->DeallocatePacket(l_packet), 
		     l_packet = m_peer->Receive())
        {
            //get type
            message_id typeMsg= l_packet->data[0];
            //type of message
            switch (typeMsg)
            {
                case ID_REMOTE_DISCONNECTION_NOTIFICATION: DEBUG_MESSAGE("Another client has disconnected.\n"); break;
                case ID_REMOTE_CONNECTION_LOST: DEBUG_MESSAGE("Another client has lost the connection.\n"); break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION: DEBUG_MESSAGE("Another client has connected.\n"); break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:	DEBUG_MESSAGE("The server is full.\n"); break;
                case ID_CONNECTION_REQUEST_ACCEPTED: DEBUG_MESSAGE("Our connection request has been accepted.\n");break;
                    
                case ID_NEW_INCOMING_CONNECTION:
                {
                    DEBUG_MESSAGE("A connection is incoming.\n");
                    UID userid=add_new_client(l_packet->systemAddress);
                    client& l_client=*get(userid);
                    send_uid(l_client);
                    on_connected(l_client);
                }
                break;
                case ID_DISCONNECTION_NOTIFICATION:
                case ID_CONNECTION_LOST:
                {
                    DEBUG_MESSAGE("Another client has disconnected.\n");
                    client* l_client =get_client_by_address(l_packet->systemAddress);
                    if (l_client)
                    {
                        on_disconnected(*l_client);
                        m_clients.erase(l_client->m_uid);
                    }
                }
                break;
                    
                case ID_CLIENT_MSG:
                {
                    bit_stream bstream(l_packet->data, l_packet->length, false);
                    //ignore id message
                    bstream.IgnoreBytes(sizeof(RakNet::MessageID));
                    //get uid/client
                    UID client_uid{-1};
                    bstream.Read< UID >(client_uid);
                    //get client
                    client* l_client=get(client_uid);
                    //read message
                    bit_stream msg;
                    bstream.Read(msg);
                    //if exists
                    if(l_client) call_all_on_message(*l_client,msg);
                }
                break;
                    
                default:
                    DEBUG_MESSAGE("Message with identifier " << (int)typeMsg << " has arrived.\n");
                break;
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//enable connection
bool client_listener::connect(const std::string& addrServer,unsigned short port,  double timeOut)
{
    //close last connection
    close();
    //open connection
    m_peer = RakNet::RakPeerInterface::GetInstance();
    RakNet::SocketDescriptor sd;
    m_peer->Startup(1, &sd, 1);
    //address
    m_address.SetBinaryAddress((addrServer+":"+std::to_string(port)).c_str());
    //connection
    auto res=m_peer->Connect(addrServer.c_str(), port, 0, 0);
    m_connected = (res == RakNet::CONNECTION_ATTEMPT_STARTED);
    //return status
    return m_connected;
}
//enable disconnect
bool client_listener::disconnect()
{
    on_disconnected();
    close();
    m_connected = false;
    return true;
}
//get my andress
client_listener::system_address client_listener::get_my_address() const
{
    return m_peer->GetMyBoundAddress();
}
//get my andress
client_listener::system_address client_listener::get_server_address() const
{
    return m_address;
}
//get uid
client_listener::UID client_listener::get_uid() const
{
    return m_uid;
}
//send message to client
void client_listener::send(const std::vector<unsigned char>& byte,PacketReliability type)
{
    bit_stream stream;
    stream.Write((message_id)ID_CLIENT_MSG);
    stream.Write< UID >(m_uid);
    stream.WriteBits((const unsigned char*)(byte.data()),((bit_size_t)byte.size()) * 8);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, m_address, false);
}
//send message to client
void client_listener::send(const byte_vector_stream& byte,PacketReliability type)
{
	bit_stream stream;
    stream.Write((message_id)ID_CLIENT_MSG);
    stream.Write< UID >(m_uid);
    stream.WriteBits((const unsigned char*)(byte.data()),((bit_size_t)byte.size()) * 8);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, m_address, false);
}
//send "c" message to client
void client_listener::send(const unsigned char* data,size_t size,PacketReliability type)
{
	bit_stream stream;
    stream.Write((message_id)ID_CLIENT_MSG);
    stream.Write< UID >(m_uid);
    stream.WriteBits(data,((bit_size_t)size) * 8);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, m_address, false);
}
//send Rak message to client
void client_listener::send(const bit_stream& instream,PacketReliability type)
{
	bit_stream stream;
    stream.Write((message_id)ID_CLIENT_MSG);
    stream.Write< UID >(m_uid);
    stream.Write(instream);
    m_peer->Send(&stream, HIGH_PRIORITY, type, 0, m_address, false);
}
//close connection
void client_listener::close()
{
    if (m_peer)
    {
        if (m_connected)
        {
            m_peer->CloseConnection(m_peer->GetMyGUID(),true);
        }
        m_connected = false;
        RakNet::RakPeerInterface::DestroyInstance(m_peer);
    }
    m_peer = nullptr;
}
//update client
void client_listener::update()
{
    if(m_peer)
    {
        packet* l_packet = nullptr;
        for (l_packet = m_peer->Receive(); 
		     l_packet && m_peer;
             m_peer->DeallocatePacket(l_packet), 
			 l_packet = m_peer->Receive())
        {
            
            switch (l_packet->data[0])
            {
                case ID_REMOTE_DISCONNECTION_NOTIFICATION: DEBUG_MESSAGE("Another client has disconnected.\n"); break;
                case ID_REMOTE_CONNECTION_LOST: DEBUG_MESSAGE("Another client has lost the connection.\n"); break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION: DEBUG_MESSAGE("Another client has connected.\n"); break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:	DEBUG_MESSAGE("The server is full.\n"); break;
                case ID_CONNECTION_REQUEST_ACCEPTED: DEBUG_MESSAGE("Our connection request has been accepted.\n");break;
                    
                case ID_NEW_INCOMING_CONNECTION:
                {
                    DEBUG_MESSAGE("A connection is incoming.\n");
                    on_connected();
                }
                break;
                case ID_DISCONNECTION_NOTIFICATION:
                case ID_CONNECTION_LOST:
                {
                    DEBUG_MESSAGE("Another client has disconnected.\n");
                    on_disconnected();
                    m_connected = false;
                }
                break;
                case ID_SERVER_SEND_UID:
                {
                    bit_stream bstream(l_packet->data, l_packet->length, false);
                    bstream.IgnoreBytes(sizeof(RakNet::MessageID));
                    bstream.Read< UID >(m_uid);
                    on_get_uid(m_uid);
                }
                break;
                case ID_SERVER_MSG:
                {
                    //message uid
                    UID msgUid{ -1 };
					bit_stream bstream(l_packet->data, l_packet->length, false);
                    bstream.IgnoreBytes(sizeof(RakNet::MessageID));
                    bstream.Read< UID >(msgUid);
                    //read message
					bit_stream msg;
                    bstream.Read(msg);
                    //message uid == clinet uid?
                    if(msgUid==m_uid)
                    {
                        on_message(msg);
                    }
                    DEBUG_CODE(else{ DEBUG_MESSAGE("wrong uid message"); })
                }
                break;
                default:
                DEBUG_MESSAGE("Message with identifier " << std::to_string((int)l_packet->data[0]) << " has arrived..");
                break;
            }
        }
    }
}

