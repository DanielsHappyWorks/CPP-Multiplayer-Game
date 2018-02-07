#include "GameState.hpp"
#include "MusicPlayer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

GameState::GameState(StateStack& stack, Context context)
	: State(stack, context)
	, mWorld(*context.window, *context.fonts, *context.sounds, false)
	, mPlayer(nullptr, 1, context.keys1)
	, mPlayer2(nullptr, 2, context.keys2)
{
	//add both players
	mWorld.addCharacter(1, 100.f, 100.f);
	mWorld.addCharacter(2, 500.f, 100.f);

	// Play game theme
	context.music->play(Music::MissionTheme);
}

void GameState::draw()
{
	mWorld.draw();
}

bool GameState::update(sf::Time dt)
{
	mWorld.update(dt);

	//check for winner of game
	int lastOne = mWorld.isLastOneStanding();
	if (lastOne == 1)
	{
		requestStackPush(States::MissionSuccess1);
	}
	else if (lastOne == 2) {
		requestStackPush(States::MissionSuccess2);
	}
	else if (lastOne == 0) {
		requestStackPush(States::MissionDraw);
	}

	CommandQueue& commands = mWorld.getCommandQueue();
	mPlayer.handleRealtimeInput(commands);
	mPlayer2.handleRealtimeInput(commands);

	return true;
}

bool GameState::handleEvent(const sf::Event& event)
{
	// Game input handling
	CommandQueue& commands = mWorld.getCommandQueue();
	mPlayer.handleEvent(event, commands);
	mPlayer2.handleEvent(event, commands);

	// Escape pressed, trigger the pause screen
	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
		requestStackPush(States::Pause);

	return true;
}