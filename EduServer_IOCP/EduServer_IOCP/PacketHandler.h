#pragma once
#include "ClientPacket.pb.h"
#include "XTL.h"

class ClientSession;
class Player;

class PacketHandler
{
public:
	PacketHandler();
	virtual ~PacketHandler();

	void RecvPacketProcess( ClientSession& remote, const unsigned char* buffer, int size );
	bool SendPacketProcess( ClientSession& remote, ClientPacket::MessageType type, const google::protobuf::MessageLite& data );

	bool SendLoginResult( ClientSession& remote, google::protobuf::int32 playerId, const std::string& playerName, const ClientPacket::Position& playerPos );
	bool SendMoveResult( ClientSession& remote, google::protobuf::int32 playerId, const ClientPacket::Position& playerPos );
	bool SendMoveResult( ClientSession& remote, google::protobuf::int32 playerId, float posX, float posY, float posZ );
	bool SendChatResult( ClientSession& remote, const std::string& playerName, const std::string& message );
	bool SendSightResult( ClientSession& remote, xvector<Player*>::type& inSightList );

private:
	void WriteMessageToStream( ClientPacket::MessageType msgType, const google::protobuf::MessageLite& message, google::protobuf::io::CodedOutputStream& stream );
};

extern PacketHandler* GPacketHandler;