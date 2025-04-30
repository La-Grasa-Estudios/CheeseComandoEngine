#include "..\systems\TransformSystem.hpp"
#include "..\components\TransformComponent.hpp"
#include "..\core\Coordinator.hpp"

extern Coordinator gCoordinator;

void TransformSystem::Init()
{

}

void TransformSystem::Update(float deltaTime)
{
	for (auto const& entity : mEntities) 
	{
		auto& transform = gCoordinator.GetComponent<Transform>(entity);

		transform.position.x += (180.0f * deltaTime);
	}
}