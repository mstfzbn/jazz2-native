﻿#include "Uterus.h"
#include "../../../ILevelHandler.h"
#include "../Crab.h"
#include "../../Player.h"
#include "../../Explosion.h"
#include "../../Weapons/ShotBase.h"

#include "../../../../nCine/Base/Random.h"

#include <float.h>

namespace Jazz2::Actors::Bosses
{
	Uterus::Uterus()
		:
		_state(StateWaiting),
		_stateTime(0.0f),
		_endText(0),
		_anglePhase(0.0f),
		_hasShield(false)
	{
	}

	Uterus::~Uterus()
	{
		for (int i = 0; i < _countof(_shields); i++) {
			if (_shields[i] != nullptr) {
				_shields[i]->DecreaseHealth(INT32_MAX);
				_shields[i] = nullptr;
			}
		}
	}

	void Uterus::Preload(const ActorActivationDetails& details)
	{
		PreloadMetadataAsync("Boss/Uterus"_s);
		PreloadMetadataAsync("Enemy/Crab"_s);
	}

	Task<bool> Uterus::OnActivatedAsync(const ActorActivationDetails& details)
	{
		_endText = details.Params[1];
		_originPos = _pos;
		_lastPos = _pos;

		SetState(ActorState::SkipPerPixelCollisions | ActorState::IsInvulnerable, true);
		SetState(ActorState::CanBeFrozen | ActorState::ApplyGravitation, false);

		_scoreValue = 3000;

		co_await RequestMetadataAsync("Boss/Uterus"_s);
		SetAnimation(AnimState::Idle);

		_renderer.setDrawEnabled(false);

		co_return true;
	}

	bool Uterus::OnActivatedBoss()
	{
		SetHealthByDifficulty(50);
		_renderer.setDrawEnabled(true);

		MoveInstantly(_originPos, MoveType::Absolute | MoveType::Force);

		_state = StateOpen;
		_stateTime = 240.0f;

		_spawnCrabTime = 500.0f;

		_hasShield = true;

		for (int i = 0; i < _countof(_shields); i++) {
			_shields[i] = std::make_shared<ShieldPart>();
			_shields[i]->Phase = (fTwoPi * i / _countof(_shields));
			_shields[i]->OnActivated({
				.LevelHandler = _levelHandler,
				.Pos = Vector3i((int)_pos.X, (int)_pos.Y, _renderer.layer() + 2)
			});
			_levelHandler->AddActor(_shields[i]);
		}
		return true;
	}

	bool Uterus::OnPlayerDied()
	{
		_renderer.setDrawEnabled(false);
		_state = StateWaiting;

		for (int i = 0; i < _countof(_shields); i++) {
			if (_shields[i] != nullptr) {
				_shields[i]->DecreaseHealth(INT32_MAX);
				_shields[i] = nullptr;
			}
		}

		return true;
	}

	void Uterus::OnUpdate(float timeMult)
	{
		OnUpdateHitbox();
		HandleBlinking(timeMult);

		switch (_state) {
			case StateOpen: {
				if (_stateTime <= 0.0f) {
					PlaySfx("Closing"_s);

					_state = StateTransition;
					SetAnimation((AnimState)1073741825);
					SetTransition((AnimState)1073741824, false, [this]() {
						_state = StateClosed;
						_stateTime = 280.0f;

						SetState(ActorState::IsInvulnerable, true);
					});
				}

				if (_spawnCrabTime <= 0.0f) {
					float force = Random().NextFloat(-15.0f, 15.0f);

					// TODO: Implement Crab spawn animation
					std::shared_ptr<Enemies::Crab> crab = std::make_shared<Enemies::Crab>();
					crab->OnActivated({
						.LevelHandler = _levelHandler,
						.Pos = Vector3i((int)_pos.X, (int)_pos.Y, _renderer.layer() - 4)
					});
					crab->AddExternalForce(force, 0.0f);
					_levelHandler->AddActor(crab);

					_spawnCrabTime = (_hasShield ? Random().NextFloat(160.0f, 220.0f) : Random().NextFloat(120.0f, 200.0f));
				} else {
					_spawnCrabTime -= timeMult;
				}
				break;
			}

			case StateClosed: {
				if (_stateTime <= 0.0f) {
					PlaySfx("Opening"_s);

					_state = StateTransition;
					SetAnimation(AnimState::Idle);
					SetTransition((AnimState)1073741826, false, [this]() {
						_state = StateOpen;
						_stateTime = 280.0f;

						SetState(ActorState::IsInvulnerable, _hasShield);
					});
				}
				break;
			}
		}

		_stateTime -= timeMult;

		if (_state != StateWaiting) {
			FollowNearestPlayer();

			_anglePhase += timeMult * 0.02f;
			_renderer.setRotation(fPiOver2 + sinf(_anglePhase) * 0.2f);

			Vector2f pos = _lastPos + Vector2f(cosf(_anglePhase) * 60.0f, sinf(_anglePhase) * 60.0f);
			MoveInstantly(pos, MoveType::Absolute | MoveType::Force);

			if (_hasShield) {
				int shieldCount = 0;
				for (int i = 0; i < _countof(_shields); i++) {
					if (_shields[i] != nullptr && _shields[i]->GetHealth() > 0) {
						if (_shields[i]->FallTime <= 0.0f) {
							_shields[i]->MoveInstantly(pos + Vector2f(cosf(_anglePhase + _shields[i]->Phase) * 50.0f, sinf(_anglePhase + _shields[i]->Phase) * 50.0f), MoveType::Absolute | MoveType::Force);
							_shields[i]->Recover(_anglePhase + _shields[i]->Phase);
						}
						shieldCount++;
					}
				}

				if (shieldCount == 0) {
					_hasShield = false;
					SetState(ActorState::IsInvulnerable, _state != StateOpen);
				}
			}
		}

		OnUpdateHitbox();
	}

