#include "global_utils.h"
#include "interface.h"

// Struct for a toolbar element
typedef struct toolbar_element {
    char id;
    short sheet_pos_x;
    short sheet_pos_y;
    short width;
    short height;
    double x;
    double y;
}* Tool;

// Struct for selectable menu option
typedef struct select_arrow_spot {
    double x;
    double y;
    char mode_in;
    char return_val;
}* Selection;

// List of toolbar elements
enum elements
{ COOLDOWN_BAR, HEALTH_BAR, LOGO, ARROW };

// Text alignment options
enum text_align
{ L, C };

SDL_Texture* toolbar;       // Texture containing all toolbar elements
Tool* element_list;         // Array of all toolbar elements
Selection* menu_selections; // Array containing locations and return values of select arrows
int numElements = 4;        // Total number of toolbar elements
int numMenuOptions = 5;     // Total number of selection options
int font_size = 30;         // Size in pixels of a letter

// Assign toolbar element fields
static Tool initTool(char id, short sheet_x, short sheet_y, short width, short height, double x, double y)
{
    Tool this_tool = (Tool) malloc(sizeof(struct toolbar_element));
    this_tool->id = id;
    this_tool->sheet_pos_x = sheet_x;   this_tool->sheet_pos_y = sheet_y;
    this_tool->width = width;           this_tool->height = height;
    this_tool->x = x;                   this_tool->y = y;
    return this_tool;
}

// Assign menu option fields
static Selection initMenuOption(char mode_in, char return_val, double x, double y)
{
    Selection this_option = (Selection) malloc(sizeof(struct select_arrow_spot));
    this_option->x = x;
    this_option->y = y;
    this_option->mode_in = mode_in;
    this_option->return_val = return_val;
    return this_option;
}

// Load the toolbar texture, toolbar elements, and selection text into memory
void loadToolbar()
{
    toolbar = loadTexture("Art/Toolbar.bmp");
    element_list = (Tool*) malloc(numElements * sizeof(Tool));
    menu_selections = (Selection*) malloc(numMenuOptions * sizeof(Selection));
    
    element_list[COOLDOWN_BAR] = initTool(COOLDOWN_BAR, 700, 0, 39, 39, 86, 64);
    element_list[HEALTH_BAR] = initTool(HEALTH_BAR, 0, 0, 350, 80, 50, 25);
    element_list[LOGO] = initTool(LOGO, 0, 100, 520, 225, 258, 50);
    element_list[ARROW] = initTool(ARROW, 150, 441, font_size, font_size, 287, 300);
    
    menu_selections[0] = initMenuOption(TITLE, VS, 370, 300);
    menu_selections[1] = initMenuOption(TITLE, AI, 370, 300 + font_size + 10);
    menu_selections[2] = initMenuOption(TITLE, CONTROLS, 370, 300 + (font_size + 10) * 2);
    menu_selections[3] = initMenuOption(STAGE_SELECT, 0, 250, 120);
    menu_selections[4] = initMenuOption(STAGE_SELECT, 1, 250, 120 + font_size + 10);
}

// Free a toolbar element from memory
static void freeTool(Tool tl)
{
    free(tl);
}

// Free the toolbar elements, toolbar texture, and selection text from memory
void freeToolbar()
{
    for(int i = 0; i < numElements; i++) {
        freeTool(element_list[i]);
    }
    free(element_list);
    for(int i = 0; i < numMenuOptions; i++) {
        free(menu_selections[i]);
    }
    free(menu_selections);
    SDL_DestroyTexture(toolbar);
}

// Render the guys' cooldown meters (alpha blended black bars)
static void renderCooldowns(double* guy_cds, double* guy2_cds)
{
    // Starting location for cooldown meter
    SDL_Rect clip = {element_list[COOLDOWN_BAR]->sheet_pos_x, element_list[COOLDOWN_BAR]->sheet_pos_y,
                     element_list[COOLDOWN_BAR]->width, element_list[COOLDOWN_BAR]->height};
    SDL_Rect renderQuad = {(int)element_list[COOLDOWN_BAR]->x, (int)element_list[COOLDOWN_BAR]->y,
                           element_list[COOLDOWN_BAR]->width, element_list[COOLDOWN_BAR]->height};
    
    // Render cooldown meters on each spell for each guy
    SDL_SetTextureAlphaMod(toolbar, 125);
    for(int i = 0; i < numSpells; i++) {
        double cooled_down = (int)(element_list[COOLDOWN_BAR]->width * guy_cds[i]);
        clip.w = cooled_down;
        renderQuad.w = cooled_down;
        renderQuad.x = (int)element_list[COOLDOWN_BAR]->x + i * 60;
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        
        if(guy2_cds) {
            cooled_down = (int)(element_list[COOLDOWN_BAR]->width * guy2_cds[i]);
            clip.w = cooled_down;
            renderQuad.w = cooled_down;
            renderQuad.x = SCREEN_WIDTH - element_list[HEALTH_BAR]->width - (int)element_list[HEALTH_BAR]->x + 36 + i * 60;
            SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        }
    }
    SDL_SetTextureAlphaMod(toolbar, 255);
}

