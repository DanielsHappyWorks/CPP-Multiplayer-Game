#pragma once
#include "Entity.hpp"
#include "Command.hpp"
#include "ResourceIdentifiers.hpp"
#include "Projectile.hpp"
#include "TextNode.hpp"
#include "Animation.hpp"

#include <SFML/Graphics/Sprite.hpp>


class Character : public Entity
{
public:
	enum Type
	{
		Eagle,
		Raptor,
		Avenger,
		TypeCount
	};


public:
	Character(Type type, const TextureHolder& textures, const FontHolder& fonts);

	virtual unsigned int	getCategory() const;
	virtual sf::FloatRect	getBoundingRect() const;
	virtual void			remove();
	virtual bool 			isMarkedForRemoval() const;
	bool					isAllied() const;
	float					getMaxSpeed() const;
	void					disablePickups();

	void					increaseFireRate();
	void					collectMissiles(unsigned int count);

	void 					fire();
	void 					jump(float vx, float vy);
	void					launchMissile();
	void					playLocalSound(CommandQueue& commands, SoundEffect::ID effect);
	int						getIdentifier();
	void					setIdentifier(int identifier);
	float					getKnockback();
	void					setKnockback(float increment);
	void					incrementKnockback(float increment);
	int						getMissileAmmo() const;
	void					setMissileAmmo(int ammo);

	bool					mIsGrounded;
	sf::Vector2f			mPreviousPositionOnFire;


private:
	virtual void			drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void 			updateCurrent(sf::Time dt, CommandQueue& commands);
	void					updateMovementPattern(sf::Time dt);
	void					checkPickupDrop(CommandQueue& commands);
	void					checkProjectileLaunch(sf::Time dt, CommandQueue& commands);

	void					createBullets(SceneNode& node, const TextureHolder& textures);
	void					createProjectile(SceneNode& node, Projectile::Type type, const TextureHolder& textures);

	void					updateTexts();
	void					updateRollAnimation();


private:
	Type					mType;
	sf::Sprite				mSprite;
	Animation				mExplosion;
	Command 				mFireCommand;
	Command					mMissileCommand;
	sf::Time				mFireCountdown;
	bool 					mIsFiring;
	bool					mIsLaunchingMissile;
	bool 					mShowExplosion;
	bool					mExplosionBegan;
	bool					mPlayedExplosionSound;
	bool					mSpawnedPickup;
	bool					mPickupsEnabled;

	int						mFireRateLevel;
	int						mMissileAmmo;

	Command 				mDropPickupCommand;
	float					mTravelledDistance;
	std::size_t				mDirectionIndex;
	TextNode*				mPlayerDisplay;
	TextNode*				mHealthDisplay;
	TextNode*				mMissileDisplay;
	TextNode*				mKnockbackDisplay;

	int						mIdentifier;
	float					mKnockbackModifier;
	int						mShootDirection;
	int						mCurrentAnimation;
	int						mAinmationFrameTimer;
};