#include "../headers/constants.h"
#include "../headers/interface.h"

// Struct for a toolbar element
typedef struct toolbar_element
{
    int id;            // which toolbar element is this (LOGO, HEALTH_BAR, etc)
    int sheet_pos_x;   // x location on the sprite sheet
    int sheet_pos_y;   // y location on the sprite sheet
    int width;         // width in pixels
    int height;        // height in pixels
    double x;          // x position to render to
    double y;          // y position to render to
}* Tool;

// Struct for selectable menu option
typedef struct menu_selection
{
    double x;          // x position to render to
    double y;          // y position to render to
    int mode_in;       // mode this option appears in
    int mode_out;      // mode this option redirects to
}* Selection;

// List of toolbar elements
enum elements
{ COOLDOWN_BAR, HEALTH_BAR, LOGO, ARROW };

// Text alignment options
enum text_align
{ L, C };

#define NUM_ELEMENTS 4      // Total number of toolbar elements
#define NUM_MENU_OPTIONS 5  // Total number of menu options (across all menus)
#define FONT_SIZE 30        // Size in pixels of a letter

SDL_Texture* toolbar;       // Texture containing all toolbar elements
Tool* element_list;         // Array of all toolbar elements
Selection* menu_selections; // Array containing locations and return values of select arrows
int score = 0;              // The score, for 1-player games

/* SETTERS */

// Move the text selection arrow
int hover(int mode, int direction)
{
    double current_y = element_list[ARROW]->y;
    double best_y = 999; int ret_val   = 0;
    double best_x = 0;   int self_ret  = 0;

    // From the menu options active in this mode, pick the closest in the direction we're moving
    for(int i = 0; i < NUM_MENU_OPTIONS; i++)
    {
        Selection op = menu_selections[i];
        if(op->mode_in == mode)
        {
            if(op->y == current_y)
            {
                self_ret = op->mode_out;
            }
            else if(direction == UP && op->y < current_y && fabs(op->y - current_y) < fabs(best_y - current_y))
            {
                best_y = op->y;
                best_x = op->x;
                ret_val = op->mode_out;
            }
            else if(direction == DOWN && op->y > current_y && fabs(op->y - current_y) < fabs(best_y - current_y))
            {
                best_y = op->y;
                best_x = op->x;
                ret_val = op->mode_out;
            }
        }
    }

    // If we didn't find anything (there were no further options in the direction we moved),
    // stick with the current spot
    if(best_y == 999) return self_ret;

    // Otherwise, switch to the new spot
    element_list[ARROW]->x = best_x;
    element_list[ARROW]->y = best_y;
    return ret_val;
}

// Reset the score
void setScore(int new_score)
{
    score = new_score;
}

// Add points to the score
void updateScore(int points)
{
    score += points;
}

/* GETTERS */

// Convert an integer score into a string readable by renderText
static char* stringScore(int score)
{
    // Copy number into buffer
    char* str = (char*) malloc(sizeof(char) * 7);
    sprintf(str, "%06d", score);

    // Swap out zeros for the letter O
    for(int i = 0; i < 6; i++)
    {
        if(str[i] == '0') str[i] = 'O';
    }
    return str;
}

/* ELEMENT RENDERING */

// Render the guys' cooldown meters (alpha blended black bars)
static void renderCooldowns(double* guy1_cds, double* guy2_cds)
{
    // Starting location for cooldown meter
    Tool bar = element_list[COOLDOWN_BAR];
    SDL_Rect clip = {bar->sheet_pos_x, bar->sheet_pos_y, bar->width, bar->height};
    SDL_Rect renderQuad = {(int) bar->x, (int) bar->y, bar->width, bar->height};

    // Render cooldown meters on each spell for each guy
    SDL_SetTextureAlphaMod(toolbar, 125);
    for(int i = 0; guy1_cds[i] >= 0; i++)
    {
        // Render cooldown meter of spell i for first Guy
        double cooled_down = (int) (bar->width * guy1_cds[i]);
        clip.w = cooled_down;
        renderQuad.w = cooled_down;
        renderQuad.x = (int) bar->x + i * 60;
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);

        // Render cooldown meter of spell i for second Guy if in VS mode
        if(guy2_cds)
        {
            cooled_down = (int) (bar->width * guy2_cds[i]);
            clip.w = cooled_down;
            renderQuad.w = cooled_down;
            Tool hp_bar = element_list[HEALTH_BAR];
            renderQuad.x = SCREEN_WIDTH - hp_bar->width - (int) hp_bar->x + 36 + i * 60;
            SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        }
    }
    SDL_SetTextureAlphaMod(toolbar, 255);
}

