#include "MultiplayerGameState.hpp"
#include "MusicPlayer.hpp"
#include "Foreach.hpp"
#include "Utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/IpAddress.hpp>

#include <fstream>


sf::IpAddress getAddressFromFile()
{
	{ // Try to open existing file (RAII block)
		std::ifstream inputFile("ip.txt");
		std::string ipAddress;
		if (inputFile >> ipAddress)
			return ipAddress;
	}

	// If open/read failed, create new file
	std::ofstream outputFile("ip.txt");
	std::string localAddress = "127.0.0.1";
	outputFile << localAddress;
	return localAddress;
}

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool isHost)
	: State(stack, context)
	, mWorld(*context.window, *context.fonts, *context.sounds, true)
	, mWindow(*context.window)
	, mTextureHolder(*context.textures)
	, mConnected(false)
	, mGameServer(nullptr)
	, mActiveState(true)
	, mHasFocus(true)
	, mHost(isHost)
	, mGameStarted(false)
	, mClientTimeout(sf::seconds(2.f))
	, mTimeSinceLastPacket(sf::seconds(0.f))
{
	mBroadcastText.setFont(context.fonts->get(Fonts::Main));
	mBroadcastText.setPosition(1024.f / 2, 100.f);

	// We reuse this text for "Attempt to connect" and "Failed to connect" messages
	mFailedConnectionText.setFont(context.fonts->get(Fonts::Main));
	mFailedConnectionText.setString("Attempting to connect...");
	mFailedConnectionText.setCharacterSize(35);
	mFailedConnectionText.setColor(sf::Color::White);
	centerOrigin(mFailedConnectionText);
	mFailedConnectionText.setPosition(mWindow.getSize().x / 2.f, mWindow.getSize().y / 2.f);

	// Render a "establishing connection" frame for user feedback
	mWindow.clear(sf::Color::Black);
	mWindow.draw(mFailedConnectionText);
	mWindow.display();
	mFailedConnectionText.setString("Could not connect to the remote server!");
	centerOrigin(mFailedConnectionText);

	sf::IpAddress ip;
	if (isHost)
	{
		mGameServer.reset(new GameServer(mWindow.getSize()));
		ip = "127.0.0.1";
	}
	else
	{
		ip = getAddressFromFile();
	}

	if (mSocket.connect(ip, ServerPort, sf::seconds(5.f)) == sf::TcpSocket::Done)
		mConnected = true;
	else
		mFailedConnectionClock.restart();

	mSocket.setBlocking(false);

	// Play game theme
	context.music->play(Music::MissionTheme);
}

void MultiplayerGameState::draw()
{
	if (mConnected)
	{
		mWorld.draw();

		// Broadcast messages in default view
		mWindow.setView(mWindow.getDefaultView());

		if (!mBroadcasts.empty())
			mWindow.draw(mBroadcastText);

		if (mLocalPlayerIdentifiers.size() < 2 && mPlayerInvitationTime < sf::seconds(0.5f))
			mWindow.draw(mPlayerInvitationText);
	}
	else
	{
		mWindow.draw(mFailedConnectionText);
	}
}

void MultiplayerGameState::onActivate()
{
	mActiveState = true;
}

void MultiplayerGameState::onDestroy()
{
	if (!mHost && mConnected)
	{
		// Inform server this client is dying
		sf::Packet packet;
		packet << static_cast<sf::Int32>(Client::Quit);
		mSocket.send(packet);
	}
}

