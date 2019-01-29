#pragma once

#include	"Common.hpp"

#define		MAX_RECEIVE_CHARS 128

class		Server;


//////////////////////////////////////////////////
//	ServSideClient
//////////////////////////////////////////////////
class ServSideClient
{
public:

	ServSideClient	( SOCKET& acceptSocket, const std::string userName, Server* server );
	~ServSideClient	( void );

	//////////////////////////////////////////////////

	SOCKET&			GetSockRef		( void )	{ return *m_ClientSock; }
	uInt			GetPeerPort		( void )	{ return m_PeerPort; }
	std::string&	GetPeerIP		( void )	{ return m_PeerIP; }
	std::string&	GetMsg			( void )	{ return m_Msg; }
	std::string&	GetName			( void )	{ return m_UserName; }

	//////////////////////////////////////////////////

private:

	void	InitClientInfo			( void );

	void	ReceiveFromClientSide	( void );

	//////////////////////////////////////////////////

private:

	char		m_arrRecvMsg[ MAX_RECEIVE_CHARS ];

	std::string	m_Msg;
	std::string	m_UserName;
	std::string	m_PeerIP;

	std::thread	m_ThreadReceive;

	std::mutex	m_Mutex;

	uInt		m_PeerPort;

	SOCKET*		m_ClientSock;

	Server*		m_Server;	// No ownage // Don't delete, only nullptr

};