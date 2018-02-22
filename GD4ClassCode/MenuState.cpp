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
	playButton->setPosition(420, 270);
	playButton->setText("Play");
	playButton->setCallback([this]()
	{
		requestStackPop();
		requestStackPush(States::Game);
	});

	auto hostPlayButton = std::make_shared<GUI::Button>(context);
	hostPlayButton->setPosition(420, 325);
	hostPlayButton->setText("Host");
	hostPlayButton->setCallback([this]()
	{
		requestStackPop();
		requestStackPush(States::HostGame);
	});

	auto joinPlayButton = std::make_shared<GUI::Button>(context);
	joinPlayButton->setPosition(420, 380);
	joinPlayButton->setText("Join");
	joinPlayButton->setCallback([this]()
	{
		requestStackPop();
		requestStackPush(States::JoinGame);
	});

	auto optionsPlayButton = std::make_shared<GUI::Button>(context);
	optionsPlayButton->setPosition(420, 435);
	optionsPlayButton->setText("Options");
	optionsPlayButton->setCallback([this]()
	{
		requestStackPush(States::Options);
	});

	auto settingsButton = std::make_shared<GUI::Button>(context);
	settingsButton->setPosition(420, 490);
	settingsButton->setText("Controls");
	settingsButton->setCallback([this]()
	{
		requestStackPush(States::Settings);
	});
	
	auto HighScoreButton = std::make_shared<GUI::Button>(context);
	HighScoreButton->setPosition(420, 545);
	HighScoreButton->setText("HighScore");
	HighScoreButton->setCallback([this]()
	{
		requestStackPush(States::HighScore);
	});

	auto exitButton = std::make_shared<GUI::Button>(context);
	exitButton->setPosition(420, 600);
	exitButton->setText("Exit");
	exitButton->setCallback([this]()
	{
		requestStackPop();
	});

	mGUIContainer.pack(playButton);
	mGUIContainer.pack(hostPlayButton);
	mGUIContainer.pack(joinPlayButton);
	mGUIContainer.pack(optionsPlayButton);
	mGUIContainer.pack(settingsButton);
	mGUIContainer.pack(HighScoreButton);
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

