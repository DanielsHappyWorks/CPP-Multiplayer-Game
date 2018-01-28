#pragma once

#include "SceneNode.hpp"
#include "NetworkProtocol.hpp"

#include <queue>


class NetworkNode : public SceneNode
{
public:
	NetworkNode();

	void					notifyGameAction(GameActions::Type type, sf::Vector2f position);
	bool					pollGameAction(GameActions::Action& out);

	virtual unsigned int	getCategory() const;


private:
	std::queue<GameActions::Action>	mPendingActions;
};