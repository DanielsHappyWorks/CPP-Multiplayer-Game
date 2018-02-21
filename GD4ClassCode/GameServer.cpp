#include "GameServer.hpp"
#include "NetworkProtocol.hpp"
#include "Foreach.hpp"
#include "Utility.hpp"
#include "Pickup.hpp"
#include "Character.hpp"
#include "DataTables.hpp"

#include <SFML/Network/Packet.hpp>

GameServer::RemotePeer::RemotePeer()
	: ready(false)
	, timedOut(false)
{
	socket.setBlocking(false);
}

GameServer::GameServer(sf::Vector2u windowSize)
	: mThread(&GameServer::executionThread, this)
	, mListeningState(false)
	, mClientTimeoutTime(sf::seconds(3.f))
	, mMaxConnectedPlayers(10)
	, mConnectedPlayers(0)
	, mWindowSize(windowSize)
	, mCharacterCount(0)
	, mPeers(1)
	, mCharacterIdentifierCounter(1)
	, mWaitingThreadEnd(false)
	, mLastSpawnTime(sf::Time::Zero)
	, mTimeForNextSpawn(sf::seconds(5.f))
{
	mListenerSocket.setBlocking(false);
	mPeers[0].reset(new RemotePeer());
	mThread.launch();
}

GameServer::~GameServer()
{
	mWaitingThreadEnd = true;
	mThread.wait();
}

void GameServer::notifyPlayerRealtimeChange(sf::Int32 characterIdentifier, sf::Int32 action, bool actionEnabled)
{
	for (std::size_t i = 0; i < mConnectedPlayers; ++i)
	{
		if (mPeers[i]->ready)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Server::PlayerRealtimeChange);
			packet << characterIdentifier;
			packet << action;
			packet << actionEnabled;

			mPeers[i]->socket.send(packet);
		}
	}
}

void GameServer::notifyPlayerEvent(sf::Int32 characterIdentifier, sf::Int32 action)
{
	for (std::size_t i = 0; i < mConnectedPlayers; ++i)
	{
		if (mPeers[i]->ready)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Server::PlayerEvent);
			packet << characterIdentifier;
			packet << action;

			mPeers[i]->socket.send(packet);
		}
	}
}

void GameServer::notifyPlayerSpawn(sf::Int32 characterIdentifier)
{
	for (std::size_t i = 0; i < mConnectedPlayers; ++i)
	{
		if (mPeers[i]->ready)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Server::PlayerConnect);
			packet << characterIdentifier << mCharacterInfo[characterIdentifier].position.x << mCharacterInfo[characterIdentifier].position.y;
			mPeers[i]->socket.send(packet);
		}
	}
}

void GameServer::setListening(bool enable)
{
	// Check if it isn't already listening
	if (enable)
	{
		if (!mListeningState)
			mListeningState = (mListenerSocket.listen(ServerPort) == sf::TcpListener::Done);
	}
	else
	{
		mListenerSocket.close();
		mListeningState = false;
	}
}

void GameServer::executionThread()
{
	setListening(true);

	sf::Time stepInterval = sf::seconds(1.f / 60.f);
	sf::Time stepTime = sf::Time::Zero;
	sf::Time tickInterval = sf::seconds(1.f / 20.f);
	sf::Time tickTime = sf::Time::Zero;
	sf::Clock stepClock, tickClock;

	while (!mWaitingThreadEnd)
	{
		handleIncomingPackets();
		handleIncomingConnections();

		//stepTime += stepClock.getElapsedTime();
		//stepClock.restart();

		tickTime += tickClock.getElapsedTime();
		tickClock.restart();

		// Fixed update step
		//while (stepTime >= stepInterval)
		//{
			//stepTime -= stepInterval;
		//}

		// Fixed tick step
		while (tickTime >= tickInterval)
		{
			tick();
			tickTime -= tickInterval;
		}

		// Sleep to prevent server from consuming 100% CPU
		sf::sleep(sf::milliseconds(100));
	}
}

