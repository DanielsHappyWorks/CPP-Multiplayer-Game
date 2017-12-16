#pragma once

#include "StateIdentifiers.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

#include <memory>

namespace sf
{
	class RenderWindow;
}

class StateStack;
class Player;
class MusicPlayer;
class SoundPlayer;
class KeyBinding;

class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{
		Context(sf::RenderWindow& window, TextureHolder& textures,
			FontHolder& fonts, MusicPlayer& music, SoundPlayer& sounds, KeyBinding& keys1, KeyBinding& keys2);
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;
		MusicPlayer* music;
		SoundPlayer* sounds;
		KeyBinding*	keys1;
		KeyBinding*	keys2;
	};

public:
	State(StateStack&, Context context);
	virtual ~State();

	virtual void draw() = 0;
	virtual bool update(sf::Time dt) = 0;
	virtual bool handleEvent(const sf::Event& event) = 0;

	virtual void		onActivate();
	virtual void		onDestroy();

protected:
	void requestStackPush(States::ID stateID);
	void requestStackPop();
	void requestStackClear();

	Context getContext() const;

private:
	StateStack* mStack;
	Context mContext;
};
