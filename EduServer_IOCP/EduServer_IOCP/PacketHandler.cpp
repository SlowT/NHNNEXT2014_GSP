#include "stdafx.h"
#include "PacketHandler.h"
#include "ClientSession.h"
#include "PlayerManager.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

PacketHandler* GPacketHandler = nullptr;

struct MessageHeader
{
	MessageHeader() : size( 0 ), type( -1 ) {}
	short size;
	short type;
};
const size_t MessageHeaderSize = sizeof( MessageHeader );

PacketHandler::PacketHandler()
{
}


PacketHandler::~PacketHandler()
{
}

void PacketHandler::RecvPacketProcess( ClientSession& remote, const unsigned char* buffer, int size )
{
	google::protobuf::io::CodedInputStream inputStream( buffer, size );

	MessageHeader header;
	while( inputStream.ReadRaw( &header, MessageHeaderSize ) )
	{
		// 직접 억세스 할수 있는 버퍼 포인터와 남은 길이를 알아냄
		const void* payload_ptr = nullptr;
		int payloadSize = 0;

		inputStream.GetDirectBufferPointer( &payload_ptr, &payloadSize );
		if( payloadSize < (signed)header.size )
			break;

		google::protobuf::io::CodedInputStream payLoadStream( (const google::protobuf::uint8*)payload_ptr, header.size );

		// 메세지 종류별로 역직렬화해서 적절한 메서드를 호출해줌
		switch( header.type )
		{
		case ClientPacket::PKT_CS_MOVE:
		{
			ClientPacket::MoveRequest message;
			if( false == message.ParseFromCodedStream( &payLoadStream ) )
				break;

			if( message.playerid() != remote.mPlayer.GetPlayerId() ) // 통신 중인 세션의 플레이어와 패킷의 플레이어가 같아야 정상이다.
				break;

			const ClientPacket::Position pos( message.playerpos() );
			remote.mPlayer.ResponseUpdatePosition( pos.x(), pos.y(), pos.z() );
		}
			break;
		case ClientPacket::PKT_CS_SIGHT:
		{
			ClientPacket::SightRequest message;
			if( false == message.ParseFromCodedStream( &payLoadStream ) )
				break;

			if( message.playerid() != remote.mPlayer.GetPlayerId() )
				break;

			

			remote.mPlayer.ViewSight();
		}
			break;
		case ClientPacket::PKT_CS_CHAT:
		{
			ClientPacket::ChatRequest message;
			if( false == message.ParseFromCodedStream( &payLoadStream ) )
				break;

			if( message.playerid() != remote.mPlayer.GetPlayerId() )
				break;

			remote.mPlayer.Chat( message.playermessage() );
		}
			break;
		case ClientPacket::PKT_CS_LOGIN:
		{
			ClientPacket::LoginRequest message;
			if( false == message.ParseFromCodedStream( &payLoadStream ) )
				break;

			remote.mPlayer.RequestLoad( message.playerid() );
		}
			break;
		}
		inputStream.Skip( header.size );
	}
}

bool PacketHandler::SendPacketProcess( ClientSession& remote, ClientPacket::MessageType type, const google::protobuf::MessageLite& data )
{
	// 필요한 버퍼의 사이즈를 알아내서 버퍼를 할당
	int bufSize = 0;
	bufSize += MessageHeaderSize + data.ByteSize();
	google::protobuf::uint8* outputBuf = new google::protobuf::uint8[bufSize];

	// 메세지를 출력할 스트림 생성
	google::protobuf::io::ArrayOutputStream output_array_stream( outputBuf, bufSize );
	google::protobuf::io::CodedOutputStream output_coded_stream( &output_array_stream );

	// 메세지를 스트림에 쓴다
	WriteMessageToStream( type, data, output_coded_stream );

	return remote.PostSend( reinterpret_cast<char*>(outputBuf), bufSize );
}

void PacketHandler::WriteMessageToStream( ClientPacket::MessageType msgType, const google::protobuf::MessageLite& message, google::protobuf::io::CodedOutputStream& stream )
{
	MessageHeader messageHeader;
	messageHeader.size = message.ByteSize();
	messageHeader.type = msgType;
	stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
	message.SerializeToCodedStream( &stream );
}

bool PacketHandler::SendLoginResult( ClientSession& remote, google::protobuf::int32 playerId, const std::string& playerName, const ClientPacket::Position& playerPos )
{
	ClientPacket::LoginResult data;
	data.set_playerid( playerId );
	data.set_playername( playerName );
	ClientPacket::Position* tPos = new ClientPacket::Position( playerPos );
	data.set_allocated_playerpos( tPos );

	return SendPacketProcess( remote, ClientPacket::PKT_SC_LOGIN, data );
}

bool PacketHandler::SendMoveResult( ClientSession& remote, google::protobuf::int32 playerId, const ClientPacket::Position& playerPos )
{
	ClientPacket::MoveResult data;
	data.set_playerid( playerId );
	ClientPacket::Position* tPos = new ClientPacket::Position( playerPos );
	data.set_allocated_playerpos( tPos );

	return SendPacketProcess( remote, ClientPacket::PKT_SC_MOVE, data );
}

bool PacketHandler::SendMoveResult( ClientSession& remote, google::protobuf::int32 playerId, float posX, float posY, float posZ )
{
	ClientPacket::MoveResult data;
	data.set_playerid( playerId );
	ClientPacket::Position* tPos = new ClientPacket::Position();
	tPos->set_x( posX );
	tPos->set_y( posY );
	tPos->set_z( posZ );
	data.set_allocated_playerpos( tPos );

	return SendPacketProcess( remote, ClientPacket::PKT_SC_MOVE, data );
}

bool PacketHandler::SendChatResult( ClientSession& remote, const std::string& playerName, const std::string& message )
{
	ClientPacket::ChatResult data;
	data.set_playername( playerName );
	data.set_playermessage( message );

	return SendPacketProcess( remote, ClientPacket::PKT_SC_CHAT, data );
}

bool PacketHandler::SendSightResult( ClientSession& remote, PlayerList& inSightList )
{
	ClientPacket::SightResult data;
	ClientPacket::SightResult::InSightPlayer* inSightPlayer = nullptr;
	for( auto& player : inSightList ){
		inSightPlayer = data.add_insightplayer();
		
		std::string* tStr = new std::string( player->GetNameByString() );
		inSightPlayer->set_allocated_playername( tStr );

		ClientPacket::Position* tPos = new ClientPacket::Position();
		tPos->set_x( player->GetPosX() );
		tPos->set_y( player->GetPosY() );
		tPos->set_z( player->GetPosZ() );
		inSightPlayer->set_allocated_playerpos( tPos );
	}

	return SendPacketProcess( remote, ClientPacket::PKT_SC_SIGHT, data );
}
