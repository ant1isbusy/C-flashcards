#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <dirent.h>

#define WIDTH 800
#define HEIGHT 600
#define ITERATE_WITH_SPACE 1
#define COLOR_BLACK {0, 0, 0, 255}
#define COLOR_WHITE {255, 255, 255, 255}
#define COLOR_RED {255, 0, 0, 255}

typedef struct
{
    char *front;
    char *back;
} Card;

typedef struct _File_
{
    char *name;
    struct _File_ *next;
} File;

typedef struct
{
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Event event;
    TTF_Font *font;
    TTF_Font *small_font;

    struct
    {
        int current;
        int total;
        Card *items;
    } cards;

    char pressed_once;

    struct
    {
        File *selected;
        File *all_files;
        int total_files;
    } decks;

    char running;
    char flipped;
} AppState;

void freeCards(AppState *state)
{
    Card *cards = state->cards.items;
    for (size_t i = 0; i < state->cards.total; i++)
    {
        free(cards[i].front);
        free(cards[i].back);
    }
    free(cards);
    printf("Freed all cards!\n");
}

void addtoLinkedList(File **head, File *new_file)
{
    if (*head == NULL)
    {
        *head = new_file;
        return;
    }
    File *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = new_file;
}

void getFlashCardFiles(File **head, AppState *state)
{
    struct dirent *entry;
    DIR *dir = opendir(".");

    if (dir == NULL)
    {
        fprintf(stderr, "Error opening directory\n");
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // locate substring;
        if (strstr(entry->d_name, ".txt") != NULL)
        {
            File *new_file = (File *)malloc(sizeof(File));
            new_file->name = strdup(entry->d_name);
            new_file->next = NULL;
            state->decks.total_files++;
            addtoLinkedList(head, new_file);
        }
    }
}

void printFlashCardFiles(File *head)
{
    File *current = head;
    while (current != NULL)
    {
        printf("%s\n", current->name);
        current = current->next;
    }
}

void load_cards(const char *filename, AppState *state)
{
    int total_cards = 0;
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

        state->cards.items = realloc(state->cards.items, sizeof(Card) * (total_cards + 1));
        state->cards.items[total_cards].front = front;
        state->cards.items[total_cards].back = back;
        total_cards++;
    }
    state->cards.total = total_cards;
    free(line);
    fclose(file);
}

void render_text(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *small_f, SDL_Color text_color, SDL_Color bg_color, const char *text, int x, int y, AppState *state)
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
    snprintf(num, sizeof(num), "%ld/%ld", state->cards.current + 1, state->cards.total);

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

/* void storeLastSeenCard()
{
    FILE *file = fopen(".cache", "w");
    if (!file) // should never happen
    {
        fprintf(stderr, "Couldn't open file last_card.txt\n");
        exit(1);
    }
    fprintf(file, "%ld", current_card);
    fclose(file);
}

void loadLastSeenCard()
{
    FILE *file = fopen(".cache", "r");
    // if file does not exist, just return
    if (!file)
        return;

    // otherwise we start again from that card.
    fscanf(file, "%ld", &current_card);
    fclose(file);
} */

File *selectionMenu(AppState *state)
{
    if (state->decks.all_files == NULL)
    {
        fprintf(stderr, "No flashcard files found!\n");
        // TODO: replace with window output
        exit(1);
    }

    int selected_index = 0;
    int total_files = 0;

    File *current = state->decks.all_files;
    while (current)
    {
        total_files++;
        current = current->next;
    }

    const int line_height = TTF_FontHeight(state->font) + 5;
    const SDL_Color WHITE = COLOR_WHITE;
    const SDL_Color BLACK = COLOR_BLACK;
    const SDL_Color RED = COLOR_RED;

    char selected_deck = 0;
    SDL_Event event;
    while (!selected_deck)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                return NULL;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_UP:
                    selected_index = (selected_index - 1 + total_files) % total_files;
                    break;
                case SDLK_DOWN:
                    selected_index = (selected_index + 1) % total_files;
                    break;
                case SDLK_RETURN:
                    selected_deck = 1;
                    break;
                case SDLK_ESCAPE:
                    return NULL;
                }
                break;
            }
        }
        SDL_SetRenderDrawColor(state->renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);
        SDL_RenderClear(state->renderer);

        // Render file list
        int count = 0;
        int start_y = (HEIGHT - (total_files * line_height)) / 2; // Center vertically
        current = state->decks.all_files;

        while (current)
        {
            SDL_Color color = (count == selected_index) ? RED : BLACK;

            // Render text
            SDL_Surface *surface = TTF_RenderText_Blended(state->font, current->name, color);
            if (!surface)
                continue;

            SDL_Texture *texture = SDL_CreateTextureFromSurface(state->renderer, surface);
            if (!texture)
            {
                SDL_FreeSurface(surface);
                continue;
            }

            // Center text horizontally
            int text_width, text_height;
            SDL_QueryTexture(texture, NULL, NULL, &text_width, &text_height);
            SDL_Rect dest = {
                (WIDTH - text_width) / 2,
                start_y + (count * line_height),
                text_width,
                text_height};

            SDL_RenderCopy(state->renderer, texture, NULL, &dest);

            // Cleanup
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);

            current = current->next;
            count++;
        }

        SDL_RenderPresent(state->renderer);
        SDL_Delay(32);
    }

    File *selected = state->decks.all_files;
    for (int i = 0; i < selected_index && selected; i++)
    {
        selected = selected->next;
    }
    return selected;
}

