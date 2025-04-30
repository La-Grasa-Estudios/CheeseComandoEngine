#include "AnimationSystem.hpp"
#include "..\components\RenderableComponent.hpp"
#include "..\components\AnimationComponent.hpp"
#include "..\core\Coordinator.hpp"
extern Coordinator gCoordinator;
void AnimationSystem::Init() 
{

}

void AnimationSystem::Update(float deltaTime) {
    for (Entity entity : mEntities) {
        auto& renderable = gCoordinator.GetComponent<Renderable>(entity);
        auto& animation = gCoordinator.GetComponent<Animation>(entity);

        if (!animation.isPlaying) continue;

        animation.currentFrameTime += deltaTime;

        if (animation.currentFrameTime >= animation.frameRate) {
            animation.currentFrameTime = 0.0f;
            animation.currentFrame++;


            if (animation.currentFrame >= animation.frames.size()) {
                if (animation.loop) {
                    animation.currentFrame = 0;
                }
                else {
                    animation.isPlaying = false;
                    animation.currentFrame = animation.frames.size() - 1;
                }
            }


            renderable.srcRect = animation.frames[animation.currentFrame];
        }
    }
}