	void Uterus::OnUpdateHitbox()
	{
		UpdateHitbox(38, 60);
	}

	bool Uterus::OnPerish(ActorBase* collider)
	{
		Explosion::Create(_levelHandler, Vector3i((int)_pos.X, (int)_pos.Y, _renderer.layer() + 2), Explosion::Type::SmokePoof);
		Explosion::Create(_levelHandler, Vector3i((int)_pos.X, (int)_pos.Y, _renderer.layer() - 2), Explosion::Type::RF);

		CreateDeathDebris(collider);
		_levelHandler->PlayCommonSfx("Splat"_s, Vector3f(_pos.X, _pos.Y, 0.0f));

		StringView text = _levelHandler->GetLevelText(_endText, -1, '|');
		_levelHandler->ShowLevelText(text);

		return BossBase::OnPerish(collider);
	}

	void Uterus::FollowNearestPlayer()
	{
		bool found = false;
		Vector2f targetPos = Vector2f(FLT_MAX, FLT_MAX);

		auto& players = _levelHandler->GetPlayers();
		for (auto& player : players) {
			Vector2f newPos = player->GetPos();
			if ((_pos - newPos).Length() < (_pos - targetPos).Length()) {
				targetPos = newPos;
				found = true;
			}
		}

		if (found) {
			targetPos.Y -= 100.0f;

			Vector2f diff = (targetPos - _lastPos).Normalized();
			// TODO: There is something strange (speedX == speedY == 0)...
			Vector2f speed = (Vector2f(_speed.X, _speed.Y) + diff * 0.4f).Normalized();

			float mult = (_hasShield ? 0.8f : 2.0f);
			_lastPos.X += speed.X * mult;
			_lastPos.Y += speed.Y * mult;
		}
	}

	Task<bool> Uterus::ShieldPart::OnActivatedAsync(const ActorActivationDetails& details)
	{
		SetState(ActorState::CollideWithTileset | ActorState::CollideWithSolidObjects | ActorState::CanBeFrozen | ActorState::ApplyGravitation, false);
		CanCollideWithAmmo = false;

		_elasticity = 0.3f;

		_health = 3;

		co_await RequestMetadataAsync("Boss/Uterus"_s);
		SetAnimation((AnimState)1073741827);

		co_return true;
	}

	void Uterus::ShieldPart::OnUpdate(float timeMult)
	{
		if (FallTime > 0.0f) {
			EnemyBase::OnUpdate(timeMult);
			FallTime -= timeMult;
		} else {
			HandleBlinking(timeMult);
		}
	}

	void Uterus::ShieldPart::OnUpdateHitbox()
	{
		UpdateHitbox(6, 6);
	}

	bool Uterus::ShieldPart::OnHandleCollision(std::shared_ptr<ActorBase> other)
	{
		if (auto shotBase = dynamic_cast<Weapons::ShotBase*>(other.get())) {
			DecreaseHealth(shotBase->GetStrength(), shotBase);

			FallTime = 400.0f;

			SetState(ActorState::CollideWithTileset | ActorState::CollideWithSolidObjects | ActorState::ApplyGravitation, true);
			return true;
		}

		return EnemyBase::OnHandleCollision(other);
	}

	void Uterus::ShieldPart::Recover(float phase)
	{
		SetState(ActorState::CollideWithTileset | ActorState::CollideWithSolidObjects | ActorState::ApplyGravitation, false);

		_renderer.setRotation(phase);
	}
}