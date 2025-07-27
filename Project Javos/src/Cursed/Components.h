#pragma once

#include <Entity/ECS.h>

namespace Funkin
{
	static inline const char* C_PLAY_CARD_COMPONENT = "PlayingCardComponent";
	static inline const char* C_CARD_COMPONENT = "CardComponent";
	static inline const char* C_TILT_COMPONENT = "TextTiltComponent";

	const uint32_t BaseCardChips[] =
	{
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		9,
		10,
		10,
		10,
		10,
		11
	};

	const uint32_t CardTypeSordWeight[]
	{
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		9,
		10,
		11,
		12,
		13,
		14
	};

	enum CardSuit : uint8_t
	{
		CARD_SUIT_HEARTS,
		CARD_SUIT_CLUBS,
		CARD_SUIT_DIAMONDS,
		CARD_SUIT_SPADES
	};

	enum CardType : uint8_t
	{
		CARD_TYPE_TWO,
		CARD_TYPE_THREE,
		CARD_TYPE_FOUR,
		CARD_TYPE_FIVE,
		CARD_TYPE_SIX,
		CARD_TYPE_SEVEN,
		CARD_TYPE_EIGHT,
		CARD_TYPE_NINE,
		CARD_TYPE_TEN,
		CARD_TYPE_JACK,
		CARD_TYPE_QUEEN,
		CARD_TYPE_KING,
		CARD_TYPE_ACE,
		CARD_TYPE_INVALID,
	};

	struct TextTiltComponent
	{
		int seed = 0;
		bool credits = true;
	};

	struct CardComponent
	{
		float tiltX = 0.0f;
		float tiltY = 0.0f;
		float tiltFactor = 10.0f;
		float rotation = 0.0f;
		float userRotation = 0.0f;
		float moveSpeed = 4.0f;
		float grabTimer = 0.0f;
		float scaleFactor = 1.0f;
		bool grabbable = true;
		bool grabbed = false;
		bool isHovered = false;
		float grabbedTimer = 0;
		int seed = 0;
		glm::vec3 position = {};
		Stratum::ECS::edict_t bgEntity;
		Stratum::ECS::edict_t bgShadowEntity;
		uint32_t renderLayer = 0;
	};

	struct PlayingCardComponent
	{
		uint32_t chips = 11;
		CardType type = CARD_TYPE_ACE;
		CardSuit suit = CARD_SUIT_HEARTS;
		uint8_t cardIndex = 0;
		bool selected = false;
		bool played = false;
	};
}