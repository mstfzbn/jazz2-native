﻿#include "Turtle.h"
#include "../../LevelInitialization.h"
#include "../../ILevelHandler.h"
#include "../../Tiles/TileMap.h"
#include "TurtleShell.h"
#include "../Explosion.h"
#include "../Player.h"
#include "../Weapons/Thunderbolt.h"
#include "../Weapons/ToasterShot.h"

#include "../../../nCine/Base/Random.h"

namespace Jazz2::Actors::Enemies
{
	Turtle::Turtle()
		:
		_isTurning(false),
		_isWithdrawn(false),
		_isAttacking(false)
	{
	}

	void Turtle::Preload(const ActorActivationDetails& details)
	{
		uint8_t theme = details.Params[0];
		switch (theme) {
			case 0:
			default:
				PreloadMetadataAsync("Enemy/Turtle"_s);
				PreloadMetadataAsync("Enemy/TurtleShell"_s);
				break;
			case 1: // Xmas
				PreloadMetadataAsync("Enemy/TurtleXmas"_s);
				PreloadMetadataAsync("Enemy/TurtleShellXmas"_s);
				break;
		}
	}

	Task<bool> Turtle::OnActivatedAsync(const ActorActivationDetails& details)
	{
		SetHealthByDifficulty(1);
		_scoreValue = 100;

		SetState(ActorState::CollideWithTilesetReduced, true);

		_theme = details.Params[0];
		switch (_theme) {
			case 0:
			default:
				co_await RequestMetadataAsync("Enemy/Turtle"_s);
				break;
			case 1: // Xmas
				co_await RequestMetadataAsync("Enemy/TurtleXmas"_s);
				break;
		}
		SetFacingLeft(nCine::Random().NextBool());
		SetAnimation(AnimState::Walk);

		_speed.X = (IsFacingLeft() ? -1 : 1) * DefaultSpeed;

		PlaceOnGround();

		co_return true;
	}

	void Turtle::OnUpdate(float timeMult)
	{
		EnemyBase::OnUpdate(timeMult);

		if (_frozenTimeLeft > 0.0f) {
			return;
		}

		if (GetState(ActorState::CanJump)) {
			if (std::abs(_speed.X) > 0.0f && !CanMoveToPosition(_speed.X * 4, 0)) {
				SetTransition(AnimState::TransitionWithdraw, false, [this]() {
					HandleTurn(true);
				});
				_isTurning = true;
				_canHurtPlayer = false;
				_speed.X = 0;
				PlaySfx("Withdraw"_s, 0.2f);
			}
		}

		if (_isAttacking) {
			// Turtles attack only with animation, so check collisions every frame
			SetState(ActorState::IsDirty, true);
		} else if (!_isTurning && !_isWithdrawn) {
			AABBf aabb = AABBInner + Vector2f(_speed.X * 32, 0);
			TileCollisionParams params = { TileDestructType::None, false };
			if (_levelHandler->TileMap()->IsTileEmpty(aabb, params)) {
				_levelHandler->GetCollidingPlayers(aabb + Vector2f(_speed.X * 32, 0), [this](ActorBase* player) -> bool {
					if (!player->IsInvulnerable()) {
						Attack();
						return false;
					}
					return true;
				});
			}
		}
	}

	void Turtle::OnUpdateHitbox()
	{
		UpdateHitbox(24, 24);
	}

	bool Turtle::OnPerish(ActorBase* collider)
	{
		// Animation should be paused only if enemy is frozen
		bool shouldDestroy = _renderer.AnimPaused;
		if (auto player = dynamic_cast<Player*>(collider)) {
			if (player->GetSpecialMove() != Player::SpecialMoveType::None) {
				shouldDestroy = true;
			}
		} else if (auto toasterShot = dynamic_cast<Weapons::ToasterShot*>(collider)) {
			shouldDestroy = true;
		} else if (auto thunderbolt = dynamic_cast<Weapons::Thunderbolt*>(collider)) {
			shouldDestroy = true;
		}

		if (shouldDestroy) {
			CreateDeathDebris(collider);
			_levelHandler->PlayCommonSfx("Splat"_s, Vector3f(_pos.X, _pos.Y, 0.0f));

			// Add score also for turtle shell
			_scoreValue += 100;

			TryGenerateRandomDrop();
		} else {
			std::shared_ptr<TurtleShell> shell = std::make_shared<TurtleShell>();
			uint8_t shellParams[9];
			*(float*)&shellParams[0] = _speed.X * 1.1f;
			*(float*)&shellParams[4] = (_pos.Y < _levelHandler->WaterLevel() ? -1.1f : -0.65f);
			shellParams[8] = _theme;
			shell->OnActivated({
				.LevelHandler = _levelHandler,
				.Pos = Vector3i((int)_pos.X, (int)_pos.Y, _renderer.layer()),
				.Params = shellParams
			});
			_levelHandler->AddActor(shell);

			Explosion::Create(_levelHandler, Vector3i((int)_pos.X, (int)_pos.Y, _renderer.layer() - 2), Explosion::Type::SmokeGray);
		}

		return EnemyBase::OnPerish(collider);
	}

	void Turtle::HandleTurn(bool isFirstPhase)
	{
		if (_isTurning) {
			if (isFirstPhase) {
				SetFacingLeft(!IsFacingLeft());
				SetTransition(AnimState::TransitionWithdrawEnd, false, [this]() {
				   HandleTurn(false);
				});
				PlaySfx("WithdrawEnd"_s, 0.2f);
				_isWithdrawn = true;
			} else {
				_canHurtPlayer = true;
				_isWithdrawn = false;
				_isTurning = false;
				_speed.X = (IsFacingLeft() ? -1 : 1) * DefaultSpeed;
			}
		}
	}

	void Turtle::Attack()
	{
		_speed.X = 0;
		_isAttacking = true;
		PlaySfx("Attack"_s);

		SetTransition(AnimState::TransitionAttack, false, [this]() {
			_speed.X = (IsFacingLeft() ? -1 : 1) * DefaultSpeed;
			_isAttacking = false;

			// TODO: Bad timing
			PlaySfx("Attack2"_s);
		});
	}
}