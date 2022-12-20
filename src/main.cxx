#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#include <iostream>
#include <list>
#include <random>
#include <string>
#include <algorithm>
#include <vector>
#include <numeric>


struct SnakeTile
{
    int x;
    int y;
    int type;
};

inline bool operator==(SDL_Point lhs, SDL_Point rhs)
{
    return rhs.x == lhs.x && rhs.y == lhs.y;
}

inline bool operator==(const SnakeTile& lhs, SDL_Point rhs)
{
    return rhs.x == lhs.x && rhs.y == lhs.y;
}

inline bool operator==(const SnakeTile& lhs, const SnakeTile& rhs)
{
    return rhs.x == lhs.x && rhs.y == lhs.y;
}

int getSpeedOrient(SDL_Point speed)
{
    if (speed.x > 1)
        speed.x -= 30;
    if (speed.x < - 1)
        speed.x += 30;

    if (speed.y > 1)
        speed.y -= 30;
    if (speed.y < - 1)
        speed.y += 30;

    if (speed == SDL_Point{1, 0})
        return 0;
    if (speed == SDL_Point{0, 1})
        return 1;
    if (speed == SDL_Point{- 1, 0})
        return 2;

    return 3;
}

void autorunStep(const std::vector<SnakeTile>& snake, SDL_Point speed, SDL_Point apple)
{
    SDL_Event event{};
    event.type = SDL_KEYDOWN;

    SDL_Point next = {snake[0].x+speed.x, snake[0].y+speed.y};
    if (std::find(snake.begin(), snake.end(), next) != snake.end())
    {
        SDL_Point speeds[] = {
            {1, 0},
            {0, 1},
            {- 1, 0},
            {0, - 1}
        };
        for (auto speedNew : speeds)
        {
            next = {snake[0].x+speedNew.x, snake[0].y+speedNew.y};
            if (std::find(snake.begin(), snake.end(), next) == snake.end())
            {
                speed = speedNew;
                break;
            }
        }
    }

    SDL_Keycode syms[] = {
        SDLK_RIGHT,
        SDLK_UP,
        SDLK_LEFT,
        SDLK_DOWN
    };

    event.key.keysym.sym = syms[getSpeedOrient(speed)];
    SDL_PushEvent(&event);
}

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, - 1, SDL_RENDERER_SOFTWARE);
    Mix_Chunk* eat = Mix_LoadWAV("../sound/eat.wav");
    Mix_Music* music = Mix_LoadMUS("../music/doom.wav");

    TTF_Font* font = TTF_OpenFont("../font/8bitOperatorPlus.ttf", 32);
    SDL_Surface* tempSurface = IMG_Load("../sprites/snake.png");
    SDL_Texture* snakeTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    tempSurface = IMG_Load("../sprites/apple.png");
    SDL_Texture* appleTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);

    SDL_Rect spriteClips[] = {
        {1, 1, 20, 20},
        {22, 1, 20, 20},
        {22, 1, 20, 20},
        {1, 22, 20, 20},
        {22, 22, 20, 20},
    };

    std::vector<SnakeTile> snake = {};
    snake.reserve(900);

    snake.push_back({15, 15, 0});
    snake.push_back({14, 15, 0});
    snake.push_back({13, 15, 0});

    SDL_Point speed = {1, 0};

    auto getHeadNextType = [&snake]()
    {
        int headOrient = snake[0].type % 4;
        int tailOrient = getSpeedOrient({snake[1].x - snake[2].x, snake[1].y - snake[2].y});

        if (headOrient == tailOrient)
            return tailOrient;

        if ((headOrient + 4 - tailOrient) % 4 == 3)
            return 8 + (tailOrient + 1) % 4;

        return 4 + (tailOrient + 1) % 4;
    };
    auto getTailPrevType = [&snake]()
    {
        SnakeTile tail = snake.back();
        SnakeTile tailPrev = *(snake.rbegin() + 1);
        return 12 + getSpeedOrient({tailPrev.x - tail.x, tailPrev.y - tail.y});
    };

    std::vector<SDL_Point> tiles;
    tiles.reserve(900);

    std::random_device rd;
    std::uniform_int_distribution<int> smallDistrib(0, 29);
    std::uniform_int_distribution<int> bigDistrib;

    SDL_Point apple;
    auto generateApple = [&]()
    {
        apple = {smallDistrib(rd), smallDistrib(rd)};
        if (std::find(snake.begin(), snake.end(), apple) != snake.end())
        {
            tiles.resize(900);
            for (int i = 0; i < 30; ++i)
                for (int j = 0; j < 30; ++j)
                    tiles[i * 30 + j] = {j, i};
            for (const auto& tail : snake)
                tiles.erase(tiles.begin() + 30*tail.y + tail.x);

            bigDistrib = std::uniform_int_distribution<int>(0, tiles.size());
            apple = tiles[bigDistrib(rd)];
        }
    };
    generateApple();

    int score = 0;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Press any key to start", {128, 128, 128, 255});
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {0, 200, 600, (600 * textSurface->h) / textSurface->w};
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    SDL_RenderPresent(renderer);
    SDL_Event event;
    bool autorun = false;
    while (event.type != SDL_KEYDOWN)
        SDL_WaitEvent(&event);

    if (event.key.keysym.sym == SDLK_a)
        autorun = true;

    //Mix_PlayMusic(music, - 1);

    bool run = true;
    while (run)
    {
        if (autorun)
            autorunStep(snake, speed, apple);

        SDL_Event event{};
        SDL_Point speedNew = speed;
        while (SDL_PollEvent(&event))
        {
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                run = false;
                break;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_RIGHT:
                    if (speed.x != - 1)
                        speedNew = {1, 0};
                    break;
                case SDLK_UP:
                    if (speed.y != 1)
                        speedNew = {0, - 1};
                    break;
                case SDLK_LEFT:
                    if (speed.x != 1)
                        speedNew = {- 1, 0};
                    break;
                case SDLK_DOWN:
                    if (speed.y != - 1)
                        speedNew = {0, 1};
                    break;
                }
            }
        }
        speed = speedNew;

        SnakeTile head = {
            snake.front().x + speed.x,
            snake.front().y + speed.y,
            16 + getSpeedOrient(speed)
        };

        if (head.x < 0)
            head.x += 30;

        if (head.x >= 30)
            head.x -= 30;

        if (head.y < 0)
            head.y += 30;

        if (head.y >= 30)
            head.y -= 30;

        snake.insert(snake.begin(), head);
        snake[1].type = getHeadNextType();

        if (head == apple)
        {
            generateApple();
            Mix_PlayChannel(- 1, eat, 0);
            score++;
        }
        else
            snake.pop_back();

        snake.back().type = getTailPrevType();

        if (std::find(snake.begin() + 1, snake.end(), head) != snake.end())
            run = false;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (const auto& tile : snake)
        {
            SDL_Rect box = {20*tile.x, 20*tile.y, 20, 20};
            SDL_RenderCopyEx(renderer, snakeTexture, &spriteClips[tile.type / 4], &box,
                    90*(tile.type % 4), nullptr, tile.type/4 == 2 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
        }

        SDL_Rect box = {20*apple.x, 20*apple.y, 20, 20};
        SDL_RenderCopy(renderer, appleTexture, &spriteClips[0], &box);

        SDL_RenderPresent(renderer);
        SDL_Delay(80);
    }

    textSurface = TTF_RenderText_Solid(font, "GAME OVER", {255, 0, 0, 128});
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {0, 200, 600, (600 * textSurface->h) / textSurface->w};
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    textSurface = TTF_RenderText_Solid(font, ("Your score: " + std::to_string(score)).c_str(), {255, 0, 0, 128});
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {300 - textSurface->w / 2, 300, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    SDL_RenderPresent(renderer);
    while (event.window.event != SDL_WINDOWEVENT_CLOSE)
        SDL_WaitEvent(&event);

    SDL_DestroyTexture(appleTexture);
    SDL_DestroyTexture(snakeTexture);
    TTF_CloseFont(font);
    Mix_FreeChunk(eat);
    Mix_FreeMusic(music);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    TTF_Quit();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    return 0;
}
