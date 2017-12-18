#include "OptionsState.hpp"
#include "Utility.hpp"
#include "ResourceHolder.hpp"

#include <SFML/Graphics/RenderWindow.hpp>


OptionsState::OptionsState(StateStack& stack, Context context)
	: State(stack, context)
	, mGUIContainer()
	, isVsyncOn(true)
{
	if (context.sounds->getVolume() == 0) {
		isMuteSoundOn = false;
	}
	else {
		isMuteSoundOn = true;
	}

	if (context.music->getVolume() == 0) {
		isMuteMusicOn = false;
	}
	else {
		isMuteMusicOn = true;
	}


	mBackgroundSprite.setTexture(context.textures->get(Textures::TitleScreen));

	auto backButton = std::make_shared<GUI::Button>(context);
	backButton->setPosition(100.f, 450.f);
	backButton->setText("Back");
	backButton->setCallback(std::bind(&OptionsState::requestStackPop, this));


	auto vSyncButton = std::make_shared<GUI::Button>(context);
	vSyncButton->setPosition(100.f, 300.f);
	vSyncButton->setText("vsync");
	vSyncButton->setCallback([this]()
	{
		//vsyncLable = app.toggleVsync();
		setVsync();
	});
	//vsync
	mBindingLabels[0] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[0]->setPosition(350.f, 315.f);

	auto muteButtonMusic = std::make_shared<GUI::Button>(context);
	muteButtonMusic->setPosition(100.f, 350.f);
	muteButtonMusic->setText("mute music");
	muteButtonMusic->setCallback([this]()
	{
		//vsyncLable = app.toggleVsync();
		setMuteMusic();
	});
	//mute Music
	mBindingLabels[1] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[1]->setPosition(350.f, 365.f);

	auto muteButtonSound = std::make_shared<GUI::Button>(context);
	muteButtonSound->setPosition(100.f, 400.f);
	muteButtonSound->setText("mute sounds");
	muteButtonSound->setCallback([this]()
	{
		setMuteSound();
	});
	//mute Sounds
	mBindingLabels[2] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[2]->setPosition(350.f, 415.f);

	mGUIContainer.pack(vSyncButton);
	mGUIContainer.pack(mBindingLabels[0]);
	mGUIContainer.pack(muteButtonMusic);
	mGUIContainer.pack(mBindingLabels[1]);
	mGUIContainer.pack(muteButtonSound);
	mGUIContainer.pack(mBindingLabels[2]);
	mGUIContainer.pack(backButton);
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
		mBindingLabels[0]->setText("On");
	}
	else 
	{
		mBindingLabels[0]->setText("Off");
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