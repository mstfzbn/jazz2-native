#pragma once

#define NCINE_INCLUDE_OPENGL
#include "../../CommonHeaders.h"

namespace nCine
{
	/// A class to store information about an active OpenGL shader uniform
	/*! Its only purpose is to initialize a `GLUniformCache` class. */
	class GLUniform
	{
	public:
		GLUniform();
		GLUniform(GLuint program, GLuint index);

		inline GLuint index() const {
			return index_;
		}
		inline GLint blockIndex() const {
			return blockIndex_;
		}
		inline GLint location() const {
			return location_;
		}
		inline GLint size() const {
			return size_;
		}
		inline GLenum type() const {
			return type_;
		}
		inline GLint offset() const {
			return offset_;
		}
		inline const char* name() const {
			return name_;
		}
		GLenum basicType() const;
		unsigned int numComponents() const;
		inline unsigned int memorySize() const {
			return size() * numComponents() * sizeof(basicType());
		}

	private:
		GLuint index_;
		/// Active uniforms not in a block have a block index of -1
		GLint blockIndex_;
		GLint location_;
		GLint size_;
		GLenum type_;
		GLint offset_;
		static const int MaxNameLength = 32;
		char name_[MaxNameLength];

		friend class GLUniformBlock;
	};

}
