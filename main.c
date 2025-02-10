
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    freeCards();
}