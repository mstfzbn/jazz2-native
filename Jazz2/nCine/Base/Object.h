#pragma once

namespace nCine
{
	/// Static RRTI and identification index
	class Object
	{
	public:
		/// Object types
		enum class ObjectType {
			BASE = 0,
			TEXTURE,
			SCENENODE,
			SPRITE,
			MESH_SPRITE,
			ANIMATED_SPRITE,
			PARTICLE,
			PARTICLE_SYSTEM,
			FONT,
			TEXTNODE,
			AUDIOBUFFER,
			AUDIOBUFFER_PLAYER,
			AUDIOSTREAM_PLAYER
		};

		/// Maximum length for an object name
		static const unsigned int MaxNameLength = 128;

		/// Constructs an object with a specified type and adds it to the index
		explicit Object(ObjectType type);
		/// Constructs an object with a specified type and name and adds it to the index
		Object(ObjectType type, const char* name);
		/// Removes an object from the index and then destroys it
		virtual ~Object();

		/// Move constructor
		Object(Object&& other);
		/// Move assignment operator
		Object& operator=(Object&& other);

		/// Returns the object identification number
		inline unsigned int id() const {
			return id_;
		}

		/// Returns the object type (RTTI)
		inline ObjectType type() const {
			return type_;
		}
		/// Static method to return class type
		inline static ObjectType sType() {
			return ObjectType::BASE;
		}

		/// Returns a casted pointer to the object with the specified id, if any exists
		template <class T> static T* fromId(unsigned int id);

	protected:
		/// Object type
		ObjectType type_;

		/// Protected copy constructor used to clone objects
		Object(const Object& other);

	private:
		/// Object identification in the indexer
		unsigned int id_;

		 /// Deleted assignment operator
		Object& operator=(const Object&) = delete;
	};

}
