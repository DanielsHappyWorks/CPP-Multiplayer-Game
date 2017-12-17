#pragma once
#include "Entity.hpp"
#include "Command.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/Graphics/Sprite.hpp>

class Platform : public Entity
{
public:
	enum Type
	{
		smallPlatform,
		largePlatform,
		TypeCount
	};
public:
	Type 					mType;

public:
	Platform(Type type, const TextureHolder& textures);
	virtual unsigned int	getCategory() const;
	virtual sf::FloatRect	getBoundingRect() const;

protected:
	virtual void			drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;

private:
	sf::Sprite				mSprite;
};