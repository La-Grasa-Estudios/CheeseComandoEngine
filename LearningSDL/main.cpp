#define SDL_MAIN_HANDLED
#include <SDL.h> 
#include <SDL_image.h>
#include "core\Coordinator.hpp"
#include "systems\TransformSystem.hpp"
#include "systems\RenderSystem.hpp"
#include "systems\AnimationSystem.hpp"
#include "components\TransformComponent.hpp"
#include "components\RenderableComponent.hpp"
#include "components\AnimationComponent.hpp"
#include "tools\generic_tools.hpp"

#include <vector>

Coordinator gCoordinator;

//CORRECT ANIMATIONSYSTEM

void CreateEntity(std::vector<SDL_Rect> Frames)
{
    Entity entity = gCoordinator.CreateEntity();

    gCoordinator.AddComponent<Transform>(entity,
        { .position = Vec2{static_cast<float>(generic_tools::random_number(0,  1500)), static_cast<float>(generic_tools::random_number(0, 800)) }, .scale = Vec2{1.0f,1.0f} });

    gCoordinator.AddComponent<Animation>(entity,
        {
         .frames = Frames,
         .frameRate = 1.0f / 12.0f,
         .loop = true,
         .isPlaying = true
        });


    gCoordinator.AddComponent<Renderable>
        (entity,
            {
            .texture_path = "images/test.png",
            .srcRect = Frames[0],
            .color_mod = { 255, 255, 255, 255 },
            .angle = 0.0f,
            .flip = SDL_FLIP_NONE,
            .origin = { 0, 0 },
            .renderLayer = 1,
            .isVisible = true
            }
        );

}

int main()
{
    int ENTTQUANTITY;
    std::cout << "\nPuedes hacer click para crear una entidad\n";
    std::cout << "Selecciona la cantidad de Entidades que deseas renderizar (cap 99999): ";
    std::cin >> ENTTQUANTITY;

    srand(time(0));

    gCoordinator.RegisterComponent<Transform>();
    gCoordinator.RegisterComponent<Renderable>();
    gCoordinator.RegisterComponent<Animation>();

    auto TranformSystem = gCoordinator.RegisterSystem<TransformSystem>();
    {
        Signature transformSignature;
        transformSignature.set(gCoordinator.GetComponentType<Transform>());
        gCoordinator.SetSystemSignature<TransformSystem>(transformSignature);
    }

    auto animationSystem = gCoordinator.RegisterSystem<AnimationSystem>();
    {
        Signature animationSignature;
        animationSignature.set(gCoordinator.GetComponentType<Animation>());
        animationSignature.set(gCoordinator.GetComponentType<Renderable>());        
        gCoordinator.SetSystemSignature<AnimationSystem>(animationSignature);
    }

    auto renderSystem = gCoordinator.RegisterSystem<RenderSystem>();
    {
        Signature renderSignature;
        renderSignature.set(gCoordinator.GetComponentType<Transform>());
        renderSignature.set(gCoordinator.GetComponentType<Renderable>());
        gCoordinator.SetSystemSignature<RenderSystem>(renderSignature);
    }

    SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("ECS test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_SHOWN);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	
    if (IMG_Init(IMG_INIT_PNG) == -1)
    {
        std::cout << "\nImage System could not load\n";
    }
    else
    {
        std::cout << "\nImage System ready to go\n";
    }
    std::vector<SDL_Rect> Frames;

    for (int i = 0; i < 12; ++i) 
    {
        SDL_Rect frame;
        frame.x = 0 + (128 * i);
        frame.y = 0;
        frame.w = 128;
        frame.h = 128;

        Frames.push_back(frame);
    }
    int change = 0;
    for (int i = 0; i < ENTTQUANTITY; ++i)
    {
        CreateEntity(Frames);

    }

    bool run = true;
    SDL_Event event;
    Uint32 previous_time = SDL_GetTicks();
    float deltaTime = 0.0f;

    while (run)
    {
        Uint32 current_time = SDL_GetTicks();
        deltaTime = (current_time - previous_time) / 1000.0f;
        previous_time = current_time;
        

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                run = false;
            }

            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (event.type == SDL_MOUSEBUTTONDOWN) 
                {
                    CreateEntity(Frames);
                }
            }
        }
        SDL_RenderClear(renderer);
        TranformSystem->Update(deltaTime);
        animationSystem->Update(deltaTime);
        renderSystem->Update(renderer);
        SDL_RenderPresent(renderer);
    }

}