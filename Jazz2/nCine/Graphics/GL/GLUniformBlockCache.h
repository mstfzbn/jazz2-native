#pragma once

#define NCINE_INCLUDE_OPENGL
#include "../../CommonHeaders.h"

#include "GLUniformCache.h"
#include "../../Base/StaticHashMap.h"

#include <string>

namespace nCine
{
	class GLUniformBlock;
	class GLBufferObject;

	/// A class to cache a uniform block buffer and then update it in the shader
	class GLUniformBlockCache
	{
	public:
		GLUniformBlockCache();
		explicit GLUniformBlockCache(GLUniformBlock* uniformBlock);

		inline const GLUniformBlock* uniformBlock() const {
			return uniformBlock_;
		}
		/// Wrapper around `GLUniformBlock::index()`
		GLuint index() const;
		/// Wrapper around `GLUniformBlock::bindingIndex()`
		GLuint bindingIndex() const;
		/// Wrapper around `GLUniformBlock::size()`
		GLint size() const;

		inline GLubyte* dataPointer() {
			return dataPointer_;
		}
		inline const GLubyte* dataPointer() const {
			return dataPointer_;
		}
		void setDataPointer(GLubyte* dataPointer);

		inline GLint usedSize() const {
			return usedSize_;
		}
		inline void setUsedSize(GLint usedSize) {
			usedSize_ = usedSize;
		}

		GLUniformCache* uniform(const char* name);
		/// Wrapper around `GLUniformBlock::setBlockBinding()`
		void setBlockBinding(GLuint blockBinding);

	private:
		GLUniformBlock* uniformBlock_;
		GLubyte* dataPointer_;
		/// Keeps tracks of how much of the cache needs to be uploaded to the UBO
		GLint usedSize_;

		static const int UniformHashSize = 8;
		StaticHashMap<std::string, GLUniformCache, UniformHashSize> uniformCaches_;
	};

}
