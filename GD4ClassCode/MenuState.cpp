#include "MenuState.hpp"
#include "Utility.hpp"
#include "Button.hpp"
#include "ResourceHolder.hpp"
#include "MusicPlayer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>


MenuState::MenuState(StateStack& stack, Context context)
	: State(stack, context)
	, mGUIContainer()
{
	sf::Texture& texture = context.textures->get(Textures::TitleScreen);
	mBackgroundSprite.setTexture(texture);

	auto playButton = std::make_shared<GUI::Button>(context);
	playButton->setPosition(420, 300);
	playButton->setText("Play");
	playButton->setCallback([this]()
	{
		requestStackPop();
		requestStackPush(States::Game);
	});

	/*auto hostPlayButton = std::make_shared<GUI::Button>(context);
	hostPlayButton->setPosition(100, 350);
	hostPlayButton->setText("Host");
	hostPlayButton->setCallback([this]()
	{
		requestStackPop();
		requestStackPush(States::HostGame);
	});*/

	auto optionsPlayButton = std::make_shared<GUI::Button>(context);
	optionsPlayButton->setPosition(420, 350);
	optionsPlayButton->setText("Options");
	optionsPlayButton->setCallback([this]()
	{
		requestStackPush(States::Options);
	});
	
	/*auto joinPlayButton = std::make_shared<GUI::Button>(context);
	joinPlayButton->setPosition(100, 400);
	joinPlayButton->setText("Join");
	joinPlayButton->setCallback([this]()
	{
		requestStackPop();
		requestStackPush(States::JoinGame);
	});
	*/

	auto settingsButton = std::make_shared<GUI::Button>(context);
	settingsButton->setPosition(420, 400);
	settingsButton->setText("Controls");
	settingsButton->setCallback([this]()
	{
		requestStackPush(States::Settings);
	});

	auto exitButton = std::make_shared<GUI::Button>(context);
	exitButton->setPosition(420, 450);
	exitButton->setText("Exit");
	exitButton->setCallback([this]()
	{
		requestStackPop();
	});

	mGUIContainer.pack(playButton);
	mGUIContainer.pack(optionsPlayButton);
	/*mGUIContainer.pack(hostPlayButton);
	mGUIContainer.pack(joinPlayButton);*/
	mGUIContainer.pack(settingsButton);
	mGUIContainer.pack(exitButton);

	//Play menu theme
	context.music->play(Music::MenuTheme);
}

void MenuState::draw()
{
	sf::RenderWindow& window = *getContext().window;

	window.setView(window.getDefaultView());

	window.draw(mBackgroundSprite);
	window.draw(mGUIContainer);
}

bool MenuState::update(sf::Time)
{
	return true;
}

bool MenuState::handleEvent(const sf::Event& event)
{
	mGUIContainer.handleEvent(event);
	return false;
}

