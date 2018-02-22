#pragma once
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"
#include "SceneNode.hpp"
#include "SpriteNode.hpp"
#include "Character.hpp"
#include "CommandQueue.hpp"
#include "Command.hpp"
#include "Pickup.hpp"
#include "Platform.hpp"
#include "BloomEffect.hpp"
#include "SoundPlayer.hpp"
#include "NetworkProtocol.hpp"


#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <queue>

//Foward declaration
namespace sf
{
	class RenderTarget;
}

class NetworkNode;

class World : private sf::NonCopyable
{
public:
	explicit World(sf::RenderTarget& window, FontHolder& font, SoundPlayer& sounds, bool networked = false);
	void update(sf::Time dt);
	void draw();

	sf::FloatRect getViewBounds() const;
	CommandQueue& getCommandQueue();
	Character* addCharacter(int identifier, float x, float y);
	Character* addCharacter(int identifier);
	void removeCharacter(int identifier);
	void setWorldHeight(float height);
	std::map<int, int> getSurvivabilities();

	void addPlatform(float x, float y, Platform::Type type);
	void sortEnemies();

	bool hasAlivePlayer() const;
	int isLastOneStanding();

	Character* getCharacter(int identifier) const;
	sf::FloatRect getBattlefieldBounds() const;

	void createPickup(sf::Vector2f position, Pickup::Type type);
	bool pollGameAction(GameActions::Action& out);

private:
	void loadTextures(); 
	void adaptPlayerPosition();
	void adaptPlayerVelocity();
	void handleCollisions();
	void handleCollisionsPlatform();
	void updateSounds();

	void buildScene();
	void addPlatforms();
	void destroyEntitiesOutsideView();
	void guideMissiles();

private:
	enum Layer
	{
		Background,
		LowerAir,
		UpperAir,
		LayerCount
	};

	struct SpawnPoint
	{
		SpawnPoint(Character::Type type, float x, float y)
			: type(type)
			, x(x)
			, y(y)
		{
		}

		Character::Type type;
		float x;
		float y;
	};

public:
	static bool							mShadersEnabled;

private:
	sf::RenderTarget&					mTarget;
	sf::RenderTexture					mSceneTexture;
	sf::View							mWorldView;
	TextureHolder						mTextures;
	FontHolder&							mFonts;
	SoundPlayer&						mSounds;

	SceneNode							mSceneGraph;
	std::array<SceneNode*, LayerCount>	mSceneLayers;
	CommandQueue						mCommandQueue;

	sf::FloatRect						mWorldBounds;
	sf::Vector2f						mSpawnPosition;
	sf::Vector2f						mGravity;
	float								mScrollSpeed;
	float								mScrollSpeedCompensation;
	std::vector<Character*>				mPlayerCharacters;

	std::vector<Character*>				mActivePlayers;
	std::map<int, int>					mSurvivabilities;

	BloomEffect							mBloomEffect;

	bool								mNetworkedWorld;
	NetworkNode*						mNetworkNode;
	SpriteNode*							mFinishSprite;
};