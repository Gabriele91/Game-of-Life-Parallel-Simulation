//
//  rak_listener.h
//  BallsWars
//
//  Created by Gabriele Di Bari on 14/08/15.
//  Copyright (c) 2015 Gabriele Di Bari. All rights reserved.

#pragma once
#include <map>
#include <array>
#include <vector>
#include <functional>
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/BitStream.h>
#include <RakNet/RakNetTypes.h>

enum RAK_ENUM_MESSAGE
{
    ID_NONE = ID_USER_PACKET_ENUM + 1,
    ID_SEND_UID,
    ID_SAY_GOODBYE,
    ID_SERVER_SEND_UID,
    ID_SERVER_MSG,
    ID_CLIENT_MSG
};
 
class byte_vector_stream : protected std::vector< unsigned char >
{
private:
        
	unsigned char* m_ptr { nullptr };
    //data remaning
    size_t remaining() const
    {
        return data()+size() - m_ptr;
    }
    size_t offset() const
    {
        return m_ptr - data();
    }
    //realloc
    void required(size_t rsize)
    {
        if(remaining() < rsize)
        {
            //offset
            size_t ptrpos = offset();
            //resize/realloc
            resize(size()+rsize-remaining());
            //reset ptr
			m_ptr = data() + ptrpos;
        }
    }
        
public:
        
    byte_vector_stream()
    {
        reset();
    }
        
    template< typename... Args >
    byte_vector_stream(Args&&... args)
    {
        reset();
        add(args...);
    }
        
    template < typename T, typename... Args >
    void add(const T& value, Args&&... args)
    {
        add(value);
        add(args...);
    }
        
    template < typename T, typename... Args >
    void get(T& value, Args&&... args)
    {
        get(value);
        get(args...);
    }
        
    template < typename T >
    void add(const T& value)
    {
        required(sizeof(T));
        (*((T*)m_ptr)) = value;
        m_ptr+=sizeof(T);
    }
        
    template < typename T >
    void get(T& value)
    {
        value = (*((T*)m_ptr));
        m_ptr+=sizeof(T);
    }
        
    void reset()
    {
        m_ptr = data();
    }
        
    unsigned char* data()
    {
        return std::vector< unsigned char >::data();
    }
        
    const unsigned char* data() const
    {
        return std::vector< unsigned char >::data();
    }
        
    size_t size() const
    {
        return std::vector< unsigned char >::size();
    }
        
    void clear()
    {
        std::vector< unsigned char >::clear();
        reset();
    }
        
};
    
class server_listener
{
        
public:
    //using
    using system_address     = RakNet::SystemAddress;
    using bit_stream         = RakNet::BitStream;
    using message_id         = RakNet::MessageID;
    using packet             = RakNet::Packet;
    using bit_size_t         = RakNet::BitSize_t;
    //fw declaretion
    struct client;
    //type id
    typedef short UID;
    //type of callback
    typedef std::function< void (client& client,
                                    bit_stream& stream) >  callback_on_msg;
    //client struct
    struct client
    {
        UID m_uid{ -1 };
        system_address m_address;
		callback_on_msg m_callback;
        void* m_data{ nullptr };

		client(){}
		client(UID uid,
			   const system_address& address,
			   callback_on_msg callback,
			   void* data)
		{
			m_uid = uid;
			m_address = address;
			m_callback = callback;
			m_data = data;
		}
    };
        
    //open connection
    void open(unsigned short port,
              double time_out,
              int maximumIncomingConnections);
    //close connection
    void close();
    //safe close
    virtual ~server_listener(){ close(); }
    //number of connections
    int get_maximum_incoming_connections() const;
    //get my andress
    system_address get_my_address() const;
    //new client
    UID add_new_client(system_address&);
    //get id by address
    UID get_uid_by_address(const system_address&);
    //get Client by address
    client* get_client_by_address(const system_address&);
    //get id client
    client* get(UID client);
    //client exist?
    bool exists(UID client) const;
    //close a connection
    void close_client_connection(UID client);
    //close a connection
    void close_client_connection(client& client);
    //number of clients
    unsigned int count_clients() const;
    //send message to client (by uid)
    void send(UID client,const std::vector<unsigned char>& byte,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send message to client (by object)
    void send(client& client,const std::vector<unsigned char>& byte,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send message to client (by uid)
    void send(UID client,const byte_vector_stream& byte,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send message to client (by object)
    void send(client& client,const byte_vector_stream& byte,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send "c" message to client (by uid)
    void send(UID client,const unsigned char* msg,size_t size,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send "c" message to client (by object)
    void send(client& client,const unsigned char* msg,size_t size,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send Rak message to client (by uid)
    void send(UID client,const bit_stream& stream,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send Rak message to client (by object)
    void send(client& client,const bit_stream& stream,
                PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send T message (by uid)
    template < class T > void send(UID client,
                                   const T& msg,
                                   PacketReliability type = PacketReliability::RELIABLE_ORDERED)
    {
        send(client,&msg,sizeof(T),type);
    }
    //send T message (by object)
    template < class T > void send(client& client,
                                   const T& msg,
                                   PacketReliability type = PacketReliability::RELIABLE_ORDERED)
    {
        send(client,&msg,sizeof(T),type);
    }
    //update sever
    virtual void update();
    //connection.....
    virtual void on_connected(client& client){ }
    virtual void on_message(client& client, bit_stream& stream){ }
    virtual void on_disconnected(client& client){ }
        
protected:
    //connection
    RakNet::RakPeerInterface *m_peer{ nullptr };
    //client map
    std::map< UID, client > m_clients;
    //id generator
    UID m_uid_generator{ 0 };
    //new id
    UID new_uid();
    //send uid to client
    void send_uid(client& client);
    //callback all onMessage
    void call_all_on_message(client& client, bit_stream& stream);
};
    
class client_listener
{
public:
    //using
    using system_address     = RakNet::SystemAddress;
    using bit_stream         = RakNet::BitStream;
    using message_id         = RakNet::MessageID;
    using packet             = RakNet::Packet;
    using bit_size_t         = RakNet::BitSize_t;
    using UID                = server_listener::UID;
    //enable connection
    bool connect(const std::string& addrServer,unsigned short port,  double timeOut);
    //disconnect
    bool disconnect();
    //safe close
    virtual ~client_listener(){ close(); }
    //update
    void update();
    //actions
    virtual void on_connected(){ }
    virtual void on_get_uid(UID uid){ }
    virtual void on_message(bit_stream& stream){ }
    virtual void on_disconnected(){ }
    //get my andress
    system_address get_my_address() const;
    //get my andress
    system_address get_server_address() const;
    //get uid
    UID get_uid() const;
    //send message to client
    void send(const std::vector<unsigned char>& byte,PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send message to client
    void send(const byte_vector_stream& byte,PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send "c" message to client
    void send(const unsigned char* msg,size_t size,PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send Rak message to client
    void send(const bit_stream& stream,PacketReliability type = PacketReliability::RELIABLE_ORDERED);
    //send T message
    template < class T > void send(const T& msg,PacketReliability type = PacketReliability::RELIABLE_ORDERED)
    {
        send(&msg,sizeof(T),type);
    }
        
protected:
        
    //close connection
    void close();
    //peer
    RakNet::RakPeerInterface *m_peer{ nullptr };
    //connection status
    bool m_connected{ false };
    //server address
    system_address m_address;
    //uid
    UID m_uid{ -1 };
        
};