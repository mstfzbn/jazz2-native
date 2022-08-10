﻿#include "TileMap.h"

#include "../LevelHandler.h"

#include "../../nCine/Graphics/RenderQueue.h"
#include "../../nCine/IO/IFileStream.h"
#include "../../nCine/Base/Random.h"

namespace Jazz2::Tiles
{
	TileMap::TileMap(LevelHandler* levelHandler, const StringView& tileSetPath)
		:
		_levelHandler(levelHandler),
		_sprLayerIndex(-1),
		_hasPit(false),
		_renderCommandsCount(0),
		_collapsingTimer(0.0f),
		_triggerState(TriggerCount),
		_texturedBackgroundLayer(-1),
		_texturedBackgroundPass(this)
	{
		_tileSet = ContentResolver::Current().RequestTileSet(tileSetPath, true);
		_renderCommands.reserve(128);

		if (_tileSet == nullptr) {
			LOGE_X("Cannot load tileset \"%s\"", tileSetPath.data());
		}
	}

	Vector2i TileMap::Size()
	{
		if (_sprLayerIndex == -1) {
			return Vector2i();
		}

		return _layers[_sprLayerIndex].LayoutSize;
	}

	Recti TileMap::LevelBounds()
	{
		if (_sprLayerIndex == -1) {
			return Recti();
		}

		Vector2i layoutSize = _layers[_sprLayerIndex].LayoutSize;
		return Recti(0, 0, layoutSize.X * TileSet::DefaultTileSize, layoutSize.Y * TileSet::DefaultTileSize);
	}

	void TileMap::OnUpdate(float timeMult)
	{
		SceneNode::OnUpdate(timeMult);

		// Update animated tiles
		for (auto& animTile : _animatedTiles) {
			if (animTile.FrameDuration <= 0.0f || animTile.Tiles.size() < 2) {
				continue;
			}

			animTile.FramesLeft -= timeMult;
			while (animTile.FramesLeft <= 0.0f) {
				if (animTile.Forwards) {
					if (animTile.CurrentTileIdx == animTile.Tiles.size() - 1) {
						if (animTile.PingPong) {
							animTile.Forwards = false;
							animTile.FramesLeft += (animTile.FrameDuration * (1 + animTile.PingPongDelay));
						} else {
							animTile.CurrentTileIdx = 0;
							animTile.FramesLeft += (animTile.FrameDuration * (1 + animTile.Delay));
						}
					} else {
						animTile.CurrentTileIdx++;
						animTile.FramesLeft += animTile.FrameDuration;
					}
				} else {
					if (animTile.CurrentTileIdx == 0) {
						// Reverse only occurs on ping pong mode so no need to check for that here
						animTile.Forwards = true;
						animTile.FramesLeft += (animTile.FrameDuration * (1 + animTile.Delay));
					} else {
						animTile.CurrentTileIdx--;
						animTile.FramesLeft += animTile.FrameDuration;
					}
				}
			}
		}

		// Update layer scrolling
		for (auto& layer : _layers) {
			if (std::abs(layer.AutoSpeedX) > 0) {
				layer.OffsetX += layer.AutoSpeedX * timeMult;
				if (layer.RepeatX) {
					if (layer.AutoSpeedX > 0) {
						while (layer.OffsetX > (layer.LayoutSize.X * 32)) {
							layer.OffsetX -= (layer.LayoutSize.X * 32);
						}
					} else {
						while (layer.OffsetX < 0) {
							layer.OffsetX += (layer.LayoutSize.X * 32);
						}
					}
				}
			}
			if (std::abs(layer.AutoSpeedY) > 0) {
				layer.OffsetY += layer.AutoSpeedY * timeMult;
				if (layer.RepeatY) {
					if (layer.AutoSpeedY > 0) {
						while (layer.OffsetY > (layer.LayoutSize.Y * 32)) {
							layer.OffsetY -= (layer.LayoutSize.Y * 32);
						}
					} else {
						while (layer.OffsetY < 0) {
							layer.OffsetY += (layer.LayoutSize.Y * 32);
						}
					}
				}
			}
		}

		AdvanceCollapsingTileTimers(timeMult);
		UpdateDebris(timeMult);
	}

	bool TileMap::OnDraw(RenderQueue& renderQueue)
	{
		SceneNode::OnDraw(renderQueue);

		_renderCommandsCount = 0;

		for (auto& layer : _layers) {
			DrawLayer(renderQueue, layer);
		}

		DrawDebris(renderQueue);

		return true;
	}

	bool TileMap::IsTileEmpty(int x, int y)
	{
		// TODO: Is this function used correctly?
		// Consider out-of-level coordinates as solid walls
		if (x < 0 || y < 0 || _sprLayerIndex == -1) {
			return false;
		}

		Vector2i layoutSize = _layers[_sprLayerIndex].LayoutSize;
		if (x >= layoutSize.X) {
			return false;
		}
		if (y >= layoutSize.Y) {
			return _hasPit;
		}

		LayerTile& tile = _layers[_sprLayerIndex].Layout[y * layoutSize.X + x];
		int tileId = ResolveTileID(tile);
		return _tileSet->IsTileMaskEmpty(tileId);
	}