bool MultiplayerGameState::update(sf::Time dt)
{
	// Connected to server: Handle all the network logic
	if (mConnected)
	{
		mWorld.update(dt);

		// Remove players whose characters were destroyed
		bool foundLocalPlane = false;
		for (auto itr = mPlayers.begin(); itr != mPlayers.end(); )
		{
			// Check if there are no more local planes for remote clients
			if (std::find(mLocalPlayerIdentifiers.begin(), mLocalPlayerIdentifiers.end(), itr->first) != mLocalPlayerIdentifiers.end())
			{
				foundLocalPlane = true;
			}

			if (!mWorld.getCharacter(itr->first))
			{
				itr = mPlayers.erase(itr);

				// No more players left: Mission failed
				if (mPlayers.empty())
					requestStackPush(States::GameOver);
			}
			else
			{
				++itr;
			}
		}

		if (!foundLocalPlane && mGameStarted)
		{
			requestStackPush(States::GameOver);
		}

		// Only handle the realtime input if the window has focus and the game is unpaused
		if (mActiveState && mHasFocus)
		{
			CommandQueue& commands = mWorld.getCommandQueue();
			FOREACH(auto& pair, mPlayers)
				pair.second->handleRealtimeInput(commands);
		}

		// Always handle the network input
		CommandQueue& commands = mWorld.getCommandQueue();
		FOREACH(auto& pair, mPlayers)
			pair.second->handleRealtimeNetworkInput(commands);

		// Handle messages from server that may have arrived
		sf::Packet packet;
		if (mSocket.receive(packet) == sf::Socket::Done)
		{
			mTimeSinceLastPacket = sf::seconds(0.f);
			sf::Int32 packetType;
			packet >> packetType;
			handlePacket(packetType, packet);
		}
		else
		{
			// Check for timeout with the server
			if (mTimeSinceLastPacket > mClientTimeout)
			{
				mConnected = false;

				mFailedConnectionText.setString("Lost connection to server");
				centerOrigin(mFailedConnectionText);

				mFailedConnectionClock.restart();
			}
		}

		updateBroadcastMessage(dt);

		// Time counter for blinking 2nd player text
		mPlayerInvitationTime += dt;
		if (mPlayerInvitationTime > sf::seconds(1.f))
			mPlayerInvitationTime = sf::Time::Zero;

		// Events occurring in the game
		GameActions::Action gameAction;
		while (mWorld.pollGameAction(gameAction))
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Client::GameEvent);
			packet << static_cast<sf::Int32>(gameAction.type);
			packet << gameAction.position.x;
			packet << gameAction.position.y;

			mSocket.send(packet);
		}

		// Regular position updates
		if (mTickClock.getElapsedTime() > sf::seconds(1.f / 20.f))
		{
			sf::Packet positionUpdatePacket;
			positionUpdatePacket << static_cast<sf::Int32>(Client::PositionUpdate);
			positionUpdatePacket << static_cast<sf::Int32>(mLocalPlayerIdentifiers.size());

			FOREACH(sf::Int32 identifier, mLocalPlayerIdentifiers)
			{
				if (Character* character = mWorld.getCharacter(identifier)) {
					positionUpdatePacket << identifier << character->getPosition().x << character->getPosition().y << static_cast<sf::Int32>(character->getHitpoints()) << static_cast<sf::Int32>(character->getMissileAmmo()) << static_cast<float>(character->getKnockback());
					//std::cout << "Position Up :" << identifier << " x: " << character->getPosition().x << "  y: " << character->getPosition().y << " hp: " << static_cast<sf::Int32>(character->getHitpoints()) << " m: " << static_cast<sf::Int32>(character->getMissileAmmo()) << " k: " << static_cast<float>(character->getKnockback()) << std::endl;
				}
			}

			mSocket.send(positionUpdatePacket);
			mTickClock.restart();
		}

		mTimeSinceLastPacket += dt;
	}

	// Failed to connect and waited for more than 5 seconds: Back to menu
	else if (mFailedConnectionClock.getElapsedTime() >= sf::seconds(5.f))
	{
		requestStackClear();
		requestStackPush(States::Menu);
	}

	return true;
}

void MultiplayerGameState::disableAllRealtimeActions()
{
	mActiveState = false;

	FOREACH(sf::Int32 identifier, mLocalPlayerIdentifiers)
		mPlayers[identifier]->disableAllRealtimeActions();
}

bool MultiplayerGameState::handleEvent(const sf::Event& event)
{
	// Game input handling
	CommandQueue& commands = mWorld.getCommandQueue();

	// Forward event to all players
	FOREACH(auto& pair, mPlayers)
		pair.second->handleEvent(event, commands);

	if (event.type == sf::Event::KeyPressed)
	{
		// Escape pressed, trigger the pause screen
		if (event.key.code == sf::Keyboard::Escape)
		{
			disableAllRealtimeActions();
			requestStackPush(States::NetworkPause);
		}
	}
	else if (event.type == sf::Event::GainedFocus)
	{
		mHasFocus = true;
	}
	else if (event.type == sf::Event::LostFocus)
	{
		mHasFocus = false;
	}

	return true;
}

void MultiplayerGameState::updateBroadcastMessage(sf::Time elapsedTime)
{
	if (mBroadcasts.empty())
		return;

	// Update broadcast timer
	mBroadcastElapsedTime += elapsedTime;
	if (mBroadcastElapsedTime > sf::seconds(2.5f))
	{
		// If message has expired, remove it
		mBroadcasts.erase(mBroadcasts.begin());

		// Continue to display next broadcast message
		if (!mBroadcasts.empty())
		{
			mBroadcastText.setString(mBroadcasts.front());
			centerOrigin(mBroadcastText);
			mBroadcastElapsedTime = sf::Time::Zero;
		}
	}
}