void handleEvents(AppState *state)
{
    while (SDL_PollEvent(&state->event))
    {
        if (state->event.type == SDL_QUIT)
            state->running = 0;

        else if (state->event.type == SDL_KEYDOWN)
        {
            switch (state->event.key.keysym.sym)
            {
            case SDLK_SPACE:
                state->flipped = !state->flipped;
                if (ITERATE_WITH_SPACE)
                {
                    if (state->pressed_once && state->cards.current < state->cards.total - 1)
                    {
                        state->cards.current++;
                        state->pressed_once = 0;
                    }
                    else
                        state->pressed_once = 1;
                }
                break;
            case SDLK_LEFT:
                if (state->cards.current > 0)
                    state->cards.current--;
                state->flipped = 0;
                break;
            case SDLK_RIGHT:
                if (state->cards.current < state->cards.total - 1)
                    state->cards.current++;
                state->flipped = 0;
                break;
            case SDLK_ESCAPE:
                state->running = 0;
                break;
            }
        }
    }
}

void init(AppState *state)
{
    state->running = 1;
    state->flipped = 0;
    state->cards.total = 0;
    state->cards.current = 0;
    state->decks.selected = NULL;
    state->cards.items = NULL;

    getFlashCardFiles(&state->decks.all_files, state);
    printFlashCardFiles(state->decks.all_files);

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    state->window = SDL_CreateWindow("Flashcards",
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     WIDTH, HEIGHT, 0);

    state->renderer = SDL_CreateRenderer(state->window, -1,
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    state->font = TTF_OpenFont("JetBrainsMono-Medium.ttf", HEIGHT / 15);
    if (!state->font)
    {
        // TODO: do proper cleanup up previously malloced memory!
        fprintf(stderr, "Error loading font: %s\n", TTF_GetError());
        exit(1);
    }
    state->small_font = TTF_OpenFont("JetBrainsMono-Medium.ttf", 20);

    // loadLastSeenCard(); TODO: see if we want to keep this, how we can adapt to multiple files.
    state->pressed_once = 0;
}

void freeFiles(File *head)
{
    File *current = head;
    while (current)
    {
        File *next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
}

void render(AppState *state)
{
    Card *cards = state->cards.items;
    int current_card = state->cards.current;

    const char *text = state->flipped ? cards[current_card].back : cards[current_card].front;

    SDL_Color bg = state->flipped ? (SDL_Color)COLOR_BLACK : (SDL_Color)COLOR_WHITE;
    SDL_Color text_color = state->flipped ? (SDL_Color)COLOR_WHITE : (SDL_Color)COLOR_BLACK;

    SDL_SetRenderDrawColor(state->renderer, bg.r, bg.g, bg.b, 255);
    SDL_RenderClear(state->renderer);
    render_text(state->renderer, state->font, state->small_font, text_color, bg, text, WIDTH / 2, HEIGHT / 2, state);

    SDL_RenderPresent(state->renderer);
}

void cleanup(AppState *state)
{
     SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    TTF_CloseFont(state->font);
    TTF_CloseFont(state->small_font);
    SDL_Quit();
    freeCards(state);
    freeFiles(state->decks.all_files);
}

int main(int argc, char *argv[])
{
    AppState state;
    init(&state);

    if (!state.decks.selected)
    {
        // select flashcard deck
        state.decks.selected = selectionMenu(&state);
        if (!state.decks.selected)
        {
            state.running = 0;
        }
        else
            load_cards(state.decks.selected->name, &state);
    }

    while (state.running)
    {
        handleEvents(&state);
        render(&state);
        SDL_Delay(128); // increase delay ~7-8 fps, more efficient
    }

    // cleanup
    cleanup(&state);
}