	bool TileMap::IsTileEmpty(const AABBf& aabb, TileCollisionParams& params)
	{
		if (_sprLayerIndex == -1) {
			return false;
		}

		Vector2i layoutSize = _layers[_sprLayerIndex].LayoutSize;

		int limitRightPx = layoutSize.X * TileSet::DefaultTileSize;
		int limitBottomPx = layoutSize.Y * TileSet::DefaultTileSize;

		// Consider out-of-level coordinates as solid walls
		if (aabb.L < 0 || aabb.T < 0 || aabb.R >= limitRightPx) {
			return false;
		}
		if (aabb.B >= limitBottomPx) {
			return _hasPit;
		}

		// Check all covered tiles for collisions; if all are empty, no need to do pixel collision checking
		int hx1 = (int)aabb.L;
		int hx2 = std::min((int)std::ceil(aabb.R), limitRightPx - 1);
		int hy1 = (int)aabb.T;
		int hy2 = std::min((int)std::ceil(aabb.B), limitBottomPx - 1);

		const int SmallHitboxHeight = 8;

		int hx1t = hx1 / TileSet::DefaultTileSize;
		int hx2t = hx2 / TileSet::DefaultTileSize;
		int hy1t = hy1 / TileSet::DefaultTileSize;
		int hy2t = hy2 / TileSet::DefaultTileSize;

		auto sprLayerLayout = _layers[_sprLayerIndex].Layout.get();

		for (int y = hy1t; y <= hy2t; y++) {
			for (int x = hx1t; x <= hx2t; x++) {
			RecheckTile:
				LayerTile& tile = sprLayerLayout[y * layoutSize.X + x];
				int tileId = ResolveTileID(tile);
				if (tile.SuspendType != SuspendType::None || _tileSet->IsTileMaskEmpty(tileId) || (tile.IsOneWay && !params.Downwards)) {
					continue;
				}

				int tx = x * TileSet::DefaultTileSize;
				int ty = y * TileSet::DefaultTileSize;

				int left = std::max(hx1 - tx, 0);
				int right = std::min(hx2 - tx, TileSet::DefaultTileSize - 1);
				int top = std::max(hy1 - ty, 0);
				int bottom = std::min(hy2 - ty, TileSet::DefaultTileSize - 1);

				if (tile.IsFlippedX) {
					int left2 = left;
					left = (TileSet::DefaultTileSize - 1 - right);
					right = (TileSet::DefaultTileSize - 1 - left2);
				}
				if (tile.IsFlippedY) {
					int top2 = top;
					top = (TileSet::DefaultTileSize - 1 - bottom);
					bottom = (TileSet::DefaultTileSize - 1 - top2);
				}

				top *= TileSet::DefaultTileSize;
				bottom *= TileSet::DefaultTileSize;

				uint8_t* mask = _tileSet->GetTileMask(tileId);
				for (int ry = top; ry <= bottom; ry += TileSet::DefaultTileSize) {
					for (int rx = left; rx <= right; rx++) {
						if (mask[ry | rx]) {
							if (tile.DestructType == TileDestructType::Weapon && (params.DestructType & TileDestructType::Weapon) == TileDestructType::Weapon) {
								if (params.WeaponType == WeaponType::Freezer && (_animatedTiles[tile.DestructAnimation].Tiles.size() - 2) > tile.DestructFrameIndex) {
									// TODO
									/*FrozenBlock frozen = new FrozenBlock();
									frozen.OnActivated(new ActorActivationDetails {
										LevelHandler = _levelHandler,
										Pos = new Vector3(32 * tx + 16 - 1, 32 * ty + 16 - 1, ILevelHandler::MainPlaneZ)
									});
									levelHandler.AddActor(frozen);*/
									//params.TilesDestroyed++;
									return false;
								} else if (tile.ExtraData == 0 || tile.ExtraData == ((unsigned int)params.WeaponType + 1)) {
									if (AdvanceDestructibleTileAnimation(tile, x, y, params.WeaponStrength, "SceneryDestruct"_s)) {
										params.TilesDestroyed++;
										if (params.WeaponStrength <= 0) {
											return false;
										} else {
											goto RecheckTile;
										}
									} else {
										return false;
									}
								}
							} else if (tile.DestructType == TileDestructType::Special && (params.DestructType & TileDestructType::Special) == TileDestructType::Special) {
								int amount = 1;
								if (AdvanceDestructibleTileAnimation(tile, x, y, amount, "SceneryDestruct"_s)) {
									params.TilesDestroyed++;
									goto RecheckTile;
								} else {
									return false;
								}
							} else if (tile.DestructType == TileDestructType::Speed && (params.DestructType & TileDestructType::Speed) == TileDestructType::Speed) {
								int amount = 1;
								// TODO
								if (/*tile.ExtraData +*/ 5 <= params.Speed && AdvanceDestructibleTileAnimation(tile, x, y, amount, "SceneryDestruct"_s)) {
									params.TilesDestroyed++;
									goto RecheckTile;
								} else {
									return false;
								}
							} else if (tile.DestructType == TileDestructType::Collapse && (params.DestructType & TileDestructType::Collapse) == TileDestructType::Collapse) {
								bool found = false;
								for (auto& current : _activeCollapsingTiles) {
									if (current == Vector2i(x, y)) {
										found = true;
										break;
									}
								}

								if (!found) {
									_activeCollapsingTiles.emplace_back(x, y);
									params.TilesDestroyed++;
								}
								return false;
							} else {
								return false;
							}
						}
					}
				}
			}
		}

		return true;
	}

	SuspendType TileMap::GetTileSuspendState(float x, float y)
	{
		constexpr int Tolerance = 4;

		if (x < 0 || y < 0 || _sprLayerIndex == -1) {
			return SuspendType::None;
		}

		int ax = (int)x / TileSet::DefaultTileSize;
		int ay = (int)y / TileSet::DefaultTileSize;

		Vector2i layoutSize = _layers[_sprLayerIndex].LayoutSize;
		if (ax >= layoutSize.X || ay >= layoutSize.Y) {
			return SuspendType::None;
		}

		TileMapLayer& layer = _layers[_sprLayerIndex];
		LayerTile& tile = layer.Layout[ax + ay * layer.LayoutSize.X];
		if (tile.SuspendType == SuspendType::None) {
			return SuspendType::None;
		}

		int tileId = ResolveTileID(tile);
		uint8_t* mask = _tileSet->GetTileMask(tileId);

		int rx = (int)x & 31;
		int ry = (int)y & 31;

		if (tile.IsFlippedX) {
			rx = (TileSet::DefaultTileSize - 1 - rx);
		}
		if (tile.IsFlippedY) {
			ry = (TileSet::DefaultTileSize - 1 - ry);
		}

		int top = std::max(ry - Tolerance, 0) << 5;
		int bottom = std::min(ry + Tolerance, TileSet::DefaultTileSize - 1) << 5;

		for (int ti = bottom | rx; ti >= top; ti -= TileSet::DefaultTileSize) {
			if (mask[ti]) {
				return tile.SuspendType;
			}
		}

		return SuspendType::None;
	}

	bool TileMap::AdvanceDestructibleTileAnimation(LayerTile& tile, int tx, int ty, int& amount, const StringView& soundName)
	{
		int max = (int)(_animatedTiles[tile.DestructAnimation].Tiles.size() - 2);
		if (tile.DestructFrameIndex < max) {
			// Tile not destroyed yet, advance counter by one
			int current = std::min(amount, max - tile.DestructFrameIndex);

			tile.DestructFrameIndex += current;
			tile.TileID = _animatedTiles[tile.DestructAnimation].Tiles[tile.DestructFrameIndex].TileID;
			if (tile.DestructFrameIndex >= max) {
				if (!soundName.empty()) {
					_levelHandler->PlayCommonSfx(soundName, Vector3f(tx * TileSet::DefaultTileSize + (TileSet::DefaultTileSize / 2),
						ty * TileSet::DefaultTileSize + (TileSet::DefaultTileSize / 2), 0.0f));
				}
				AnimatedTile& anim = _animatedTiles[tile.DestructAnimation];
				CreateTileDebris(anim.Tiles[anim.Tiles.size() - 1].TileID, tx, ty);
			}

			amount -= current;

#if MULTIPLAYER && SERVER
			((LevelHandler)levelHandler).OnAdvanceDestructibleTileAnimation(tx, ty, current);
#endif
			return true;
		}
		return false;
	}

