

#ifndef _NETWORK_H
#define _NETWORK_H

#include "template.h"

#include <string.h>
#include <vector>
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"  // MessageID

#define MAX_NAMELENGTH 16
#define MAX_CLIENTS 8
#define DEBUG_MESSAGES 0
#pragma warning (disable : 4530)

namespace Tmpl8
{
	enum MessageIds
	{
		ID_MESSAGE = ID_USER_PACKET_ENUM + 1,  // Message packet ID 1						 
		ID_PLAYER_CONNECT,
		ID_PLAYER_ACTION,
		ID_PLAYER_ADD,
		ID_PLAYER_DISCONNECT,
		ID_PLAYER_UPDATE,
		ID_PLAYER_INPUT,
		ID_PLAYER_QUIT,
		ID_CONNECTION_TIMEOUT,
		ID_FLAG_INTERACTION,
		ID_NAME_CHANGE,
		ID_COMMAND,
		ID_SCORE,
	};
	enum MServIDs
	{
		ID_UPDATE_MS = 146, // Careful, must equal masterservers enum
		ID_REQUEST_LIST,
		ID_RECEIVE_LIST
	};

	enum PlayerAction
	{
		JUMP, SHOOT, DEATH,
	};


	typedef unsigned int SystemAddress;

	struct Client
	{
		char idx;
		RakNet::SystemAddress adress;
		RakNet::RakNetGUID guid;
		int ping;
		char name[MAX_NAMELENGTH];
	};

	struct AvailableServer
	{
		RakNet::AddressOrGUID addr;
		RakNet::RakString name;
		int numPlayers;
	};
	class Server;
	class Network
	{
	public:
		friend class Server;
		enum State
		{
			STATE_DISCONNECTED,
			STATE_CONNECTED,
			STATE_CONNECTING,
			STATE_SERVER,
			STATE_BROWSER,
			STATE_QUIT
		};

		Network();
		~Network();
		void Init( bool server );
		bool ConnectToGame( RakNet::AddressOrGUID addr );
		bool ConnectToMS();

		void ReadMsg( RakNet::Packet* packet, char* buffer, int bufferSize );
		void SendMsg( const char* msg, RakNet::AddressOrGUID to, bool debug = false, bool broadcast = true );
		void ReadCmd( RakNet::Packet* packet, char* buffer, int bufferSize );
		void SendCmd( const char* cmd, RakNet::AddressOrGUID to, bool broadcast = false );
		void SendCmd( const char* cmd );

		/* Client Side */
		bool StartClient();
		void LoopClient();

		void SendPacket( const char* packet, const int size, unsigned char type, RakNet::AddressOrGUID dest, bool broadcast = false );
		void SendInput( bool forward, bool backward, bool left, bool right, float pitch, float yaw );
		void SendAction( bool fire, bool jump );
		void SendAction( PlayerAction action );

		void SendNameChange( std::string name );

		void RequestServerList();

		bool IsConnected();
		bool IsServer() { return m_state == STATE_SERVER;	}
		void Disconnect();
		void CloseNetworking();

		void IncrementNumPlayers() { m_numPlayers++; }
		void DecrementNumPlayers() { m_numPlayers--; }
		int GetNumPlayers() { return m_numPlayers; }

		Client& FindClient( RakNet::RakNetGUID systemAdress );
		Client& FindClient( int id ) { return m_clients[id]; }

		const RakNet::AddressOrGUID GetServerAdress() { return m_serverAdress; }
		State GetStatus() { return m_state; }
		bool isBrowsingServers() { return m_ms; }
		int GetPing( RakNet::AddressOrGUID target );
		int GetScore( int team );
		void Ping( RakNet::AddressOrGUID target );
		void SetMasterServerAddr( std::string addr );
		unsigned int GetServerPort() { return m_serverPort; }
		std::string m_localName;

		/* Server Side*/
		bool StartServer();
		std::vector<AvailableServer> m_serverList;
		Server* server;
		static Network* m_instance;
		static Network* Get() { if( m_instance ) return m_instance; else return m_instance = new Network(); }

	protected:

		void ReceiveServerList( RakNet::Packet* packet );
		void IdentifyMyself();
		int AssignClient( const RakNet::RakNetGUID guid );
		void OnClientJoin( RakNet::Packet* info );
		void OnClientExit( RakNet::Packet* info );
		void SendCorrection( float3 pos, float angle, RakNet::RakNetGUID guid );
		void ReceiveAction( RakNet::Packet* packet );
		void TakeCorrection( RakNet::Packet* data );
		void ReceiveInput( RakNet::Packet* packet );
		void TakeFlagInteraction( RakNet::Packet* packet );
		void TakeNameChange( RakNet::Packet* packet );
		void TakeScore( RakNet::Packet* packet );

		RakNet::SocketDescriptor m_socketDesc;
		RakNet::AddressOrGUID m_serverAdress;
		RakNet::AddressOrGUID m_msAdress;
		RakNet::RakPeerInterface* m_peer;

		State m_state;
		Client m_clients[MAX_CLIENTS];
		bool m_isServer, m_ms;
		unsigned int m_numPlayers;
		int m_scoreRed, m_scoreBlue;
		unsigned char GetPacketIdentifier( RakNet::Packet* packet );

		// config
		std::string m_masterServerAdress;
		unsigned int m_masterServerPort;
		unsigned int m_serverPort;
	};
};

#endif