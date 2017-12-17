#pragma once

#include "Command.hpp"
#include "KeyBinding.hpp"

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Network/TcpSocket.hpp>

#include <map>


class CommandQueue;

class Player : private sf::NonCopyable
{
public:
	typedef PlayerAction::Type Action;

	enum MissionStatus
	{
		MissionRunning,
		MissionSuccess,
		MissionFailure
	};


public:
	Player(sf::TcpSocket* socket, sf::Int32 identifier, const KeyBinding* binding);

	void					handleEvent(const sf::Event& event, CommandQueue& commands);
	void					handleRealtimeInput(CommandQueue& commands);
	void 					setMissionStatus(MissionStatus status);
	MissionStatus 			getMissionStatus() const;

	/*
	void					disableAllRealtimeActions();
	*/
	bool					isLocal() const;

private:
	void					initializeActions();


private:
	const KeyBinding*			mKeyBinding;
	std::map<Action, Command>	mActionBinding;
	std::map<Action, bool>		mActionProxies;
	MissionStatus 				mCurrentMissionStatus;
	int							mIdentifier;
	sf::TcpSocket*				mSocket;
};