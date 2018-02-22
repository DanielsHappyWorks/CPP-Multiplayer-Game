#include "HighScoreState.hpp"
#include "Utility.hpp"
#include "ResourceHolder.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <SFML/Graphics/RenderWindow.hpp>

/*
Handles the highscore menu,

*/

HighScoreState::HighScoreState(StateStack& stack, Context context)
	: State(stack, context)
	, mGUIContainer()
{
	mBackgroundSprite.setTexture(context.textures->get(Textures::TitleScreen));

	mBindingLabels[0] = std::make_shared<GUI::Label>("HighScore", *context.fonts);
	mBindingLabels[0]->setPosition(460.f, 200.f);
	mGUIContainer.pack(mBindingLabels[0]);

	std::string line;
	std::ifstream myfile("highscore.txt");
	if (myfile.is_open())
	{
		int count = 1;
		while (myfile.good())
		{
			getline(myfile, line);
			
			mBindingLabels[count] = std::make_shared<GUI::Label>(line, *context.fonts);
			mBindingLabels[count]->setPosition(110.f , 200.f + (20 * count));
			mGUIContainer.pack(mBindingLabels[count]);
			count++;
		}
		myfile.close();
	}
	auto backButton = std::make_shared<GUI::Button>(context);
	backButton->setPosition(420.f, 640.f);
	backButton->setText("Back");
	backButton->setCallback(std::bind(&HighScoreState::requestStackPop, this));
	mGUIContainer.pack(backButton);
}

void HighScoreState::draw()
{
	sf::RenderWindow& window = *getContext().window;

	window.draw(mBackgroundSprite);
	window.draw(mGUIContainer);
}

bool HighScoreState::update(sf::Time)
{
	//updateLabels();
	return true;
}

bool HighScoreState::handleEvent(const sf::Event& event)
{
	mGUIContainer.handleEvent(event);
	return false;
}