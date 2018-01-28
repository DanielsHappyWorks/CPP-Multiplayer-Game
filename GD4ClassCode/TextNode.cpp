#include "TextNode.hpp"
#include "Utility.hpp"

#include <SFML/Graphics/RenderTarget.hpp>


TextNode::TextNode(const FontHolder& fonts, const std::string& text)
{
	mText.setFont(fonts.get(Fonts::Main));
	mText.setCharacterSize(22);
	mText.setColor(sf::Color::Yellow);
	mText.setStyle(sf::Text::Italic);
	setString(text);
}

void TextNode::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(mText, states);
}

void TextNode::setString(const std::string& text)
{
	mText.setString(text);
	centerOrigin(mText);
}