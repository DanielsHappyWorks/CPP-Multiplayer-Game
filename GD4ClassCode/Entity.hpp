#pragma once
#include "SceneNode.hpp"

class Entity : public SceneNode
{
private:
	sf::Vector2f mVelocity;
	int mHitpoints;

public:
	explicit Entity(int hitpoints);

	int getHitpoints() const;
	void setHitpoints(int points);
	void repair(int points);
	void damage(int point);
	void destroy();
	virtual void remove();
	virtual bool isDestroyed() const;

	void setVelocity(sf::Vector2f velocity);
	void setVelocity(float vx, float vy);
	void accelerate(sf::Vector2f velocity);
	void accelerate(float vx, float vy);
	sf::Vector2f getVelocity() const;

protected:
	virtual void updateCurrent(sf::Time dt, CommandQueue& commands);
};