	void TileMap::AdvanceCollapsingTileTimers(float timeMult)
	{
		_collapsingTimer -= timeMult;
		if (_collapsingTimer > 0.0f) {
			return;
		}

		_collapsingTimer = 1.0f;

		Vector2i layoutSize = _layers[_sprLayerIndex].LayoutSize;

		for (int i = 0; i < _activeCollapsingTiles.size(); i++) {
			Vector2i tilePos = _activeCollapsingTiles[i];
			auto& tile = _layers[_sprLayerIndex].Layout[tilePos.X + tilePos.Y * layoutSize.X];
			if (tile.ExtraData == 0) {
				int amount = 1;
				if (!AdvanceDestructibleTileAnimation(tile, tilePos.X, tilePos.Y, amount, "SceneryCollapse"_s)) {
					tile.DestructType = TileDestructType::None;
					_activeCollapsingTiles.erase(_activeCollapsingTiles.begin() + i);
					i--;
				} else {
					tile.ExtraData = 4;
				}
			} else {
				tile.ExtraData--;
			}
		}
	}

	void TileMap::DrawLayer(RenderQueue& renderQueue, TileMapLayer& layer)
	{
		if (!layer.Visible) {
			return;
		}

		Vector2i viewSize = _levelHandler->GetViewSize();
		Vector2f viewCenter = _levelHandler->GetCameraPos();

		Vector2i tileCount = layer.LayoutSize;
		Vector2i tileSize = Vector2i(TileSet::DefaultTileSize, TileSet::DefaultTileSize);

		// Get current layer offsets and speeds
		float loX = layer.OffsetX;
		float loY = layer.OffsetY - (layer.UseInherentOffset ? (viewSize.Y - 200) / 2 : 0) + 1;

		// Find out coordinates for a tile from outside the boundaries from topleft corner of the screen 
		float x1 = viewCenter.X - 70 - (viewSize.X * 0.5f);
		float y1 = viewCenter.Y - 70 - (viewSize.Y * 0.5f);

		if (layer.BackgroundStyle != BackgroundStyle::Plain && tileCount.Y == 8 && tileCount.X == 8) {
			constexpr float PerspectiveSpeedX = 0.4f;
			constexpr float PerspectiveSpeedY = 0.16f;
			RenderTexturedBackground(renderQueue, layer, x1 * PerspectiveSpeedX + loX, y1 * PerspectiveSpeedY + loY);
		} else {
			// Figure out the floating point offset from the calculated coordinates and the actual tile corner coordinates
			float xt = TranslateCoordinate(x1, layer.SpeedX, loX, false, viewSize.Y, viewSize.X);
			float yt = TranslateCoordinate(y1, layer.SpeedY, loY, true, viewSize.Y, viewSize.X);

			float remX = fmod(xt, (float)TileSet::DefaultTileSize);
			float remY = fmod(yt, (float)TileSet::DefaultTileSize);

			// Calculate the index (on the layer map) of the first tile that needs to be drawn to the position determined earlier
			int tileX, tileY, tileAbsX, tileAbsY;

			// Get the actual tile coords on the layer layout
			if (xt > 0) {
				tileAbsX = (int)std::floor(xt / (float)TileSet::DefaultTileSize);
				tileX = tileAbsX % tileCount.X;
			} else {
				tileAbsX = (int)std::ceil(xt / (float)TileSet::DefaultTileSize);
				tileX = tileAbsX % tileCount.X;
				while (tileX < 0) {
					tileX += tileCount.X;
				}
			}

			if (yt > 0) {
				tileAbsY = (int)std::floor(yt / (float)TileSet::DefaultTileSize);
				tileY = tileAbsY % tileCount.Y;
			} else {
				tileAbsY = (int)std::ceil(yt / (float)TileSet::DefaultTileSize);
				tileY = tileAbsY % tileCount.Y;
				while (tileY < 0) {
					tileY += tileCount.Y;
				}
			}

			// update x1 and y1 with the remainder so that we start at the tile boundary
			// minus 1 because indices are updated in the beginning of the loops
			x1 -= remX - (float)TileSet::DefaultTileSize;
			y1 -= remY - (float)TileSet::DefaultTileSize;

			// Save the tile Y at the left border so that we can roll back to it at the start of every row iteration
			int tileYs = tileY;

			// Calculate the last coordinates we want to draw to
			float x3 = x1 + 100 + viewSize.X;
			float y3 = y1 + 100 + viewSize.Y;

			int tile_xo = -1;
			for (float x2 = x1; x2 < x3; x2 += TileSet::DefaultTileSize) {
				tileX = (tileX + 1) % tileCount.X;
				tile_xo++;
				if (!layer.RepeatX) {
					// If the current tile isn't in the first iteration of the layer horizontally, don't draw this column
					if (tileAbsX + tile_xo + 1 < 0 || tileAbsX + tile_xo + 1 >= tileCount.X) {
						continue;
					}
				}
				tileY = tileYs;
				int tile_yo = -1;
				for (float y2 = y1; y2 < y3; y2 += TileSet::DefaultTileSize) {
					tileY = (tileY + 1) % tileCount.Y;
					tile_yo++;

					LayerTile tile = layer.Layout[tileX + tileY * layer.LayoutSize.X];

					if (!layer.RepeatY) {
						// If the current tile isn't in the first iteration of the layer vertically, don't draw it
						if (tileAbsY + tile_yo + 1 < 0 || tileAbsY + tile_yo + 1 >= tileCount.Y) {
							continue;
						}
					}

					int tileId;
					bool isFlippedX, isFlippedY;
					int alpha;
					if (tile.IsAnimated) {
						if (tile.TileID < _animatedTiles.size()) {
							tileId = _animatedTiles[tile.TileID].Tiles[_animatedTiles[tile.TileID].CurrentTileIdx].TileID;
						} else {
							continue;
						}
					} else {
						tileId = tile.TileID;
					}

					isFlippedX = tile.IsFlippedX;
					isFlippedY = tile.IsFlippedY;
					alpha = tile.Alpha;

					if (alpha == 0) {
						continue;
					}

					auto command = RentRenderCommand();

					Vector2i texSize = _tileSet->TextureDiffuse->size();
					float texScaleX = TileSet::DefaultTileSize / float(texSize.X);
					float texBiasX = (tileId % _tileSet->TilesPerRow) * TileSet::DefaultTileSize / float(texSize.X);
					float texScaleY = TileSet::DefaultTileSize / float(texSize.Y);
					float texBiasY = (tileId / _tileSet->TilesPerRow) * TileSet::DefaultTileSize / float(texSize.Y);

					// ToDo: Flip normal map somehow
					if (isFlippedX) {
						texBiasX += texScaleX;
						texScaleX *= -1;
					}
					if (isFlippedY) {
						texBiasY += texScaleY;
						texScaleY *= -1;
					}

					if ((viewSize.X & 1) == 1) {
						texBiasX += 0.5f / float(texSize.X);
					}
					if ((viewSize.Y & 1) == 1) {
						texBiasY -= 0.5f / float(texSize.Y);
					}

					auto instanceBlock = command->material().uniformBlock(Material::InstanceBlockName);
					instanceBlock->uniform(Material::TexRectUniformName)->setFloatValue(texScaleX, texBiasX, texScaleY, texBiasY);
					instanceBlock->uniform(Material::SpriteSizeUniformName)->setFloatValue(TileSet::DefaultTileSize, TileSet::DefaultTileSize);
					instanceBlock->uniform(Material::ColorUniformName)->setFloatVector(Colorf(1.0f, 1.0f, 1.0f, alpha / 255.0f).Data());

					command->setTransformation(Matrix4x4f::Translation(std::floor(x2 + (TileSet::DefaultTileSize / 2)), std::floor(y2 + (TileSet::DefaultTileSize / 2)), 0.0f));
					command->setLayer(layer.Depth);
					command->material().setTexture(*_tileSet->TextureDiffuse);

					renderQueue.addCommand(command);
				}
			}
		}
	}

