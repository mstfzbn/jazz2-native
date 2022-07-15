#include "Sprite.h"
#include "RenderCommand.h"

namespace nCine
{

	///////////////////////////////////////////////////////////
	// CONSTRUCTORS and DESTRUCTOR
	///////////////////////////////////////////////////////////

	Sprite::Sprite()
		: Sprite(nullptr, nullptr, 0.0f, 0.0f)
	{
	}

	Sprite::Sprite(SceneNode* parent, Texture* texture)
		: Sprite(parent, texture, 0.0f, 0.0f)
	{
	}

	Sprite::Sprite(Texture* texture)
		: Sprite(nullptr, texture, 0.0f, 0.0f)
	{
	}

	Sprite::Sprite(SceneNode* parent, Texture* texture, float xx, float yy)
		: BaseSprite(parent, texture, xx, yy)
	{
		init();
	}

	Sprite::Sprite(SceneNode* parent, Texture* texture, const Vector2f& position)
		: Sprite(parent, texture, position.X, position.Y)
	{
	}

	Sprite::Sprite(Texture* texture, float xx, float yy)
		: Sprite(nullptr, texture, xx, yy)
	{
	}

	Sprite::Sprite(Texture* texture, const Vector2f& position)
		: Sprite(nullptr, texture, position.X, position.Y)
	{
	}

	///////////////////////////////////////////////////////////
	// PROTECTED FUNCTIONS
	///////////////////////////////////////////////////////////

	Sprite::Sprite(const Sprite& other)
		: BaseSprite(other)
	{
		init();
		setTexRect(other.texRect_);
	}

	///////////////////////////////////////////////////////////
	// PRIVATE FUNCTIONS
	///////////////////////////////////////////////////////////

	void Sprite::init()
	{

		/*if (texture_ && texture_->name() != nullptr)
		{
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(texture_->name(), nctl::strnlen(texture_->name(), Object::MaxNameLength));
		}*/

		type_ = ObjectType::SPRITE;
		renderCommand_.setType(RenderCommand::CommandTypes::SPRITE);

		Material::ShaderProgramType shaderProgramType;
		if (texture_) {
			shaderProgramType = (texture_->numChannels() >= 3 ? Material::ShaderProgramType::SPRITE : Material::ShaderProgramType::SPRITE_GRAY);
		} else {
			shaderProgramType = Material::ShaderProgramType::SPRITE_NO_TEXTURE;
		}
		renderCommand_.material().setShaderProgramType(shaderProgramType);
		spriteBlock_ = renderCommand_.material().uniformBlock("SpriteBlock");
		renderCommand_.geometry().setDrawParameters(GL_TRIANGLE_STRIP, 0, 4);

		if (texture_) {
			setTexRect(Recti(0, 0, texture_->width(), texture_->height()));
		}
	}

	void Sprite::textureHasChanged(Texture* newTexture)
	{
		if (renderCommand_.material().shaderProgramType() != Material::ShaderProgramType::CUSTOM) {
			Material::ShaderProgramType shaderProgramType;
			if (newTexture) {
				shaderProgramType = (newTexture->numChannels() >= 3 ? Material::ShaderProgramType::SPRITE : Material::ShaderProgramType::SPRITE_GRAY);
			} else {
				shaderProgramType = Material::ShaderProgramType::SPRITE_NO_TEXTURE;
			}
			const bool shaderHasChanged = renderCommand_.material().setShaderProgramType(shaderProgramType);
			if (shaderHasChanged) {
				spriteBlock_ = renderCommand_.material().uniformBlock("SpriteBlock");
				dirtyBits_.set(DirtyBitPositions::ColorBit);
			}
		}

		if (texture_ && newTexture && texture_ != newTexture) {
			Recti texRect = texRect_;
			texRect.X = (int)((texRect.X / float(texture_->width())) * float(newTexture->width()));
			texRect.Y = (int)((texRect.Y / float(texture_->height())) * float(newTexture->width()));
			texRect.W = (int)((texRect.W / float(texture_->width())) * float(newTexture->width()));
			texRect.H = (int)((texRect.H / float(texture_->height())) * float(newTexture->width()));
			setTexRect(texRect); // it also sets width_ and height_
		} else if (texture_ == nullptr && newTexture) {
			// Assigning a texture when there wasn't any
			setTexRect(Recti(0, 0, newTexture->width(), newTexture->height()));
		}
	}

}