void MultiplayerGameState::handlePacket(sf::Int32 packetType, sf::Packet& packet)
{
	switch (packetType)
	{
		// Send message to all clients
	case Server::BroadcastMessage:
	{
		std::string message;
		packet >> message;
		mBroadcasts.push_back(message);

		// Just added first message, display immediately
		if (mBroadcasts.size() == 1)
		{
			mBroadcastText.setString(mBroadcasts.front());
			centerOrigin(mBroadcastText);
			mBroadcastElapsedTime = sf::Time::Zero;
		}
	} break;

	// Sent by the server to order to spawn player 1 character on connect
	case Server::SpawnSelf:
	{
		sf::Int32 characterIdentifier;
		sf::Vector2f characterPosition;
		packet >> characterIdentifier >> characterPosition.x >> characterPosition.y;

		Character* character = mWorld.addCharacter(characterIdentifier, characterPosition.x, characterPosition.y);

		mPlayers[characterIdentifier].reset(new Player(&mSocket, characterIdentifier, getContext().keys1));
		mLocalPlayerIdentifiers.push_back(characterIdentifier);

		mGameStarted = true;
	} break;

	// 
	case Server::PlayerConnect:
	{
		sf::Int32 characterIdentifier;
		sf::Vector2f characterPosition;
		packet >> characterIdentifier >> characterPosition.x >> characterPosition.y;

		Character* character = mWorld.addCharacter(characterIdentifier);

		mPlayers[characterIdentifier].reset(new Player(&mSocket, characterIdentifier, nullptr));
	} break;

	// 
	case Server::PlayerDisconnect:
	{
		sf::Int32 characterIdentifier;
		packet >> characterIdentifier;

		mWorld.removeCharacter(characterIdentifier);
		mPlayers.erase(characterIdentifier);
	} break;

	// 
	case Server::InitialState:
	{
		sf::Int32 characterCount;
		packet >> characterCount;
		for (sf::Int32 i = 0; i < characterCount; ++i)
		{
			sf::Int32 characterIdentifier;
			sf::Vector2f characterPosition;
			packet >> characterIdentifier >> characterPosition.x >> characterPosition.y;

			Character* character = mWorld.addCharacter(characterIdentifier, characterPosition.x, characterPosition.y);

			mPlayers[characterIdentifier].reset(new Player(&mSocket, characterIdentifier, nullptr));
		}
	} break;

	// Player event (like missile fired) occurs
	case Server::PlayerEvent:
	{
		sf::Int32 characterIdentifier;
		sf::Int32 action;
		packet >> characterIdentifier >> action;

		auto itr = mPlayers.find(characterIdentifier);
		if (itr != mPlayers.end())
			itr->second->handleNetworkEvent(static_cast<Player::Action>(action), mWorld.getCommandQueue());
	} break;

	// Player's movement or fire keyboard state changes
	case Server::PlayerRealtimeChange:
	{
		sf::Int32 characterIdentifier;
		sf::Int32 action;
		bool actionEnabled;
		packet >> characterIdentifier >> action >> actionEnabled;

		auto itr = mPlayers.find(characterIdentifier);
		if (itr != mPlayers.end())
			itr->second->handleNetworkRealtimeChange(static_cast<Player::Action>(action), actionEnabled);
	} break;
	// Mission successfully completed
	case Server::MissionSuccess:
	{
		requestStackPush(States::MissionSuccess);
	} break;

	case Server::UpdateClientState:
	{
		sf::Int32 characterCount;
		packet >> characterCount;

		for (sf::Int32 i = 0; i < characterCount; ++i)
		{
			sf::Vector2f characterPosition;
			sf::Int32 characterIdentifier;
			sf::Int32 characterHitpoints;
			sf::Int32 missileAmmo;
			float characterKnockback;
			packet >> characterIdentifier >> characterPosition.x >> characterPosition.y >> characterHitpoints >> missileAmmo >> characterKnockback;
			
			//std::cout << "Update Client from server:" << characterIdentifier << " x: " << characterPosition.x << "  y: " << characterPosition.y << " hp: " << characterHitpoints << " m: " << missileAmmo << " k: " << characterKnockback << std::endl;

			Character* character = mWorld.getCharacter(characterIdentifier);
			bool isLocalPlane = std::find(mLocalPlayerIdentifiers.begin(), mLocalPlayerIdentifiers.end(), characterIdentifier) != mLocalPlayerIdentifiers.end();
			if (character && !isLocalPlane)
			{
				sf::Vector2f interpolatedPosition = character->getPosition() + (characterPosition - character->getPosition()) * 0.1f;
				character->setPosition(characterPosition.x, characterPosition.y);
				character->setHitpoints(characterHitpoints);
				character->setMissileAmmo(missileAmmo);
				character->setKnockback(characterKnockback);
			}
		}
	} break;
	}
}
