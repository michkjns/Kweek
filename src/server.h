

#ifndef _SERVER_H
#define _SERVER_H

#include "network.h"

namespace Tmpl8
{
	class Server
	{
	public:
		Server( Network* network );
		~Server();
		void Init();
		void Main();
		void ReceiveInput( const RakNet::Packet* packet );
		void ReceiveAction( const RakNet::Packet* packet );
		void SendAction( PlayerAction action, RakNet::RakNetGUID guid );
		void SendAction( PlayerAction action, int pid );

		void FlagInteraction( int flag, int pid );
		void OnScore( int team );
		void SendScore();
		void UpdateStats();

		void SendChat( char* msg, char* name );
		void AcceptClient( const RakNet::Packet* packet );
		int AssignClient( const RakNet::RakNetGUID guid);
		void OnClientQuit( const RakNet::RakNetGUID guid, const int reason );
		Client FindClient( const RakNet::RakNetGUID guid );
		void BroadcastClient( const Client& client, bool disconnect = false);

		void AcceptNameChange( const RakNet::Packet* packet );
		void ReadCmd( RakNet::Packet* packet, char* buffer, int bufferSize );
		void UpdateMS();
		Network* m_network;
		RakNet::AddressOrGUID m_serverAdress;
		Timer snapshotTimer, msPingTimer;

		int m_scoreRed, m_scoreBlue;

		static Server instance;
	};
};

#endif