void GameServer::tick()
{
	updateClientState();

	//Remove IDs of character that have been destroyed (relevant if a client has two, and loses one)
	/*
	for (auto itr = mCharacterInfo.begin(); itr != mCharacterInfo.end(); )
	{
		if (itr->second.hitpoints <= 0)
			mCharacterInfo.erase(itr++);
		else
			++itr;
	}*/
}

sf::Time GameServer::now() const
{
	return mClock.getElapsedTime();
}

void GameServer::handleIncomingPackets()
{
	bool detectedTimeout = false;

	FOREACH(PeerPtr& peer, mPeers)
	{
		if (peer->ready)
		{
			sf::Packet packet;
			while (peer->socket.receive(packet) == sf::Socket::Done)
			{
				// Interpret packet and react to it
				handleIncomingPacket(packet, *peer, detectedTimeout);

				// Packet was indeed received, update the ping timer
				peer->lastPacketTime = now();
				packet.clear();
			}

			if (now() >= peer->lastPacketTime + mClientTimeoutTime)
			{
				peer->timedOut = true;
				detectedTimeout = true;
			}
		}
	}

	if (detectedTimeout)
		handleDisconnections();
}

void GameServer::handleIncomingPacket(sf::Packet& packet, RemotePeer& receivingPeer, bool& detectedTimeout)
{
	sf::Int32 packetType;
	packet >> packetType;

	switch (packetType)
	{
	case Client::Quit:
	{
		receivingPeer.timedOut = true;
		detectedTimeout = true;
	} break;

	case Client::PlayerEvent:
	{
		sf::Int32 characterIdentifier;
		sf::Int32 action;
		packet >> characterIdentifier >> action;

		notifyPlayerEvent(characterIdentifier, action);
	} break;

	case Client::PlayerRealtimeChange:
	{
		sf::Int32 characterIdentifier;
		sf::Int32 action;
		bool actionEnabled;
		packet >> characterIdentifier >> action >> actionEnabled;
		mCharacterInfo[characterIdentifier].realtimeActions[action] = actionEnabled;
		notifyPlayerRealtimeChange(characterIdentifier, action, actionEnabled);
	} break;

	case Client::PositionUpdate:
	{
		sf::Int32 numCharacters;
		packet >> numCharacters;

		for (sf::Int32 i = 0; i < numCharacters; ++i)
		{
			sf::Int32 characterIdentifier;
			sf::Int32 characterHitpoints;
			sf::Int32 missileAmmo;
			float characterKnockback;
			sf::Vector2f characterPosition;
			packet >> characterIdentifier >> characterPosition.x >> characterPosition.y >> characterHitpoints >> missileAmmo >> characterKnockback;
			//std::cout <<  characterIdentifier << " position x: " << characterPosition.x << " position y: " << characterPosition.y << " hitpoints: " << characterHitpoints << " missle ammo: " << missileAmmo << " knockback: " << characterKnockback <<std::endl;
			mCharacterInfo[characterIdentifier].position = characterPosition;
			mCharacterInfo[characterIdentifier].hitpoints = characterHitpoints;
			mCharacterInfo[characterIdentifier].missileAmmo = missileAmmo;
			mCharacterInfo[characterIdentifier].knockback = characterKnockback;
		}
	} break;
	}
}

void GameServer::updateClientState()
{
	sf::Packet updateClientStatePacket;
	updateClientStatePacket << static_cast<sf::Int32>(Server::UpdateClientState);
	updateClientStatePacket << static_cast<sf::Int32>(mCharacterInfo.size());

	FOREACH(auto character, mCharacterInfo) {
		updateClientStatePacket << character.first << character.second.position.x << character.second.position.y << character.second.hitpoints << character.second.missileAmmo << character.second.knockback;
		//std::cout << character.first << " position x: " << character.second.position.x << " position y: " << character.second.position.y << " hitpoints: " << character.second.hitpoints << " missle ammo: " << character.second.missileAmmo << " knockback: " << character.second.knockback << std::endl;

	}
	sendToAll(updateClientStatePacket);
}