// Render the guys' healthbars
static void renderHealthbars(int guy_hp, int guy2_hp)
{
    // Render guy 1 outline
    Tool hp_bar = element_list[HEALTH_BAR];
    SDL_Rect clip = {hp_bar->sheet_pos_x, hp_bar->sheet_pos_y, hp_bar->width, hp_bar->height};
    SDL_Rect renderQuad = {(int)hp_bar->x, (int)hp_bar->y, hp_bar->width, hp_bar->height};
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
    
    // Render guy 2 outline
    if(guy2_hp != -1) {
        renderQuad.x = SCREEN_WIDTH - hp_bar->width - (int)hp_bar->x;
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        renderQuad.x = (int)hp_bar->x;
    }
    
    // Render red bars according to how much health each guy has left
    clip.x += hp_bar->width;
    
    // Guy 1 health remaining
    clip.w = 25 + guy_hp * 3;
    renderQuad.w = 25 + guy_hp * 3;
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
    
    // Guy 2 health remaining
    if(guy2_hp != -1) {
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

// Render the selection hover
static void renderSelectionArrow(int mode, long long frame)
{
    // If the select arrow coordinates don't correspond to the coordinates of any
    // menu option in the current mode, set them to a default for this mode
    double default_x = -1;
    double default_y = -1;
    bool match = false;
    Tool arrow = element_list[ARROW];
    for(int i = numMenuOptions - 1; i >= 0; i--) {
        Selection op = menu_selections[i];
        if(op->mode_in == mode) {
            default_x = op->x;
            default_y = op->y;
            if(op->x == arrow->x && op->y == arrow->y) {
                match = true;
                break;
            }
        }
    }
    if(!match) {
        arrow->x = default_x;
        arrow->y = default_y;
    }
    
    // Render the selection arrow, idling according to the frame
    SDL_Rect clip = {arrow->sheet_pos_x, arrow->sheet_pos_y, arrow->width, arrow->height};
    SDL_Rect renderQuad = {(int)arrow->x, (int)arrow->y, arrow->width, arrow->height};
    renderQuad.x -= ((frame / 50) % 2);
    SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
}

// Render a block of text to the screen
static void renderText(const char* text, int x, int y, int align, int fade)
{
    // Set initial cursor position based on text align type
    int len = (int)strlen(text);
    int cursor = x;
    if(align == C) {
        cursor = x - (len * font_size / 2);
    }
    
    // Iterate over string
    SDL_SetTextureAlphaMod(toolbar, fade);
    for(int i = 0; i < len; i++) {
        
        // ASCII shenanigans
        char c = text[i];
        if(c == 32) c = 101;
        if(c >= 49 && c <= 57) c += 42;
        c -= 65;
        
        // Convert char value to texture clip
        int clipx = font_size * (c % 10);
        int clipy = 351 + font_size * (c / 10);
        
        // Render character and move cursor
        SDL_Rect clip = {clipx, clipy, font_size, font_size};
        SDL_Rect renderQuad = {cursor, y, font_size, font_size};
        SDL_RenderCopy(renderer, toolbar, &clip, &renderQuad);
        cursor += font_size;
    }
    SDL_SetTextureAlphaMod(toolbar, 255);
}

// Convert an integer score into a string readable by renderText
static char* stringScore(int score)
{
    char* str = (char*) malloc(sizeof(char) * 7);
    char buf[7];
    sprintf(buf, "%06d", score);
    memcpy(str, buf, sizeof(char) * 7);
    for(int i = 0; i < 6; i++) {
        if(str[i] == '0') {
            str[i] = 'O';
        }
    }
    return str;
}

// Render all of the current mode's toolbar and text elements to the screen
void renderInterface(int mode, long long frame, int guy_hp, int guy2_hp, double* guy_cds, double* guy2_cds, int score)
{
    int no_fade = 255;
    int mid     = SCREEN_WIDTH/2;
    int y_mid   = SCREEN_HEIGHT/2 - 100;
    int margin  = font_size + 10;
    
    switch(mode) {
            
        case OPENING: {
            int y = 120;
            renderText("ONCE UPON A TIME",    mid, y,       C, min(no_fade, (double)(frame)/100*255));
            renderText("THERE WERE TWO GUYS", mid, y + 100, C, min(no_fade, (double)max(frame-100, 0)/100*255));
            renderText("AND THEY BATTLED",    mid, y + 200, C, min(no_fade, (double)max(frame-225, 0)/100*255));
            break;
        }
            
        case TITLE: {
            int y = 300;
            renderLogo(frame);
            renderSelectionArrow(mode, frame);
            renderText("2 PLAYER",     mid, y,                      C, no_fade);
            renderText("1 PLAYER",     mid, y + margin,             C, no_fade);
            renderText("CONTROLS",     mid, y + margin * 2,         C, no_fade);
            renderText("MAX LEVATICH", 10,  SCREEN_HEIGHT - margin, L, no_fade);
            break;
        }
            
        case CONTROLS: {
            int y = 40;
            renderText("2 PLAYER",              mid, y,              C, no_fade);
            renderText("X V D       P1 MOVE  ", mid, y + margin * 1, C, no_fade);
            renderText("1 2 3 4 5   P1 SPELLS", mid, y + margin * 2, C, no_fade);
            renderText("ARROW KEYS  P2 MOVE  ", mid, y + margin * 3, C, no_fade);
            renderText("Y U I O P   P2 SPELLS", mid, y + margin * 4, C, no_fade);
            y += 40;
            renderText("1 PLAYER",              mid, y + margin * 5, C, no_fade);
            renderText("ARROW KEYS     MOVE  ", mid, y + margin * 6, C, no_fade);
            renderText("1 2 3 4 5      SPELLS", mid, y + margin * 7, C, no_fade);
            renderText("ESC            PAUSE ", mid, y + margin * 8, C, no_fade);
            break;
        }
        
        case STAGE_SELECT: {
            int y = 120;
            renderSelectionArrow(mode, frame);
            renderText("CULTIST CLEARING", mid, y,          C, no_fade);
            renderText("PHOENIX MOUNTAIN", mid, y + margin, C, no_fade);
            break;
        }
            
        case VS: {
            renderHealthbars(guy_hp, guy2_hp);
            renderCooldowns(guy_cds, guy2_cds);
            break;
        }
        
        case AI: {
            int y = 25;
            char* score_string = stringScore(score);
            renderHealthbars(guy_hp, -1);
            renderCooldowns(guy_cds, NULL);
            renderText("SCORE",      600, y, L, no_fade);
            renderText(score_string, 780, y, L, no_fade);
            free(score_string);
            break;
        }
            
        case PAUSE: {
            int y = 25;
            char* score_string = stringScore(score);
            renderHealthbars(guy_hp, -1);
            renderCooldowns(guy_cds, NULL);
            renderText("PAUSED",     mid, y_mid, C, no_fade);
            renderText("SCORE",      600, y,    L, no_fade);
            renderText(score_string, 780, y,    L, no_fade);
            free(score_string);
            break;
        }
            
        case GAME_OVER: {
            renderText("GAME OVER", mid, y_mid, C, no_fade);
            break;
        }
    }
    
    free(guy_cds);
    free(guy2_cds);
}

// Move the text selection arrow
int hover(char mode, int direction)
{
    double current_y = element_list[ARROW]->y;
    double best_y = 999; int ret_val   = 0;
    double best_x = 0;   int self_ret  = 0;
    
    // From the menu options active in this mode, pick the closest in the direction we're moving
    for(int i = 0; i < numMenuOptions; i++) {
        Selection op = menu_selections[i];
        if(op->mode_in == mode) {
            if(op->y == current_y) {
                self_ret = op->return_val;
            }
            else if(direction == UP && op->y < current_y && fabs(op->y - current_y) < fabs(best_y - current_y)) {
                best_y = op->y;
                best_x = op->x;
                ret_val = op->return_val;
            }
            else if(direction == DOWN && op->y > current_y && fabs(op->y - current_y) < fabs(best_y - current_y)) {
                best_y = op->y;
                best_x = op->x;
                ret_val = op->return_val;
            }
        }
    }
    
    // If we didn't find anything, stick with the current spot
    if(best_y == 999) {
        return self_ret;
    }
    
    // Switch to the new spot
    element_list[ARROW]->x = best_x;
    element_list[ARROW]->y = best_y;
    return ret_val;
}
