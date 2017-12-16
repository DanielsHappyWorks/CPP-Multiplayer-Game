#include "ParticleNode.hpp"
#include "Foreach.hpp"
#include "DataTables.hpp"
#include "ResourceHolder.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <algorithm>

//anonymous namespace - can only access in this class
namespace
{
	const std::vector<ParticleData> Table = initializeParticleData();
}

ParticleNode::ParticleNode(Particle::Type type, const TextureHolder& textures)
	:SceneNode()
	, mParticles()
	, mTexture(textures.get(Textures::Particle))
	, mType(type)
	, mVertexArray(sf::Quads) 
	, mNeedsVertexUpdate(true)
{}

void ParticleNode::addParticle(sf::Vector2f position)
{
	Particle particle;
	particle.position = position;
	particle.color = Table[mType].color;
	particle.lifetime = Table[mType].lifetime;

	mParticles.push_back(particle);
}

Particle::Type ParticleNode::getParticleType() const
{
	return mType;
}

unsigned int ParticleNode::getCategory() const
{
	return Category::ParticleSystem;
}

void ParticleNode::updateCurrent(sf::Time dt, CommandQueue&)
{
	//Remove expired particles at the beginning
	while (!mParticles.empty() && mParticles.front().lifetime <= sf::Time::Zero)
	{
		mParticles.pop_front();
	}

	//Decrease the lifetime of existing particles
	FOREACH(Particle& particle, mParticles)
	{
		particle.lifetime -= dt;
	}

	mNeedsVertexUpdate = true; //check that something has changed before we set this?
}

void ParticleNode::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (mNeedsVertexUpdate)
	{
		computeVertices();
		mNeedsVertexUpdate = false;
	}

	//apply the particle texture
	states.texture = &mTexture;

	//Draw vertices
	target.draw(mVertexArray, states);
}

void ParticleNode::addVertex(float worldX, float worldY, float textureCoordX, float textureCoordY, const sf::Color& color) const
{
	sf::Vertex vertex;
	vertex.position = sf::Vector2f(worldX, worldY);
	vertex.texCoords = sf::Vector2f(textureCoordX, textureCoordY);
	vertex.color = color;

	mVertexArray.append(vertex);
}

void ParticleNode::computeVertices() const
{
	sf::Vector2f size(mTexture.getSize());
	sf::Vector2f half = size / 2.f;

	//refill vertex array
	mVertexArray.clear();
	FOREACH(const Particle& particle, mParticles)
	{
		sf::Vector2f position = particle.position;
		sf::Color color = particle.color;

		float ratio = particle.lifetime.asSeconds() / Table[mType].lifetime.asSeconds();
		color.a = static_cast<sf::Uint8>(255 * std::max(ratio, 0.f));

		//add the 4 vertices that define the particle
		addVertex(position.x - half.x, position.y - half.y, 0.f, 0.f, color); //top left
		addVertex(position.x + half.x, position.y - half.y, size.x, 0.f, color); //top right
		addVertex(position.x + half.x, position.y + half.y, size.x, size.y, color); //bottom right
		addVertex(position.x - half.x, position.y + half.y, 0.f, size.y, color); //bottom left
	}
}