	float TileMap::TranslateCoordinate(float coordinate, float speed, float offset, bool isY, int viewHeight, int viewWidth)
	{
		// Coordinate: the "vanilla" coordinate of the tile on the layer if the layer was fixed to the sprite layer with same
		// speed and no other options. Think of its position in JCS.
		// Speed: the set layer speed; 1 for anything that moves the same speed as the sprite layer (where the objects live),
		// less than 1 for backgrounds that move slower, more than 1 for foregrounds that move faster
		// Offset: any difference to starting coordinates caused by an inherent automatic speed a layer has

		// Literal 70 is the same as in DrawLayer, it's the offscreen offset of the first tile to draw.
		// Don't touch unless absolutely necessary.
		return (coordinate * speed + offset + (70 + (isY ? (viewHeight - 200) : (viewWidth - 320)) / 2) * (speed - 1));
	}

	RenderCommand* TileMap::RentRenderCommand()
	{
		if (_renderCommandsCount < _renderCommands.size()) {
			RenderCommand* command = _renderCommands[_renderCommandsCount].get();
			_renderCommandsCount++;
			return command;
		} else {
			std::unique_ptr<RenderCommand>& command = _renderCommands.emplace_back(std::make_unique<RenderCommand>());
			command->setType(RenderCommand::CommandTypes::SPRITE);
			command->material().setShaderProgramType(Material::ShaderProgramType::SPRITE);
			command->material().setBlendingEnabled(true);
			command->material().reserveUniformsDataMemory();
			command->geometry().setDrawParameters(GL_TRIANGLE_STRIP, 0, 4);

			GLUniformCache* textureUniform = command->material().uniform(Material::TextureUniformName);
			if (textureUniform && textureUniform->intValue(0) != 0) {
				textureUniform->setIntValue(0); // GL_TEXTURE0
			}
			return command.get();
		}
	}

	void TileMap::ReadLayerConfiguration(LayerType type, const std::unique_ptr<IFileStream>& s, const LayerDescription& layer)
	{
		s->Open(FileAccessMode::Read);
		if (s->GetSize() < 8) {
			return;
		}

		int32_t width = s->ReadValue<int32_t>();
		int32_t height = s->ReadValue<int32_t>();

		std::unique_ptr<LayerTile[]> layout = std::make_unique<LayerTile[]>(width * height);

		for (int i = 0; i < (width * height); i++) {
			uint16_t tileType = s->ReadValue<uint16_t>();

			uint8_t flags = s->ReadValue<uint8_t>();
			bool isFlippedX = (flags & 0x01) != 0;
			bool isFlippedY = (flags & 0x02) != 0;
			bool isAnimated = (flags & 0x04) != 0;
			uint8_t tileModifier = (uint8_t)(flags >> 4);

			LayerTile& tile = layout[i];
			tile.TileID = tileType;

			tile.IsFlippedX = isFlippedX;
			tile.IsFlippedY = isFlippedY;
			tile.IsAnimated = isAnimated;

			if (tileModifier == 1 /*Translucent*/) {
				tile.Alpha = /*127*/140;
			} else if (tileModifier == 2 /*Invisible*/) {
				tile.Alpha = 0;
			} else {
				tile.Alpha = 255;
			}
		}

		if (type == LayerType::Sprite) {
			_sprLayerIndex = (int)_layers.size();
		} else if (layer.BackgroundStyle != BackgroundStyle::Plain) {
			_texturedBackgroundLayer = (int)_layers.size();
		}

		TileMapLayer& newLayer = _layers.emplace_back();
		newLayer.Visible = true;
		newLayer.LayoutSize = Vector2i(width, height);
		newLayer.Layout = std::move(layout);

		newLayer.SpeedX = layer.SpeedX;
		newLayer.SpeedY = layer.SpeedY;
		newLayer.AutoSpeedX = layer.AutoSpeedX;
		newLayer.AutoSpeedY = layer.AutoSpeedY;
		newLayer.RepeatX = layer.RepeatX;
		newLayer.RepeatY = layer.RepeatY;
		newLayer.OffsetX = layer.OffsetX;
		newLayer.OffsetY = layer.OffsetY;
		newLayer.UseInherentOffset = layer.UseInherentOffset;
		newLayer.Depth = (uint16_t)(ILevelHandler::MainPlaneZ + layer.Depth);
		newLayer.BackgroundStyle = layer.BackgroundStyle;
		newLayer.BackgroundColor = layer.BackgroundColor;
		newLayer.ParallaxStarsEnabled = layer.ParallaxStarsEnabled;
	}

