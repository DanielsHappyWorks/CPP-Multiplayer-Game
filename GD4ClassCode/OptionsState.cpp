#include "OptionsState.hpp"
#include "Utility.hpp"
#include "ResourceHolder.hpp"

#include <SFML/Graphics/RenderWindow.hpp>


OptionsState::OptionsState(StateStack& stack, Context context)
	: State(stack, context)
	, mGUIContainer()
	, isVsyncOn(true)
	, isMuteSoundOn(true)
	, isMuteMusicOn(true)
{
	mBackgroundSprite.setTexture(context.textures->get(Textures::TitleScreen));

	auto backButton = std::make_shared<GUI::Button>(context);
	backButton->setPosition(80.f, 620.f);
	backButton->setText("Back");
	backButton->setCallback(std::bind(&OptionsState::requestStackPop, this));


	auto vSyncButton = std::make_shared<GUI::Button>(context);
	vSyncButton->setPosition(320.f, 620.f);
	vSyncButton->setText("vsync");
	vSyncButton->setCallback([this]()
	{
		//vsyncLable = app.toggleVsync();
		setVsync();
	});

	auto muteButtonMusic = std::make_shared<GUI::Button>(context);
	muteButtonMusic->setPosition(80.f, 420.f);
	muteButtonMusic->setText("mute music");
	muteButtonMusic->setCallback([this]()
	{
		//vsyncLable = app.toggleVsync();
		setMuteMusic();
	});

	auto muteButtonSound = std::make_shared<GUI::Button>(context);
	muteButtonSound->setPosition(80.f, 220.f);
	muteButtonSound->setText("mute sounds");
	muteButtonSound->setCallback([this]()
	{
		//vsyncLable = app.toggleVsync();
		setMuteSound();
	});

	//vsync
	mBindingLabels[0] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[0]->setPosition(540.f, 620.f);
	//mute Music
	mBindingLabels[1] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[1]->setPosition(160.f, 420.f);

	//mute Sounds
	mBindingLabels[2] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[2]->setPosition(160.f, 220.f);


	mGUIContainer.pack(backButton);
	mGUIContainer.pack(vSyncButton);
	mGUIContainer.pack(muteButtonMusic);
	mGUIContainer.pack(muteButtonSound);
	mGUIContainer.pack(mBindingLabels[0]);
	mGUIContainer.pack(mBindingLabels[1]);
	mGUIContainer.pack(mBindingLabels[2]);
}

void OptionsState::draw()
{
	sf::RenderWindow& window = *getContext().window;

	window.draw(mBackgroundSprite);
	window.draw(mGUIContainer);
}

bool OptionsState::update(sf::Time)
{
	updateLabels();
	return true;
}

bool OptionsState::handleEvent(const sf::Event& event)
{
	mGUIContainer.handleEvent(event);
	return false;
}

void OptionsState::updateLabels()
{
	if (isVsyncOn)
	{
		mBindingLabels[0]->setText("Off");
	}
	else 
	{
		mBindingLabels[0]->setText("On");
	}

	if (isMuteMusicOn)
	{
		mBindingLabels[1]->setText("On");
	}
	else
	{
		mBindingLabels[1]->setText("Off");
	}

	if (isMuteSoundOn)
	{
		mBindingLabels[2]->setText("On");
	}
	else
	{
		mBindingLabels[2]->setText("Off");
	}
}

void OptionsState::setVsync()
{
	if (isVsyncOn) {
		isVsyncOn = false;
	}
	else {
		isVsyncOn = true;
	}

	getContext().window->setVerticalSyncEnabled(isVsyncOn);
}

void OptionsState::setMuteMusic()
{
	if (isMuteMusicOn) {
		isMuteMusicOn = false;
		getContext().music->setVolume(0);
	}
	else {
		isMuteMusicOn = true;
		getContext().music->setVolume(100);
	}
}

void OptionsState::setMuteSound()
{
	if (isMuteSoundOn) {
		isMuteSoundOn = false;
		getContext().sounds->setVolume(0);
	}
	else {
		isMuteSoundOn = true;
		getContext().sounds->setVolume(100);
	}
}