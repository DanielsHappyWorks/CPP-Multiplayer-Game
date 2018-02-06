#include "OptionsState.hpp"
#include "Utility.hpp"
#include "ResourceHolder.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

/*
	Handles the options menu,
	Allows enabling for sound and vsync
*/

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

	if (context.music->getVolume() == 0) {
		isMuteMusicOn = false;
	}
	else {
		isMuteMusicOn = true;
	}

	isShaderOn = World::mShadersEnabled;

	mBackgroundSprite.setTexture(context.textures->get(Textures::TitleScreen));

	auto backButton = std::make_shared<GUI::Button>(context);
	backButton->setPosition(400.f, 500.f);
	backButton->setText("Back");
	backButton->setCallback(std::bind(&OptionsState::requestStackPop, this));


	auto vSyncButton = std::make_shared<GUI::Button>(context);
	vSyncButton->setPosition(400.f, 300.f);
	vSyncButton->setText("vsync");
	vSyncButton->setCallback([this]()
	{
		setVsync();
	});
	//vsync
	mBindingLabels[0] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[0]->setPosition(620.f, 315.f);

	auto muteButtonMusic = std::make_shared<GUI::Button>(context);
	muteButtonMusic->setPosition(400.f, 350.f);
	muteButtonMusic->setText("mute music");
	muteButtonMusic->setCallback([this]()
	{
		setMuteMusic();
	});

	//mute Music
	mBindingLabels[1] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[1]->setPosition(620.f, 365.f);

	auto muteButtonSound = std::make_shared<GUI::Button>(context);
	muteButtonSound->setPosition(400.f, 400.f);
	muteButtonSound->setText("mute sounds");
	muteButtonSound->setCallback([this]()
	{
		setMuteSound();
	});

	//mute Sounds
	mBindingLabels[2] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[2]->setPosition(620.f, 415.f);

	auto shadersButton = std::make_shared<GUI::Button>(context);
	shadersButton->setPosition(400.f, 450.f);
	shadersButton->setText("shaders");
	shadersButton->setCallback([this]()
	{
		setShadersEnabled();
	});
	//shaders
	mBindingLabels[3] = std::make_shared<GUI::Label>("ON", *context.fonts);
	mBindingLabels[3]->setPosition(620.f, 465.f);

	mGUIContainer.pack(vSyncButton);
	mGUIContainer.pack(mBindingLabels[0]);
	mGUIContainer.pack(muteButtonMusic);
	mGUIContainer.pack(mBindingLabels[1]);
	mGUIContainer.pack(muteButtonSound);
	mGUIContainer.pack(mBindingLabels[2]);
	mGUIContainer.pack(shadersButton);
	mGUIContainer.pack(mBindingLabels[3]);
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

	if (isShaderOn)
	{
		mBindingLabels[3]->setText("On");
	}
	else
	{
		mBindingLabels[3]->setText("Off");
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

void OptionsState::setShadersEnabled()
{
	if (isShaderOn) {
		isShaderOn = false;
	}
	else {
		isShaderOn = true;
	}

	World::mShadersEnabled = isShaderOn;
	//getContext().window->setVerticalSyncEnabled(isVsyncOn);
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