	void TileMap::ReadAnimatedTiles(const std::unique_ptr<IFileStream>& s)
	{
		s->Open(FileAccessMode::Read);

		if (s->GetSize() < 4) {
			return;
		}

		int32_t count = s->ReadValue<int32_t>();

		_animatedTiles.reserve(count);

		for (int i = 0; i < count; i++) {
			uint16_t frameCount = s->ReadValue<uint16_t>();
			if (frameCount == 0) {
				continue;
			}

			AnimatedTile& animTile = _animatedTiles.emplace_back();

			for (int j = 0; j < frameCount; j++) {
				auto& frame = animTile.Tiles.emplace_back();
				frame.TileID = s->ReadValue<uint16_t>();
				// TODO: flags
				uint8_t flag = s->ReadValue<uint8_t>();
			}

			// TODO: Adjust FPS in Import
			uint8_t speed = s->ReadValue<uint8_t>();
			animTile.FrameDuration = 70.0f / (speed * 14 / 10);
			animTile.Delay = s->ReadValue<uint16_t>();

			//animTile.DelayJitter = s->ReadValue<uint16_t>();
			uint16_t delayJitter = s->ReadValue<uint16_t>();

			animTile.PingPong = s->ReadValue<uint8_t>();
			animTile.PingPongDelay = s->ReadValue<uint16_t>();
		}
	}

	void TileMap::SetTileEventFlags(int x, int y, EventType tileEvent, uint8_t* tileParams)
	{
		auto& tile = _layers[_sprLayerIndex].Layout[x + y * _layers[_sprLayerIndex].LayoutSize.X];

		switch (tileEvent) {
			case EventType::ModifierOneWay:
				tile.IsOneWay = true;
				break;
			case EventType::ModifierVine:
				tile.SuspendType = SuspendType::Vine;
				break;
			case EventType::ModifierHook:
				tile.SuspendType = SuspendType::Hook;
				break;
			case EventType::SceneryDestruct:
				SetTileDestructibleEventFlag(tile, TileDestructType::Weapon, tileParams[0]);
				break;
			case EventType::SceneryDestructButtstomp:
				SetTileDestructibleEventFlag(tile, TileDestructType::Special, tileParams[0]);
				break;
			case EventType::TriggerArea:
				SetTileDestructibleEventFlag(tile, TileDestructType::Trigger, tileParams[0]);
				break;
			case EventType::SceneryDestructSpeed:
				SetTileDestructibleEventFlag(tile, TileDestructType::Speed, tileParams[0]);
				break;
			case EventType::SceneryCollapse:
				// ToDo: FPS (tileParams[1]) not used...
				SetTileDestructibleEventFlag(tile, TileDestructType::Collapse, tileParams[0]);
				break;
		}
	}

	void TileMap::SetTileDestructibleEventFlag(LayerTile& tile, TileDestructType type, uint16_t extraData)
	{
		if (!tile.IsAnimated) {
			return;
		}

		tile.DestructType = type;
		tile.IsAnimated = false;
		tile.DestructAnimation = tile.TileID;
		tile.TileID = _animatedTiles[tile.DestructAnimation].Tiles[0].TileID;
		tile.DestructFrameIndex = 0;
		//tile.MaterialOffset = tileset.GetTileTextureRect(tile.TileID);
		tile.ExtraData = extraData;
	}

	void TileMap::CreateDebris(const DestructibleDebris& debris)
	{
		auto& spriteLayer = _layers[_sprLayerIndex];
		if (debris.CollisionAction == DebrisCollisionAction::Disappear && debris.Depth <= spriteLayer.Depth) {
			int x = (int)debris.Pos.X / TileSet::DefaultTileSize;
			int y = (int)debris.Pos.Y / TileSet::DefaultTileSize;
			if (x < 0 || y < 0 || x >= spriteLayer.LayoutSize.X || y >= spriteLayer.LayoutSize.Y) {
				return;
			}

			int tileId = ResolveTileID(spriteLayer.Layout[x + y * spriteLayer.LayoutSize.X]);
			if (_tileSet->IsTileFilled(tileId)) {
				return;
			}

			if (_sprLayerIndex + 1 < _layers.size() && _layers[_sprLayerIndex + 1].SpeedX == 1.0f && _layers[_sprLayerIndex + 1].SpeedY == 1.0f) {
				tileId = ResolveTileID(_layers[_sprLayerIndex + 1].Layout[x + y * spriteLayer.LayoutSize.X]);
				if (_tileSet->IsTileFilled(tileId)) {
					return;
				}
			}
		}

		_debrisList.push_back(debris);
	}

	void TileMap::CreateTileDebris(int tileId, int x, int y)
	{
		constexpr float speedMultiplier[] = { -2, 2, -1, 1 };
		constexpr int quarterSize = TileSet::DefaultTileSize / 2;

		uint16_t z = _layers[_sprLayerIndex].Depth + 80;

		Vector2i texSize = _tileSet->TextureDiffuse->size();
		float texScaleX = float(quarterSize) / float(texSize.X);
		float texBiasX = ((tileId % _tileSet->TilesPerRow) * TileSet::DefaultTileSize) / float(texSize.X);
		float texScaleY = float(quarterSize) / float(texSize.Y);
		float texBiasY = ((tileId / _tileSet->TilesPerRow) * TileSet::DefaultTileSize) / float(texSize.Y);

		/*if (isFlippedX) {
			texBiasX += texScaleX;
			texScaleX *= -1;
		}
		if (isFlippedY) {
			texBiasY += texScaleY;
			texScaleY *= -1;
		}*/

		for (int i = 0; i < 4; i++) {
			DestructibleDebris& debris = _debrisList.emplace_back();
			debris.Pos = Vector2f(x * TileSet::DefaultTileSize + (i % 2) * quarterSize, y * TileSet::DefaultTileSize + (i / 2) * quarterSize);
			debris.Depth = z;
			debris.Size = Vector2f(quarterSize, quarterSize);
			debris.Speed = Vector2f(speedMultiplier[i] * nCine::Random().NextFloat(0.8f, 1.2f), -4.0f * nCine::Random().NextFloat(0.8f, 1.2f));
			debris.Acceleration = Vector2f(0.0f, 0.3f);

			debris.Scale = 1.0f;
			debris.ScaleSpeed = nCine::Random().NextFloat(-0.01f, -0.002f);
			debris.Angle = 0.0f;
			debris.AngleSpeed = speedMultiplier[i] * nCine::Random().NextFloat(0.0f, 0.014f);

			debris.Alpha = 1.0f;
			debris.AlphaSpeed = -0.01f;

			debris.Time = 120.0f;

			debris.TexScaleX = texScaleX;
			debris.TexBiasX = texBiasX + ((i % 2) * quarterSize / float(texSize.X));
			debris.TexScaleY = texScaleY;
			debris.TexBiasY = texBiasY + ((i / 2) * quarterSize / float(texSize.Y));

			debris.DiffuseTexture = _tileSet->TextureDiffuse.get();
			debris.CollisionAction = DebrisCollisionAction::None;
		}
	}

