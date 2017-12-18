#pragma once
#include "State.hpp"
#include "KeyBinding.hpp"
#include "MusicPlayer.hpp"
#include "SoundPlayer.hpp"
#include "Container.hpp"
#include "Button.hpp"
#include "Label.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>


class OptionsState : public State
{
public:
	OptionsState(StateStack& stack, Context context);

	virtual void					draw();
	virtual bool					update(sf::Time dt);
	virtual bool					handleEvent(const sf::Event& event);


private:
	void							updateLabels();
	bool							isVsyncOn;
	void							setVsync();
	bool							isMuteMusicOn;
	void							setMuteMusic();
	bool							isMuteSoundOn;
	void							setMuteSound();


private:
	sf::Sprite											mBackgroundSprite;
	GUI::Container										mGUIContainer;
	std::array<GUI::Label::Ptr, 3>						mBindingLabels;
};