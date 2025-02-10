#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 800
#define HEIGHT 600
#define ITERATE_WITH_SPACE 1

typedef struct
{
    char *front;
    char *back;
} Card;

Card *cards = NULL;
size_t total_cards = 0;
int current_card = 0;
int flipped = 0;

void freeCards()
{
    for (size_t i = 0; i < total_cards; i++)
    {
        free(cards[i].front);
        free(cards[i].back);
    }
    free(cards);
    printf("Freed all cards!\n");
}

void load_cards(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Couldn't open file %s\n", filename);
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, file) != -1)
    {
        // ignore commented lines
        if (line[0] == '#')
            continue;

        // separate using the tab
        char *tab = strchr(line, '\t');
        if (!tab)
            continue;
        *tab = '\0';

        // strdup copies using malloc the passed string
        char *front = strdup(line);
        char *back = strdup(tab + 1);

        // replace newline with null-terminator
        char *nl = strchr(back, '\n');
        if (nl)
            *nl = '\0';

        cards = realloc(cards, sizeof(Card) * (total_cards + 1));
        cards[total_cards].front = front;
        cards[total_cards].back = back;
        total_cards++;
    }

    free(line);
    fclose(file);
}

void render_text(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *small_f, SDL_Color text_color, SDL_Color bg_color, int card_num, const char *text, int x, int y)
{
    // card text;
    SDL_Surface *surface = TTF_RenderUTF8_LCD_Wrapped(font, text, text_color, bg_color, WIDTH - 100);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dst = {
        x - surface->w / 2,
        y - surface->h / 2,
        surface->w,
        surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    int num_x = WIDTH / 2;
    int num_y = HEIGHT - 30;
    char num[16];
    snprintf(num, sizeof(num), "%d/%d", current_card + 1, total_cards);

    SDL_Surface *card_num_s = TTF_RenderUTF8_LCD_Wrapped(small_f, num, text_color, bg_color, 0);
    SDL_Texture *card_num_t = SDL_CreateTextureFromSurface(renderer, card_num_s);

    SDL_Rect drect = {
        num_x - card_num_s->w / 2,
        num_y - card_num_s->h / 2,
        card_num_s->w,
        card_num_s->h};

    // blend with surface;
    SDL_RenderCopy(renderer, card_num_t, NULL, &drect);
    SDL_FreeSurface(card_num_s);
    SDL_DestroyTexture(card_num_t);
}

void storeLastSeenCard()
{
    FILE *file = fopen(".cache", "w");
    if (!file)
    {
        fprintf(stderr, "Couldn't open file last_card.txt\n");
        exit(1);
    }
    fprintf(file, "%d", current_card);
    fclose(file);
}

void loadLastSeenCard()
{
    FILE *file = fopen(".cache", "r");
    // if file does not exist, just return
    if (!file)
        return;

    // otherwise we start again from that card.
    fscanf(file, "%d", &current_card);
    fclose(file);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <flashcard-file>\n", argv[0]);
        return 1;
    }

    load_cards(argv[1]);
    for (size_t i = 0; i < 2; i++)
    {
        printf("%s\n", cards[i].front);
        printf("%s\n", cards[i].back);
        puts("");
    }

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("Flashcards",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
                                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    TTF_Font *font = TTF_OpenFont("JetBrainsMono-Medium.ttf", HEIGHT / 15);
    if (!font)
    {
        fprintf(stderr, "Error loading font: %s\n", TTF_GetError());
        exit(1);
    }
    TTF_Font *small_font = TTF_OpenFont("JetBrainsMono-Medium.ttf", 20);

    int running = 1;
    loadLastSeenCard();
    SDL_Event event;
    int pressed = 0;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = 0;

            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_SPACE:
                    flipped = !flipped;
                    if (ITERATE_WITH_SPACE)
                        if (pressed && current_card < total_cards - 1)
                        {
                            current_card++;
                            pressed = 0;
                        }
                        else
                            pressed = 1;
                    break;
                case SDLK_LEFT:
                    if (current_card > 0)
                        current_card--;
                    flipped = 0;
                    break;
                case SDLK_RIGHT:
                    if (current_card < total_cards - 1)
                        current_card++;
                    flipped = 0;
                    break;
                case SDLK_ESCAPE:
                    running = 0;
                    break;
                }
            }
        }

        const char *text = flipped ? cards[current_card].back : cards[current_card].front;

        SDL_Color bg = flipped ? (SDL_Color){0, 0, 0, 255} : (SDL_Color){255, 255, 255, 255};
        SDL_Color text_color = flipped ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){0, 0, 0, 255};

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
        SDL_RenderClear(renderer);
        render_text(renderer, font, small_font, text_color, bg, current_card, text, WIDTH / 2, HEIGHT / 2);

        SDL_RenderPresent(renderer);
        SDL_Delay(128); // increase delay ~7-8 fps, more efficient
    }

    // cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_CloseFont(small_font);
    SDL_Quit();
    freeCards();
    storeLastSeenCard();
}
