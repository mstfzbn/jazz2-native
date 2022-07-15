//#include "TextNode.h"
//#include "FontGlyph.h"
//#include "Texture.h"
//#include "RenderCommand.h"
//
//namespace nCine {
//
/////////////////////////////////////////////////////////////
//// CONSTRUCTORS and DESTRUCTOR
/////////////////////////////////////////////////////////////
//
//TextNode::TextNode()
//    : TextNode(nullptr, nullptr, DefaultStringLength)
//{
//}
//
//TextNode::TextNode(unsigned int maxStringLength)
//    : TextNode(nullptr, nullptr, maxStringLength)
//{
//}
//
//TextNode::TextNode(SceneNode *parent, Font *font)
//    : TextNode(parent, font, DefaultStringLength)
//{
//}
//
//TextNode::TextNode(SceneNode *parent, Font *font, unsigned int maxStringLength)
//    : DrawableNode(parent, 0.0f, 0.0f), string_(maxStringLength), dirtyDraw_(true),
//      dirtyBoundaries_(true), withKerning_(true), font_(font),
//      interleavedVertices_(maxStringLength * 4 + (maxStringLength - 1) * 2),
//      xAdvance_(0.0f), yAdvance_(0.0f), lineLengths_(4), alignment_(Alignment::LEFT),
//      lineHeight_(font ? font->lineHeight() : 0.0f), textnodeBlock_(nullptr)
//{
//	//ASSERT(maxStringLength > 0);
//	init();
//}
//
/////////////////////////////////////////////////////////////
//// PUBLIC FUNCTIONS
/////////////////////////////////////////////////////////////
//
//float TextNode::width() const
//{
//	calculateBoundaries();
//	return width_ * scaleFactor_.x;
//}
//
//float TextNode::height() const
//{
//	calculateBoundaries();
//	return height_ * scaleFactor_.y;
//}
//
//float TextNode::absWidth() const
//{
//	calculateBoundaries();
//	return width_ * absScaleFactor_.x;
//}
//
//float TextNode::absHeight() const
//{
//	calculateBoundaries();
//	return height_ * absScaleFactor_.y;
//}
//
//void TextNode::setFont(Font *font)
//{
//	// Allow self-assignment to take into account the case where the font stays the same but it loads new data
//
//	if (font_ && font)
//	{
//		// Keep the ratio between text node lineHeight and font one
//		lineHeight_ = (lineHeight_ / font_->lineHeight()) * font->lineHeight();
//	}
//	else if (font_ == nullptr && font)
//	{
//		// Assigning a font when there wasn't any
//		lineHeight_ = font->lineHeight();
//	}
//
//	font_ = font;
//	if (font_)
//	{
//		const Material::ShaderProgramType shaderProgramType = font_->renderMode() == Font::RenderMode::GLYPH_IN_RED
//		                                                          ? Material::ShaderProgramType::TEXTNODE_RED
//		                                                          : Material::ShaderProgramType::TEXTNODE_ALPHA;
//		const bool shaderHasChanged = renderCommand_->material().setShaderProgramType(shaderProgramType);
//		if (shaderHasChanged)
//		{
//			textnodeBlock_ = renderCommand_->material().uniformBlock("TextnodeBlock");
//			dirtyBits_.set(DirtyBitPositions::ColorBit);
//		}
//		renderCommand_->material().setTexture(*font_->texture());
//
//		dirtyDraw_ = true;
//		dirtyBoundaries_ = true;
//	}
//	else
//	{
//		renderCommand_->material().setTexture(nullptr);
//		// Skip rendering for this node if no font is assigned
//		width_ = 0;
//		height_ = 0;
//	}
//}
//
//void TextNode::enableKerning(bool withKerning)
//{
//	if (withKerning != withKerning_)
//	{
//		withKerning_ = withKerning;
//		dirtyDraw_ = true;
//		dirtyBoundaries_ = true;
//	}
//}
//
//void TextNode::setAlignment(Alignment alignment)
//{
//	if (alignment != alignment_)
//	{
//		alignment_ = alignment;
//		dirtyDraw_ = true;
//		dirtyBoundaries_ = true;
//	}
//}
//
//void TextNode::setString(const std::string &string)
//{
//	if (string_ != string)
//	{
//		string_ = string;
//		dirtyDraw_ = true;
//		dirtyBoundaries_ = true;
//	}
//}
//
//Vector2f TextNode::calculateBoundaries(const Font &font, bool withKerning, const std::string &string)
//{
//	float xAdvanceMax = 0.0f; // longest line
//	float xAdvance = 0.0f;
//	float yAdvance = 0.0f;
//
//	const float lineHeight = static_cast<float>(font.lineHeight());
//	const unsigned int length = string.length();
//	for (unsigned int i = 0; i < length;) // increments handled by UTF-8 decoding
//	{
//		if (string[i] == '\n')
//		{
//			if (xAdvance > xAdvanceMax)
//				xAdvanceMax = xAdvance;
//			xAdvance = 0.0f;
//			yAdvance += lineHeight;
//			i++; // manual increment as newline character is not decoded
//		}
//		else
//		{
//			unsigned int codepoint = nctl::Utf8::InvalidUnicode;
//			const int codePointLength = string.utf8ToCodePoint(i, codepoint);
//			const FontGlyph *glyph = (codepoint != nctl::Utf8::InvalidUnicode) ? font.glyph(codepoint) : nullptr;
//			if (glyph)
//			{
//				xAdvance += glyph->xAdvance();
//				if (withKerning)
//				{
//					// font kerning
//					if (i + codePointLength < length)
//					{
//						unsigned int nextCodepoint = nctl::Utf8::InvalidUnicode;
//						string.utf8ToCodePoint(i + codePointLength, nextCodepoint);
//						xAdvance += glyph->kerning(nextCodepoint);
//					}
//				}
//			}
//			i += codePointLength; // manual increment to next codepoint
//		}
//	}
//
//	// If the string does not end with a new line character,
//	// last line height has not been taken into account before
//	if (!string.empty() && string[string.length() - 1] != '\n')
//		yAdvance += lineHeight;
//
//	if (xAdvance > xAdvanceMax)
//		xAdvanceMax = xAdvance;
//
//	return Vector2f(xAdvanceMax, yAdvance);
//}
//
//void TextNode::transform()
//{
//	// Precalculate boundaries for horizontal alignment
//	calculateBoundaries();
//	SceneNode::transform();
//}
//
//bool TextNode::draw(RenderQueue &renderQueue)
//{
//	// Early-out if the string is empty
//	if (string_.empty())
//		return false;
//
//	if (font_ && dirtyDraw_)
//	{
//		
//		// Clear every previous quad before drawing again
//		interleavedVertices_.clear();
//
//		unsigned int currentLine = 0;
//		xAdvance_ = calculateAlignment(currentLine) - width_ * 0.5f;
//		yAdvance_ = 0.0f - height_ * 0.5f;
//		const unsigned int length = string_.length();
//		for (unsigned int i = 0; i < length;) // increments handled by UTF-8 decoding
//		{
//			if (string_[i] == '\n')
//			{
//				currentLine++;
//				xAdvance_ = calculateAlignment(currentLine) - width_ * 0.5f;
//				yAdvance_ += lineHeight_;
//				i++; // manual increment as newline character is not decoded
//			}
//			else
//			{
//				unsigned int codepoint = nctl::Utf8::InvalidUnicode;
//				const int codePointLength = string_.utf8ToCodePoint(i, codepoint);
//				const FontGlyph *glyph = (codepoint != nctl::Utf8::InvalidUnicode) ? font_->glyph(codepoint) : nullptr;
//				if (glyph)
//				{
//					Degenerate degen = Degenerate::NONE;
//					if (length > 1)
//					{
//						if (i == 0)
//							degen = Degenerate::END;
//						else if (i == length - 1)
//							degen = Degenerate::START;
//						else
//							degen = Degenerate::START_END;
//					}
//					processGlyph(glyph, degen);
//
//					if (withKerning_)
//					{
//						// font kerning
//						if (i + codePointLength < length)
//						{
//							unsigned int nextCodepoint = nctl::Utf8::InvalidUnicode;
//							string_.utf8ToCodePoint(i + codePointLength, nextCodepoint);
//							xAdvance_ += glyph->kerning(nextCodepoint);
//						}
//					}
//				}
//				i += codePointLength; // manual increment to next codepoint
//			}
//		}
//
//		// Vertices are updated only if the string changes
//		renderCommand_->geometry().setNumVertices(interleavedVertices_.size());
//		renderCommand_->geometry().setHostVertexPointer(reinterpret_cast<const float *>(interleavedVertices_.data()));
//		dirtyDraw_ = false;
//	}
//
//	return DrawableNode::draw(renderQueue);
//}
//
/////////////////////////////////////////////////////////////
//// PROTECTED FUNCTIONS
/////////////////////////////////////////////////////////////
//
//TextNode::TextNode(const TextNode &other)
//    : DrawableNode(other),
//      string_(other.string_), dirtyDraw_(true), dirtyBoundaries_(true),
//      withKerning_(other.withKerning_), font_(other.font_),
//      interleavedVertices_(string_.capacity() * 4 + (string_.capacity() - 1) * 2),
//      xAdvance_(0.0f), yAdvance_(0.0f), lineLengths_(4), alignment_(other.alignment_),
//      lineHeight_(font_ ? font_->lineHeight() : 0.0f), textnodeBlock_(nullptr)
//{
//	init();
//	setBlendingEnabled(other.isBlendingEnabled());
//}
//
/////////////////////////////////////////////////////////////
//// PRIVATE FUNCTIONS
/////////////////////////////////////////////////////////////
//
//void TextNode::init()
//{
//	
//	if (font_ && font_->texture() && font_->texture()->name() != nullptr)
//	{
//		// When Tracy is disabled the statement body is empty and braces are needed
//		ZoneText(font_->texture()->name(), nctl::strnlen(font_->texture()->name(), Object::MaxNameLength));
//	}
//
//	type_ = ObjectType::TEXTNODE;
//	renderCommand_->setType(RenderCommand::CommandTypes::TEXT);
//	renderCommand_->material().setBlendingEnabled(true);
//
//	const Material::ShaderProgramType shaderProgramType = [](Font *font)
//	{
//		if (font)
//			return (font->renderMode() == Font::RenderMode::GLYPH_IN_RED)
//			           ? Material::ShaderProgramType::TEXTNODE_RED
//			           : Material::ShaderProgramType::TEXTNODE_ALPHA;
//		else
//			return Material::ShaderProgramType::TEXTNODE_ALPHA;
//	}(font_);
//	renderCommand_->material().setShaderProgramType(shaderProgramType);
//	textnodeBlock_ = renderCommand_->material().uniformBlock("TextnodeBlock");
//
//	if (font_)
//		renderCommand_->material().setTexture(*font_->texture());
//
//	renderCommand_->geometry().setPrimitiveType(GL_TRIANGLE_STRIP);
//	renderCommand_->geometry().setNumElementsPerVertex(sizeof(Vertex) / sizeof(float));
//}
//
//void TextNode::calculateBoundaries() const
//{
//	if (font_ && dirtyBoundaries_)
//	{
//		
//		const float oldWidth = width_;
//		const float oldHeight = height_;
//
//		lineLengths_.clear();
//		float xAdvanceMax = 0.0f; // longest line
//		xAdvance_ = 0.0f;
//		yAdvance_ = 0.0f;
//		const unsigned int length = string_.length();
//		for (unsigned int i = 0; i < length;) // increments handled by UTF-8 decoding
//		{
//			if (string_[i] == '\n')
//			{
//				lineLengths_.push_back(xAdvance_);
//				if (xAdvance_ > xAdvanceMax)
//					xAdvanceMax = xAdvance_;
//				xAdvance_ = 0.0f;
//				yAdvance_ += lineHeight_;
//				i++; // manual increment as newline character is not decoded
//			}
//			else
//			{
//				unsigned int codepoint = nctl::Utf8::InvalidUnicode;
//				const int codePointLength = string_.utf8ToCodePoint(i, codepoint);
//				const FontGlyph *glyph = (codepoint != nctl::Utf8::InvalidUnicode) ? font_->glyph(codepoint) : nullptr;
//				if (glyph)
//				{
//					xAdvance_ += glyph->xAdvance();
//					if (withKerning_)
//					{
//						// font kerning
//						if (i + codePointLength < length)
//						{
//							unsigned int nextCodepoint = nctl::Utf8::InvalidUnicode;
//							string_.utf8ToCodePoint(i + codePointLength, nextCodepoint);
//							xAdvance_ += glyph->kerning(nextCodepoint);
//						}
//					}
//				}
//				i += codePointLength; // manual increment to next codepoint
//			}
//		}
//
//		// If the string does not end with a new line character,
//		// last line height has not been taken into account before
//		if (!string_.empty() && string_[string_.length() - 1] != '\n')
//			yAdvance_ += lineHeight_;
//
//		lineLengths_.push_back(xAdvance_);
//		if (xAdvance_ > xAdvanceMax)
//			xAdvanceMax = xAdvance_;
//
//		// Update node size and anchor points
//		TextNode *mutableNode = const_cast<TextNode *>(this);
//		// Total advance on the X-axis for the longest line (horizontal boundary)
//		mutableNode->width_ = xAdvanceMax;
//		// Total advance on the Y-axis for the entire string (vertical boundary)
//		mutableNode->height_ = yAdvance_;
//		mutableNode->dirtyBits_.set(DirtyBitPositions::AabbBit);
//
//		if (oldWidth > 0.0f && oldHeight > 0.0f)
//		{
//			mutableNode->anchorPoint_.x = (anchorPoint_.x / oldWidth) * width_;
//			mutableNode->anchorPoint_.y = (anchorPoint_.y / oldHeight) * height_;
//		}
//
//		dirtyBoundaries_ = false;
//	}
//}
//
//float TextNode::calculateAlignment(unsigned int lineIndex) const
//{
//	float alignOffset = 0.0f;
//
//	switch (alignment_)
//	{
//		case Alignment::LEFT:
//			alignOffset = 0.0f;
//			break;
//		case Alignment::CENTER:
//			alignOffset = (width_ - lineLengths_[lineIndex]) * 0.5f;
//			break;
//		case Alignment::RIGHT:
//			alignOffset = width_ - lineLengths_[lineIndex];
//			break;
//	}
//
//	return alignOffset;
//}
//
//void TextNode::processGlyph(const FontGlyph *glyph, Degenerate degen)
//{
//	const Vector2i size = glyph->size();
//	const Vector2i offset = glyph->offset();
//
//	const float leftPos = xAdvance_ + offset.x;
//	const float rightPos = leftPos + size.x;
//	const float topPos = -yAdvance_ - offset.y;
//	const float bottomPos = topPos - size.y;
//
//	const Vector2i texSize = font_->texture()->size();
//	const Recti texRect = glyph->texRect();
//
//	const float leftCoord = float(texRect.x) / float(texSize.x);
//	const float rightCoord = float(texRect.x + texRect.w) / float(texSize.x);
//	const float bottomCoord = float(texRect.y + texRect.h) / float(texSize.y);
//	const float topCoord = float(texRect.y) / float(texSize.y);
//
//	if (degen == Degenerate::START || degen == Degenerate::START_END)
//		interleavedVertices_.push_back(Vertex(leftPos, bottomPos, leftCoord, bottomCoord));
//
//	interleavedVertices_.push_back(Vertex(leftPos, bottomPos, leftCoord, bottomCoord));
//	interleavedVertices_.push_back(Vertex(leftPos, topPos, leftCoord, topCoord));
//	interleavedVertices_.push_back(Vertex(rightPos, bottomPos, rightCoord, bottomCoord));
//	interleavedVertices_.push_back(Vertex(rightPos, topPos, rightCoord, topCoord));
//
//	if (degen == Degenerate::START_END || degen == Degenerate::END)
//		interleavedVertices_.push_back(Vertex(rightPos, topPos, rightCoord, topCoord));
//
//	xAdvance_ += glyph->xAdvance();
//}
//
//void TextNode::updateRenderCommand()
//{
//	if (dirtyBits_.test(DirtyBitPositions::TransformationBit))
//	{
//		renderCommand_->setTransformation(worldMatrix_);
//		dirtyBits_.reset(DirtyBitPositions::TransformationBit);
//	}
//	if (dirtyBits_.test(DirtyBitPositions::ColorBit))
//	{
//		textnodeBlock_->uniform("color")->setFloatVector(Colorf(absColor()).data());
//		dirtyBits_.reset(DirtyBitPositions::ColorBit);
//	}
//}
//
//}
