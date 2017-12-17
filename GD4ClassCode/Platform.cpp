#include "Platform.hpp"
#include "DataTables.hpp"
#include "Category.hpp"
#include "CommandQueue.hpp"
#include "Utility.hpp"
#include "ResourceHolder.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

Platform::Platform(const TextureHolder& textures)
	: Entity(1)
	, mSprite(textures.get(Textures::FinishLine))
{
	centerOrigin(mSprite);
}

unsigned int Platform::getCategory() const
{
	return Category::Platform;
}

sf::FloatRect Platform::getBoundingRect() const
{
	return getWorldTransform().transformRect(mSprite.getGlobalBounds());
}

void Platform::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(mSprite, states);
}