	void TileMap::CreateParticleDebris(const GraphicResource* res, Vector3f pos, Vector2f force, int currentFrame, bool isFacingLeft)
	{
		constexpr int DebrisSize = 3;

		float x = pos.X - res->Base->Hotspot.X;
		float y = pos.Y - res->Base->Hotspot.Y;
		Vector2i texSize = res->Base->TextureDiffuse->size();

		for (int fx = 0; fx < res->Base->FrameDimensions.X; fx += DebrisSize + 1) {
			for (int fy = 0; fy < res->Base->FrameDimensions.Y; fy += DebrisSize + 1) {
				float currentSize = DebrisSize * nCine::Random().NextFloat(0.2f, 1.1f);

				DestructibleDebris& debris = _debrisList.emplace_back();
				debris.Pos = Vector2f(x + (isFacingLeft ? res->Base->FrameDimensions.X - fx : fx), y + fy);
				debris.Depth = (uint16_t)pos.Z;
				debris.Size = Vector2f(currentSize, currentSize);
				debris.Speed = Vector2f(force.X + ((fx - res->Base->FrameDimensions.X / 2) + nCine::Random().NextFloat(-2.0f, 2.0f)) * (isFacingLeft ? -1.0f : 1.0f) * nCine::Random().NextFloat(2.0f, 8.0f) / res->Base->FrameDimensions.X,
						force.Y - 1.0f * nCine::Random().NextFloat(2.2f, 4.0f));
				debris.Acceleration = Vector2f(0.0f, 0.2f);

				debris.Scale = 1.0f;
				debris.ScaleSpeed = 0.0f;
				debris.Angle = 0.0f;
				debris.AngleSpeed = 0.0f;

				debris.Alpha = 1.0f;
				debris.AlphaSpeed = -0.002f;

				debris.Time = 320.0f;

				debris.TexScaleX = (currentSize / float(texSize.X));
				debris.TexBiasX = (((float)(currentFrame % res->Base->FrameConfiguration.X) / res->Base->FrameConfiguration.X) + ((float)fx / float(texSize.X)));
				debris.TexScaleY = (currentSize / float(texSize.Y));
				debris.TexBiasY = (((float)(currentFrame / res->Base->FrameConfiguration.X) / res->Base->FrameConfiguration.Y) + ((float)fy / float(texSize.Y)));

				debris.DiffuseTexture = res->Base->TextureDiffuse.get();
				debris.CollisionAction = DebrisCollisionAction::Bounce;
			}
		}
	}

	void TileMap::CreateSpriteDebris(const GraphicResource* res, Vector3f pos, int count)
	{
		float x = pos.X - res->Base->Hotspot.X;
		float y = pos.Y - res->Base->Hotspot.Y;
		Vector2i texSize = res->Base->TextureDiffuse->size();

		for (int i = 0; i < count; i++) {
			float speedX = nCine::Random().NextFloat(-1.0f, 1.0f) * nCine::Random().NextFloat(0.2f, 0.8f) * count;

			DestructibleDebris& debris = _debrisList.emplace_back();
			debris.Pos = Vector2f(x, y);
			debris.Depth = (uint16_t)pos.Z;
			debris.Size = Vector2f((float)res->Base->FrameDimensions.X, (float)res->Base->FrameDimensions.Y);
			debris.Speed = Vector2f(speedX, -1.0f * nCine::Random().NextFloat(2.2f, 4.0f));
			debris.Acceleration = Vector2f(0.0f, 0.2f);

			debris.Scale = 1.0f;
			debris.ScaleSpeed = -0.002f;
			debris.Angle = nCine::Random().NextFloat(0.0f, fTwoPi);
			debris.AngleSpeed = speedX * 0.02f;

			debris.Alpha = 1.0f;
			debris.AlphaSpeed = -0.002f;

			debris.Time = 560.0f;

			int curAnimFrame = res->FrameOffset + nCine::Random().Next(0, res->FrameCount);
			int col = curAnimFrame % res->Base->FrameConfiguration.X;
			int row = curAnimFrame / res->Base->FrameConfiguration.X;
			debris.TexScaleX = (float(res->Base->FrameDimensions.X) / float(texSize.X));
			debris.TexBiasX = (float(res->Base->FrameDimensions.X * col) / float(texSize.X));
			debris.TexScaleY = (float(res->Base->FrameDimensions.Y) / float(texSize.Y));
			debris.TexBiasY = (float(res->Base->FrameDimensions.Y * row) / float(texSize.Y));

			debris.DiffuseTexture = res->Base->TextureDiffuse.get();
			debris.CollisionAction = DebrisCollisionAction::Bounce;
		}
	}

