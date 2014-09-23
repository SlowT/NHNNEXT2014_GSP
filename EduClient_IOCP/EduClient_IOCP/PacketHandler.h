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

	bool SendLoginRequest( ClientSession& remote, google::protobuf::int32 playerId );
	bool SendMoveToRequest( ClientSession& remote, google::protobuf::int32 playerId, float posX, float posY, float posZ );
	bool SendChatRequest( ClientSession& remote, google::protobuf::int32 playerId, const std::string& message );
	bool SendSightRequest( ClientSession& remote, google::protobuf::int32 playerId, float sightRadius );

private:
	void WriteMessageToStream( ClientPacket::MessageType msgType, const google::protobuf::MessageLite& message, google::protobuf::io::CodedOutputStream& stream );
};

extern PacketHandler* GPacketHandler;