// Render the guys' healthbars
static void renderHealthbars(int guy1_hp, int guy2_hp)
{
    // Render guy 1 outline
    Tool hp_bar = element_list[HEALTH_BAR];
    SDL_Rect clip = {hp_bar->sheet_pos_x, hp_bar->sheet_pos_y, hp_bar->width, hp_bar->height};
    SDL_Rect renderQuad = {(int)hp_bar->x, (int)hp_bar->y, hp_bar->width, hp_bar->height};
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);

    // Render guy 2 outline if in VS mode
    if(guy2_hp != -1)
    {
        renderQuad.x = SCREEN_WIDTH - hp_bar->width - (int)hp_bar->x;
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        renderQuad.x = (int)hp_bar->x;
    }

    // Render health remaining for Guy 1
    clip.x += hp_bar->width;
    clip.w = 25 + guy1_hp * 3;
    renderQuad.w = 25 + guy1_hp * 3;
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);

    // Render health remaining for Guy 2 if in VS mode
    if(guy2_hp != -1)
    {
        clip.w = 25 + guy2_hp * 3;
        renderQuad.x = SCREEN_WIDTH - hp_bar->width - (int)hp_bar->x + (300 - guy2_hp * 3);
        renderQuad.w = 25 + guy2_hp * 3;
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
    }
}

// Render title
static void renderLogo(long long frame)
{
    // Render the logo, idling according to the frame
    Tool logo = element_list[LOGO];
    SDL_Rect clip = {logo->sheet_pos_x, logo->sheet_pos_y, logo->width, logo->height};
    SDL_Rect renderQuad = {(int)logo->x, (int)logo->y, logo->width, logo->height};
    renderQuad.y -= ((frame / 30) % 2);
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
}

// Render the selection arrow
static void renderSelectionArrow(int mode, long long frame)
{
    // If the select arrow coordinates don't correspond to the coordinates of any
    // menu option in the current mode, set them to a default for this mode
    double default_x = -1;
    double default_y = -1;
    bool match = false;
    Tool arrow = element_list[ARROW];
    for(int i = NUM_MENU_OPTIONS - 1; i >= 0; i--) {
        Selection op = menu_selections[i];
        if(op->mode_in == mode)
        {
            default_x = op->x;
            default_y = op->y;
            if(op->x == arrow->x && op->y == arrow->y)
            {
                match = true;
                break;
            }
        }
    }
    if(!match)
    {
        arrow->x = default_x;
        arrow->y = default_y;
    }

    // Render the selection arrow, idling according to the frame
    SDL_Rect clip = {arrow->sheet_pos_x, arrow->sheet_pos_y, arrow->width, arrow->height};
    SDL_Rect renderQuad = {(int)arrow->x, (int)arrow->y, arrow->width, arrow->height};
    renderQuad.x -= ((frame / 50) % 2);
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
}

// Render a piece of text to the screen
static void renderText(const char* text, int x, int y, int align, int fade)
{
    // Set initial cursor position based on text align type
    int len = (int) strlen(text);
    int cursor = x;
    if(align == C) cursor = x - (len * FONT_SIZE / 2);

    // Iterate over string
    SDL_SetTextureAlphaMod(toolbar, fade);
    for(int i = 0; i < len; i++)
    {
        // ASCII shenanigans
        char c = text[i];
        if(c == 32) c = 101;
        if(c >= 49 && c <= 57) c += 42;
        c -= 65;

        // Convert char value to texture clip
        int clipx = FONT_SIZE * (c % 10);
        int clipy = 351 + FONT_SIZE * (c / 10);

        // Render character and move cursor
        SDL_Rect clip = {clipx, clipy, FONT_SIZE, FONT_SIZE};
        SDL_Rect renderQuad = {cursor, y, FONT_SIZE, FONT_SIZE};
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        cursor += FONT_SIZE;
    }
    SDL_SetTextureAlphaMod(toolbar, 255);
}

/* PER FRAME UPDATE */