	void TileMap::UpdateDebris(float timeMult)
	{
		for (int i = (int)_debrisList.size() - 1; i >= 0; i--) {
			DestructibleDebris& debris = _debrisList[i];

			debris.Time -= timeMult;
			if (debris.Scale <= 0.0f || debris.Alpha <= 0.0f) {
				_debrisList.erase(_debrisList.begin() + i);
				continue;
			}
			if (debris.Time <= 0.0f) {
				debris.AlphaSpeed = -std::min(0.02f, debris.Alpha);
			}

			if (debris.CollisionAction != DebrisCollisionAction::None) {
				// Debris should collide with tilemap
				float nx = debris.Pos.X + debris.Speed.X * timeMult;
				float ny = debris.Pos.Y + debris.Speed.Y * timeMult;
				AABB aabb = AABBf(nx - 1, ny - 1, nx + 1, ny + 1);
				TileCollisionParams params = { TileDestructType::None, true };
				if (IsTileEmpty(aabb, params)) {
					// Nothing...
				} else if (debris.CollisionAction == DebrisCollisionAction::Disappear) {
					debris.ScaleSpeed = -0.02f;
					debris.AlphaSpeed = -0.006f;
					debris.Speed = Vector2f::Zero;
					debris.Acceleration = Vector2f::Zero;
				} else {
					// Place us to the ground only if no horizontal movement was
					// involved (this prevents speeds resetting if the actor
					// collides with a wall from the side while in the air)
					aabb.T = debris.Pos.Y - 1;
					aabb.B = debris.Pos.Y + 1;

					if (IsTileEmpty(aabb, params)) {
						if (debris.Speed.Y > 0.0f) {
							debris.Speed.Y = -(0.8f/*elasticity*/ * debris.Speed.Y);
							//OnHitFloorHook();
						} else {
							debris.Speed.Y = 0;
							//OnHitCeilingHook();
						}
					}

					// If the actor didn't move all the way horizontally,
					// it hit a wall (or was already touching it)
					aabb = AABBf(debris.Pos.X - 1, ny - 1, debris.Pos.X + 1, ny + 1);
					if (IsTileEmpty(aabb, params)) {
						debris.Speed.X = -(0.8f/*elasticity*/ * debris.Speed.X);
						debris.AngleSpeed = -(0.8f/*elasticity*/ * debris.AngleSpeed);
						//OnHitWallHook();
					}
				}
			}

			debris.Pos.X += debris.Speed.X * timeMult + 0.5f * debris.Acceleration.X * timeMult * timeMult;
			debris.Pos.Y += debris.Speed.Y * timeMult + 0.5f * debris.Acceleration.Y * timeMult * timeMult;

			if (debris.Acceleration.X != 0.0f) {
				debris.Speed.X = std::min(debris.Speed.X + debris.Acceleration.X * timeMult, 10.0f);
			}
			if (debris.Acceleration.Y != 0.0f) {
				debris.Speed.Y = std::min(debris.Speed.Y + debris.Acceleration.Y * timeMult, 10.0f);
			}

			debris.Scale += debris.ScaleSpeed * timeMult;
			debris.Angle += debris.AngleSpeed * timeMult;
			debris.Alpha += debris.AlphaSpeed * timeMult;
		}
	}

	void TileMap::DrawDebris(RenderQueue& renderQueue)
	{
		for (auto& debris : _debrisList) {
			auto command = RentRenderCommand();

			auto instanceBlock = command->material().uniformBlock(Material::InstanceBlockName);
			instanceBlock->uniform(Material::TexRectUniformName)->setFloatValue(debris.TexScaleX, debris.TexBiasX, debris.TexScaleY, debris.TexBiasY);
			instanceBlock->uniform(Material::SpriteSizeUniformName)->setFloatValue(debris.Size.X, debris.Size.Y);
			instanceBlock->uniform(Material::ColorUniformName)->setFloatVector(Colorf(1.0f, 1.0f, 1.0f, debris.Alpha).Data());

			Matrix4x4f worldMatrix = Matrix4x4f::Translation(debris.Pos.X, debris.Pos.Y, 0.0f);
			worldMatrix.RotateZ(debris.Angle);
			worldMatrix.Scale(debris.Scale, debris.Scale, 1.0f);
			command->setTransformation(worldMatrix);
			command->setLayer(debris.Depth);
			command->material().setTexture(*debris.DiffuseTexture);

			renderQueue.addCommand(command);
		}
	}

	bool TileMap::GetTrigger(uint16_t triggerId)
	{
		return _triggerState[triggerId];
	}

	void TileMap::SetTrigger(uint16_t triggerId, bool newState)
	{
		if (_triggerState[triggerId] == newState) {
			return;
		}

		_triggerState.Set(triggerId, newState);

		// Go through all tiles and update any that are influenced by this trigger
		Vector2i layoutSize = _layers[_sprLayerIndex].LayoutSize;
		int n = layoutSize.X * layoutSize.Y;
		for (int i = 0; i < n; i++) {
			LayerTile& tile = _layers[_sprLayerIndex].Layout[i];
			if (tile.DestructType == TileDestructType::Trigger && tile.ExtraData == triggerId) {
				if (_animatedTiles[tile.DestructAnimation].Tiles.size() > 1) {
					tile.DestructFrameIndex = (newState ? 1 : 0);
					tile.TileID = _animatedTiles[tile.DestructAnimation].Tiles[tile.DestructFrameIndex].TileID;
				}
			}
		}
	}

	void TileMap::RenderTexturedBackground(RenderQueue& renderQueue, TileMapLayer& layer, float x, float y)
	{
		auto target = _texturedBackgroundPass._target.get();
		if (target == nullptr) {
			return;
		}

		Vector2i viewSize = _levelHandler->GetViewSize();
		Vector2f viewCenter = _levelHandler->GetCameraPos();

		auto command = &_texturedBackgroundPass._outputRenderCommand;

		auto instanceBlock = command->material().uniformBlock(Material::InstanceBlockName);
		instanceBlock->uniform(Material::TexRectUniformName)->setFloatValue(1.0f, 0.0f, 1.0f, 0.0f);
		instanceBlock->uniform(Material::SpriteSizeUniformName)->setFloatValue(viewSize.X, viewSize.Y);
		instanceBlock->uniform(Material::ColorUniformName)->setFloatVector(Colorf(1.0f, 1.0f, 1.0f, 1.0f).Data());

		command->material().uniform("ViewSize")->setFloatValue(viewSize.X, viewSize.Y);
		command->material().uniform("CameraPosition")->setFloatValue(viewCenter.X, viewCenter.Y);
		command->material().uniform("shift")->setFloatValue(x, y);
		command->material().uniform("horizonColor")->setFloatValue(layer.BackgroundColor.X, layer.BackgroundColor.Y, layer.BackgroundColor.Z);
		command->material().uniform("parallaxStarsEnabled")->setFloatValue(layer.ParallaxStarsEnabled ? 1.0f : 0.0f);

		command->setTransformation(Matrix4x4f::Translation(viewCenter.X, viewCenter.Y, 0.0f));
		command->setLayer(layer.Depth);
		command->material().setTexture(*target);

		renderQueue.addCommand(command);
	}

	void TileMap::OnInitializeViewport(int width, int height)
	{
		if (_texturedBackgroundLayer != -1) {
			_texturedBackgroundPass.Initialize();
		}
	}

