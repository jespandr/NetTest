#include	"Server.hpp"


//////////////////////////////////////////////////
//	cTor
//////////////////////////////////////////////////
Server::Server()
	: m_ServerIP			( nullptr )
	, m_ServerPort			( 0 )
	, m_CurrentServState		( eServState::INIT )
	, m_ServerIsAlive		( true  )
	, m_ServerSockIsAlive	( false )
	, m_InitListenThread	( false )
{}


//////////////////////////////////////////////////
//	dTor
//////////////////////////////////////////////////
Server::~Server()
{}


//////////////////////////////////////////////////
//	Run
//////////////////////////////////////////////////
void
Server::Run( void )
{
	while( m_ServerIsAlive )
	{
		switch( m_CurrentServState )
		{
		case eServState::INIT:

			Init();

			break;

		case eServState::RUN:

			if( m_InitListenThread == true )
				CreateThreads();

			if( m_ThreadListen.joinable() )
				m_ThreadListen.join();
			if( m_ThreadDistributeMsg.joinable() )
				m_ThreadDistributeMsg.join();

			break;

		case eServState::END:

			std::string input;
			std::cerr << "Server failed. Retry ( 1 ) or terminate ( 2 ): ";
			std::cin >> input;

			( input == "1" ) ? m_CurrentServState = eServState::INIT : m_ServerIsAlive = false;

			break;

		}
	}
}


//////////////////////////////////////////////////
//	Init
//////////////////////////////////////////////////
void
Server::Init( void )
{
	WSAData wsaData;

	int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if( iResult != NO_ERROR )
	{
		std::cerr << "WSAStartup() failed with error: " << iResult << '\n';
		m_CurrentServState = eServState::END;
	}
	else
	{
		m_InitListenThread	= true;
		m_CurrentServState		= eServState::RUN;
	}
}


//////////////////////////////////////////////////
//	Listening
//////////////////////////////////////////////////
void Server::Listening( void )
{
	m_ServerSockIsAlive	= true;
	SOCKET* listenSock	= new SOCKET;


    *listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( *listenSock == INVALID_SOCKET )
	{
		std::cerr << "Listening socket failed with error: " << WSAGetLastError() << '\n';
		m_ServerSockIsAlive	= false;
	}
	else
		std::cerr << "Listening socket creation succeeded\n";


	 sockaddr_in serverInfo;
	 serverInfo.sin_family		= AF_INET;
	 serverInfo.sin_addr.s_addr	= inet_addr( m_ServerIP );
	 serverInfo.sin_port		= htons( m_ServerPort );


	int iResult = bind( *listenSock, ( SOCKADDR* )& serverInfo, sizeof( serverInfo ) );
	if( iResult == SOCKET_ERROR )
	{
		std::cerr << "Bind function failed with error. " << WSAGetLastError() << '\n';
		iResult = closesocket( *listenSock );
		if ( iResult == SOCKET_ERROR )
			std::cerr << " Closesocket function failed with error: " << WSAGetLastError() << '\n';

		m_ServerSockIsAlive	= false;
	}


	while( m_ServerSockIsAlive )
	{
		if( listen( *listenSock, SOMAXCONN ) == SOCKET_ERROR )
		{
			std::cerr << "Listen function failed with error: " << WSAGetLastError() << '\n';
			m_ServerSockIsAlive = false;
		}

		std::cerr << "Listening on socket...\n";


		SOCKET* clientSock = new SOCKET;
		std::cerr << "Waiting for client to connect...\n";


		*clientSock = accept( *listenSock, NULL, NULL );
		if ( *clientSock == INVALID_SOCKET)
		{
			std::cerr << "Accept failed with error: " << WSAGetLastError() << '\n';
			closesocket( *listenSock );
			m_ServerSockIsAlive = false;
		}
		else
		{
			std::cout << "Clients connected: " << m_VecServSideClient.size() + 1 << '\n';

			m_VecServSideClient.push_back( new ServSideClient( *clientSock, "No name set", this ) );
			clientSock = nullptr;
		}
	}


	if( listenSock )
	{
		closesocket( *listenSock );
		AUTO_DEL( listenSock );
	}

	m_CurrentServState = eServState::END;
}


//////////////////////////////////////////////////
//	DistributeMsgs
//////////////////////////////////////////////////
void
Server::DistributeMsg( void )
{
	while( m_ServerSockIsAlive )
	{
		m_Mutex.lock();

		while( m_QueueMsg.empty() )
			Sleep( 1 );

		if( m_QueueMsg.empty() )
			continue;


		ServSideClient* client		= m_QueueMsg.front();
		m_QueueMsg.pop();

		uInt clientPort				= client->GetPeerPort();
		std::string	clientUserName	= client->GetName();
		std::string clientMsg		= client->GetMsg();


		for( uInt i = 0; i < m_VecServSideClient.size(); ++i )
		{
			if( m_VecServSideClient[ i ]->GetPeerPort() == clientPort )
				continue;

			int	iResult = send( m_VecServSideClient[ i ]->GetSockRef(), clientMsg.c_str(), ( size_t )strlen( clientMsg.c_str() ), 0 );
			if( iResult == SOCKET_ERROR )
				std::cerr << "Send failed with error: " << WSAGetLastError() << '\n';
		}

		m_Mutex.unlock();
	}
}


//////////////////////////////////////////////////
//	CreateThread
//////////////////////////////////////////////////
void
Server::CreateThreads( void )
{
	m_ThreadListen			= std::thread( &Server::Listening, this );
	m_ThreadDistributeMsg	= std::thread( &Server::DistributeMsg, this );

	m_InitListenThread		= false;
}