void GameServer::handleIncomingConnections()
{
	if (!mListeningState)
		return;

	if (mListenerSocket.accept(mPeers[mConnectedPlayers]->socket) == sf::TcpListener::Done)
	{
		// order the new client to spawn its own plane ( player 1 )
		mCharacterInfo[mCharacterIdentifierCounter].position = sf::Vector2f(mWindowSize.x / 2, mWindowSize.y / 2);
		mCharacterInfo[mCharacterIdentifierCounter].hitpoints = 100;
		mCharacterInfo[mCharacterIdentifierCounter].missileAmmo = 2;
		mCharacterInfo[mCharacterIdentifierCounter].knockback = 0;

		sf::Packet packet;
		packet << static_cast<sf::Int32>(Server::SpawnSelf);
		packet << mCharacterIdentifierCounter;
		packet << mCharacterInfo[mCharacterIdentifierCounter].position.x;
		packet << mCharacterInfo[mCharacterIdentifierCounter].position.y;

		mPeers[mConnectedPlayers]->characterIdentifiers.push_back(mCharacterIdentifierCounter);

		broadcastMessage("New player!");
		informWorldState(mPeers[mConnectedPlayers]->socket);
		notifyPlayerSpawn(mCharacterIdentifierCounter++);

		mPeers[mConnectedPlayers]->socket.send(packet);
		mPeers[mConnectedPlayers]->ready = true;
		mPeers[mConnectedPlayers]->lastPacketTime = now(); // prevent initial timeouts
		mCharacterCount++;
		mConnectedPlayers++;

		if (mConnectedPlayers >= mMaxConnectedPlayers)
			setListening(false);
		else // Add a new waiting peer
			mPeers.push_back(PeerPtr(new RemotePeer()));
	}
}

void GameServer::handleDisconnections()
{
	for (auto itr = mPeers.begin(); itr != mPeers.end(); )
	{
		if ((*itr)->timedOut)
		{
			// Inform everyone of the disconnection, erase 
			FOREACH(sf::Int32 identifier, (*itr)->characterIdentifiers)
			{
				sendToAll(sf::Packet() << static_cast<sf::Int32>(Server::PlayerDisconnect) << identifier);

				mCharacterInfo.erase(identifier);
			}

			mConnectedPlayers--;
			mCharacterCount -= (*itr)->characterIdentifiers.size();

			itr = mPeers.erase(itr);

			// Go back to a listening state if needed
			if (mConnectedPlayers < mMaxConnectedPlayers)
			{
				mPeers.push_back(PeerPtr(new RemotePeer()));
				setListening(true);
			}

			broadcastMessage("An oponent has disconnected.");
		}
		else
		{
			++itr;
		}
	}
}

// Tell the newly connected peer about how the world is currently
void GameServer::informWorldState(sf::TcpSocket& socket)
{
	sf::Packet packet;
	packet << static_cast<sf::Int32>(Server::InitialState);
	packet << static_cast<sf::Int32>(mCharacterCount);

	for (std::size_t i = 0; i < mConnectedPlayers; ++i)
	{
		if (mPeers[i]->ready)
		{
			FOREACH(sf::Int32 identifier, mPeers[i]->characterIdentifiers)
				packet << identifier << mCharacterInfo[identifier].position.x << mCharacterInfo[identifier].position.y << mCharacterInfo[identifier].hitpoints << mCharacterInfo[identifier].missileAmmo << mCharacterInfo[identifier].knockback;
		}
	}

	socket.send(packet);
}

void GameServer::broadcastMessage(const std::string& message)
{
	for (std::size_t i = 0; i < mConnectedPlayers; ++i)
	{
		if (mPeers[i]->ready)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Server::BroadcastMessage);
			packet << message;

			mPeers[i]->socket.send(packet);
		}
	}
}

void GameServer::sendToAll(sf::Packet& packet)
{
	FOREACH(PeerPtr& peer, mPeers)
	{
		if (peer->ready)
			peer->socket.send(packet);
	}
}