	void TileMap::TexturedBackgroundPass::Initialize()
	{
		bool notInitialized = (_view == nullptr);

		if (notInitialized) {
			Vector2i layoutSize = _owner->_layers[_owner->_texturedBackgroundLayer].LayoutSize;
			int width = layoutSize.X * TileSet::DefaultTileSize;
			int height = layoutSize.Y * TileSet::DefaultTileSize;

			_camera = std::make_unique<Camera>();
			_camera->setOrthoProjection(0, width, 0, height);
			_camera->setView(0, 0, 0, 1);
			_target = std::make_unique<Texture>(nullptr, Texture::Format::RGB8, width, height);
			_view = std::make_unique<Viewport>(_target.get(), Viewport::DepthStencilFormat::NONE);
			_view->setRootNode(this);
			_view->setCamera(_camera.get());
			_view->setClearMode(Viewport::ClearMode::NEVER);
			_target->setMagFiltering(SamplerFilter::Linear);
			_target->setWrap(SamplerWrapping::Repeat);

			// Prepare render commands
			int renderCommandCount = (width * height) / (TileSet::DefaultTileSize * TileSet::DefaultTileSize);
			_renderCommands.reserve(renderCommandCount);
			for (int i = 0; i < renderCommandCount; i++) {
				std::unique_ptr<RenderCommand>& command = _renderCommands.emplace_back(std::make_unique<RenderCommand>());
				command->setType(RenderCommand::CommandTypes::SPRITE);
				command->material().setShaderProgramType(Material::ShaderProgramType::SPRITE);
				command->material().reserveUniformsDataMemory();
				command->geometry().setDrawParameters(GL_TRIANGLE_STRIP, 0, 4);

				GLUniformCache* textureUniform = command->material().uniform(Material::TextureUniformName);
				if (textureUniform && textureUniform->intValue(0) != 0) {
					textureUniform->setIntValue(0); // GL_TEXTURE0
				}
			}

			// Prepare output render command
			_outputRenderCommand.setType(RenderCommand::CommandTypes::SPRITE);
			_outputRenderCommand.material().setShader(ContentResolver::Current().GetShader(_owner->_layers[_owner->_texturedBackgroundLayer].BackgroundStyle == BackgroundStyle::Circle
				? PrecompiledShader::TexturedBackgroundCircle
				: PrecompiledShader::TexturedBackground));
			_outputRenderCommand.material().reserveUniformsDataMemory();
			_outputRenderCommand.geometry().setDrawParameters(GL_TRIANGLE_STRIP, 0, 4);

			GLUniformCache* textureUniform = _outputRenderCommand.material().uniform(Material::TextureUniformName);
			if (textureUniform && textureUniform->intValue(0) != 0) {
				textureUniform->setIntValue(0); // GL_TEXTURE0
			}
		}

		Viewport::chain().push_back(_view.get());
	}

	bool TileMap::TexturedBackgroundPass::OnDraw(RenderQueue& renderQueue)
	{
		TileMapLayer& layer = _owner->_layers[_owner->_texturedBackgroundLayer];
		Vector2i layoutSize = layer.LayoutSize;
		Vector2i targetSize = _target->size();

		int renderCommandIndex = 0;
		bool isAnimated = false;

		for (int y = 0; y < layoutSize.Y; y++) {
			for (int x = 0; x < layoutSize.X; x++) {
				LayerTile& tile = layer.Layout[x + y * layer.LayoutSize.X];

				int tileId;
				bool isFlippedX, isFlippedY;
				if (tile.IsAnimated) {
					isAnimated = true;
					if (tile.TileID < _owner->_animatedTiles.size()) {
						tileId = _owner->_animatedTiles[tile.TileID].Tiles[_owner->_animatedTiles[tile.TileID].CurrentTileIdx].TileID;
						// TODO
						//isFlippedX = (_animatedTiles[tile.TileID].CurrentTile.IsFlippedX != tile.IsFlippedX);
						//isFlippedY = (_animatedTiles[tile.TileID].CurrentTile.IsFlippedY != tile.IsFlippedY);
						isFlippedX = false;
						isFlippedY = false;
					} else {
						continue;
					}
				} else {
					tileId = tile.TileID;
					isFlippedX = tile.IsFlippedX;
					isFlippedY = tile.IsFlippedY;
				}

				auto command = _renderCommands[renderCommandIndex++].get();

				Vector2i texSize = _owner->_tileSet->TextureDiffuse->size();
				float texScaleX = TileSet::DefaultTileSize / float(texSize.X);
				float texBiasX = (tileId % _owner->_tileSet->TilesPerRow) * TileSet::DefaultTileSize / float(texSize.X);
				float texScaleY = TileSet::DefaultTileSize / float(texSize.Y);
				float texBiasY = (tileId / _owner->_tileSet->TilesPerRow) * TileSet::DefaultTileSize / float(texSize.Y);

				// ToDo: Flip normal map somehow
				if (isFlippedX) {
					texBiasX += texScaleX;
					texScaleX *= -1;
				}
				if (isFlippedY) {
					texBiasY += texScaleY;
					texScaleY *= -1;
				}

				if ((targetSize.X & 1) == 1) {
					texBiasX += 0.5f / float(texSize.X);
				}
				if ((targetSize.Y & 1) == 1) {
					texBiasY -= 0.5f / float(texSize.Y);
				}

				auto instanceBlock = command->material().uniformBlock(Material::InstanceBlockName);
				instanceBlock->uniform(Material::TexRectUniformName)->setFloatValue(texScaleX, texBiasX, texScaleY, texBiasY);
				instanceBlock->uniform(Material::SpriteSizeUniformName)->setFloatValue(TileSet::DefaultTileSize, TileSet::DefaultTileSize);
				instanceBlock->uniform(Material::ColorUniformName)->setFloatVector(Colorf(1.0f, 1.0f, 1.0f, 1.0f).Data());

				Matrix4x4f worldMatrix = Matrix4x4f::Translation(std::floor(x * TileSet::DefaultTileSize + (TileSet::DefaultTileSize / 2)), std::floor(y * TileSet::DefaultTileSize + (TileSet::DefaultTileSize / 2)), 0.0f);
				command->setTransformation(worldMatrix);
				command->material().setTexture(*_owner->_tileSet->TextureDiffuse);

				renderQueue.addCommand(command);
			}
		}

		if (!isAnimated && _alreadyRendered) {
			// If it's not animated, it can be rendered only once
			auto it = Viewport::chain().begin();
			while (it != Viewport::chain().end()) {
				if (*it == _view.get()) {
					Viewport::chain().erase(it);
					break;
				}
				++it;
			}
		}

		_alreadyRendered = true;
		return true;
	}
}