#include "stdafx.h"
#include "PacketHandler.h"
#include "ClientSession.h"
#include "PlayerManager.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

PacketHandler* GPacketHandler = nullptr;

struct MessageHeader
{
	google::protobuf::uint32 size;
	ClientPacket::MessageType type;
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
		const void* payload_ptr = NULL;
		int remainSize = inputStream.BytesUntilLimit();
		if( remainSize < (signed)header.size )
			break;

		// 메세지 종류별로 역직렬화해서 적절한 메서드를 호출해줌
		switch( header.type )
		{
		case ClientPacket::PKT_SC_MOVE:
		{
			ClientPacket::MoveResult message;
			if( false == message.ParseFromCodedStream( &inputStream ) )
				break;

			int pid = message.playerid();
			const ClientPacket::Position pos( message.playerpos() );
			remote.mPlayer.ResultMoveTo( pid, pos.x(), pos.y(), pos.z() );
		}
			break;
		case ClientPacket::PKT_SC_CHAT:
		{
			ClientPacket::ChatResult message;
			if( false == message.ParseFromCodedStream( &inputStream ) )
				break;

			const std::string& name = message.playername();
			const std::string& chat = message.playermessage();
			remote.mPlayer.ResultChat( name, chat );
		}
			break;
		case ClientPacket::PKT_SC_LOGIN:
		{
			ClientPacket::LoginResult message;
			if( false == message.ParseFromCodedStream( &inputStream ) )
				break;

			int pid = message.playerid();
			std::string pName = message.playername();
			const ClientPacket::Position pos( message.playerpos() );
			remote.mPlayer.ResultLoad( pid, pName, pos.x(), pos.y(), pos.z() );
		}
			break;
		case ClientPacket::PKT_SC_SIGHT:
		{
			ClientPacket::SightResult message;
			if( false == message.ParseFromCodedStream( &inputStream ) )
				break;

			InSightPlayers& tList = remote.mPlayer.GetInSightList();
			tList.clear();
			InSightPlayer tPlayer;
			for( int i = 0; i < message.insightplayer_size(); ++i ){
				const ClientPacket::SightResult::InSightPlayer& tSrcPlayer = message.insightplayer(i);

				tPlayer.name = tSrcPlayer.playername();
				tPlayer.pos.x = tSrcPlayer.playerpos().x();
				tPlayer.pos.y = tSrcPlayer.playerpos().y();
				tPlayer.pos.z = tSrcPlayer.playerpos().z();

				tList.push_back( tPlayer );
			}
// 			remote.mPlayer.ResultSight( tList );
		}
			break;
		}
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

bool PacketHandler::SendLoginRequest( ClientSession& remote, google::protobuf::int32 playerId )
{
	ClientPacket::LoginRequest data;
	data.set_playerid( playerId );

	return SendPacketProcess( remote, ClientPacket::PKT_CS_LOGIN, data );
}

bool PacketHandler::SendMoveToRequest( ClientSession& remote, google::protobuf::int32 playerId, float posX, float posY, float posZ )
{
	ClientPacket::MoveRequest data;
	data.set_playerid( playerId );
	ClientPacket::Position* tPos = new ClientPacket::Position();
	tPos->set_x( posX );
	tPos->set_y( posY );
	tPos->set_z( posZ );
	data.set_allocated_playerpos( tPos );

	return SendPacketProcess( remote, ClientPacket::PKT_CS_MOVE, data );
}

bool PacketHandler::SendChatRequest( ClientSession& remote, google::protobuf::int32 playerId, const std::string& message )
{
	ClientPacket::ChatRequest data;
	data.set_playerid( playerId );
	data.set_playermessage( message );

	return SendPacketProcess( remote, ClientPacket::PKT_CS_CHAT, data );
}

bool PacketHandler::SendSightRequest( ClientSession& remote, google::protobuf::int32 playerId, float sightRadius )
{
	ClientPacket::SightRequest data;
	data.set_playerid( playerId );
	data.set_sightradius( sightRadius );

	return SendPacketProcess( remote, ClientPacket::PKT_CS_SIGHT, data );
}