// Render all of the current mode's toolbar and text elements to the screen
void renderInterface(int mode, long long frame, int guy1_hp, int guy2_hp, double* guy1_cds, double* guy2_cds)
{
    int alpha_max = 255;
    int x = SCREEN_WIDTH / 2;
    int margin = FONT_SIZE + 10;
    switch(mode)
    {
        case OPENING:
        {
            int y = 120;
            renderText("ONCE UPON A TIME",    x, y,       C, fmin(alpha_max, (double)(frame)/100*255));
            renderText("THERE WERE TWO GUYS", x, y + 100, C, fmin(alpha_max, (double)fmax(frame-100, 0)/100*255));
            renderText("AND THEY BATTLED",    x, y + 200, C, fmin(alpha_max, (double)fmax(frame-225, 0)/100*255));
            break;
        }

        case TITLE:
        {
            int y = 300;
            renderLogo(frame);
            renderSelectionArrow(mode, frame);
            renderText("2 PLAYER",     x,  y,                      C, alpha_max);
            renderText("1 PLAYER",     x,  y + margin,             C, alpha_max);
            renderText("CONTROLS",     x,  y + margin * 2,         C, alpha_max);
            renderText("MAX LEVATICH", 10, SCREEN_HEIGHT - margin, L, alpha_max);
            break;
        }

        case CONTROLS:
        {
            int y = 40;
            renderText("2 PLAYER",              x, y,              C, alpha_max);
            renderText("X V D       P1 MOVE  ", x, y + margin * 1, C, alpha_max);
            renderText("1 2 3 4 5   P1 SPELLS", x, y + margin * 2, C, alpha_max);
            renderText("ARROW KEYS  P2 MOVE  ", x, y + margin * 3, C, alpha_max);
            renderText("Y U I O P   P2 SPELLS", x, y + margin * 4, C, alpha_max);
            renderText("1 PLAYER",              x, y + margin * 6, C, alpha_max);
            renderText("ARROW KEYS     MOVE  ", x, y + margin * 7, C, alpha_max);
            renderText("1 2 3 4 5      SPELLS", x, y + margin * 8, C, alpha_max);
            renderText("ESC            PAUSE ", x, y + margin * 9, C, alpha_max);
            break;
        }

        case STAGE_SELECT:
        {
            int y = 120;
            renderSelectionArrow(mode, frame);
            renderText("CULTIST CLEARING", x, y,          C, alpha_max);
            renderText("PHOENIX MOUNTAIN", x, y + margin, C, alpha_max);
            break;
        }

        case VS:
        {
            renderHealthbars(guy1_hp, guy2_hp);
            renderCooldowns(guy1_cds, guy2_cds);
            break;
        }

        case AI:
        {
            int y = 25;
            char* score_string = stringScore(score);
            renderHealthbars(guy1_hp, -1);
            renderCooldowns(guy1_cds, NULL);
            renderText("SCORE",      600, y, L, alpha_max);
            renderText(score_string, 780, y, L, alpha_max);
            free(score_string);
            break;
        }

        case PAUSE:
        {
            int y = 25;
            char* score_string = stringScore(score);
            renderHealthbars(guy1_hp, -1);
            renderCooldowns(guy1_cds, NULL);
            renderText("PAUSED",     x,   280, C, alpha_max);
            renderText("SCORE",      600, y,   L, alpha_max);
            renderText(score_string, 780, y,   L, alpha_max);
            free(score_string);
            break;
        }

        case GAME_OVER_VS:
        {
            renderText("GAME OVER", x, 280, C, alpha_max);
            break;
        }

        case GAME_OVER_AI:
        {
            int y = 280;
            char* score_string = stringScore(score);
            renderText("GAME OVER",  x,       y,            C, alpha_max);
            renderText("SCORE",      x - 100, y + 2*margin, C, alpha_max);
            renderText(score_string, x + 80,  y + 2*margin, C, alpha_max);
            free(score_string);
        }
    }
    free(guy1_cds);
    free(guy2_cds);
}

/* DATA ALLOCATION / INITIALIZATION */

// Assign toolbar element fields
static Tool initTool(int id, int sheet_x, int sheet_y, int width, int height, double x, double y)
{
    Tool this_tool = (Tool) malloc(sizeof(struct toolbar_element));
    this_tool->id = id;
    this_tool->sheet_pos_x = sheet_x;   this_tool->sheet_pos_y = sheet_y;
    this_tool->width = width;           this_tool->height = height;
    this_tool->x = x;                   this_tool->y = y;
    return this_tool;
}

// Assign menu option fields
static Selection initMenuOption(int mode_in, int mode_out, double x, double y)
{
    Selection this_option = (Selection) malloc(sizeof(struct menu_selection));
    this_option->x = x;
    this_option->y = y;
    this_option->mode_in = mode_in;
    this_option->mode_out = mode_out;
    return this_option;
}

// Load the toolbar texture, toolbar elements, and selection text into memory
void loadInterface()
{
    // Load the texture containing all toolbar elements and the alphabet
    toolbar = loadTexture("art/Toolbar.bmp");

    // Make space for the toolbar elements and initialize them
    element_list = (Tool*) malloc(NUM_ELEMENTS * sizeof(Tool));
    element_list[COOLDOWN_BAR] = initTool(COOLDOWN_BAR, 700, 0, 39, 39, 86, 64);
    element_list[HEALTH_BAR] = initTool(HEALTH_BAR, 0, 0, 350, 80, 50, 25);
    element_list[LOGO] = initTool(LOGO, 0, 100, 520, 225, 258, 50);
    element_list[ARROW] = initTool(ARROW, 150, 441, FONT_SIZE, FONT_SIZE, 287, 300);

    // Make space for the different menu options and initialize them
    menu_selections = (Selection*) malloc(NUM_MENU_OPTIONS * sizeof(Selection));
    menu_selections[0] = initMenuOption(TITLE, VS, 370, 300);
    menu_selections[1] = initMenuOption(TITLE, AI, 370, 300 + FONT_SIZE + 10);
    menu_selections[2] = initMenuOption(TITLE, CONTROLS, 370, 300 + (FONT_SIZE + 10) * 2);
    menu_selections[3] = initMenuOption(STAGE_SELECT, 0, 250, 120);
    menu_selections[4] = initMenuOption(STAGE_SELECT, 1, 250, 120 + FONT_SIZE + 10);
}

/* DATA UNLOADING */

// Free the toolbar elements, selection options, and toolbar texture from memory
void freeInterface()
{
    for(int i = 0; i < NUM_ELEMENTS; i++) free(element_list[i]);
    free(element_list);

    for(int i = 0; i < NUM_MENU_OPTIONS; i++) free(menu_selections[i]);
    free(menu_selections);

    SDL_DestroyTexture(toolbar);
}
