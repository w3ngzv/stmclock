#include "app.h"
#include "input.h"
#include "oled.h"

#define MENU_ROW_H      10
#define MENU_ROWS_SHOWN 5
#define MENU_FIRST_Y    13

static void menu_enter(void);
static void menu_update(const input_state_t *in);

static const page_t page_menu = {
	.name = "MENU", .enter = menu_enter, .update = menu_update,
};

/* 菜单项, 时钟页是主界面, 不列入 */
static const page_t *const menu_items[] = {
	&page_settime,
	&page_stopwatch,
	&page_torch,
	&page_motion,
	&page_picture,
};
#define MENU_COUNT ((int)(sizeof(menu_items) / sizeof(menu_items[0])))

static const page_t *current;
static int menu_cursor;

void app_init(void)
{
	current = 0;
	app_goto(&page_clock);
}

void app_goto(const page_t *page)
{
	if (current && current->leave)
		current->leave();
	current = page;
	if (current->enter)
		current->enter();
}

void app_menu(void)
{
	app_goto(&page_menu);
}

int app_keep_awake(void)
{
	return current->keep_awake;
}

void app_update(const input_state_t *in)
{
	if (in->longs & KEYM(KEY_ENC)) {
		if (current != &page_clock)
			app_goto(&page_clock);
		return;
	}
	if ((in->keys & KEYM(KEY_BACK)) && current != &page_clock) {
		app_goto(current == &page_menu ? &page_clock : &page_menu);
		return;
	}
	current->update(in);
}

static void menu_enter(void) { }

static void menu_update(const input_state_t *in)
{
	int first, row;

	menu_cursor += in->enc;
	if (menu_cursor < 0)
		menu_cursor = 0;
	if (menu_cursor >= MENU_COUNT)
		menu_cursor = MENU_COUNT - 1;
	if (in->keys & (KEYM(KEY_OK) | KEYM(KEY_ENC))) {
		app_goto(menu_items[menu_cursor]);
		return;
	}

	first = menu_cursor - (MENU_ROWS_SHOWN - 1);
	if (first < 0)
		first = 0;

	gfx_clear();
	gfx_text(0, 2, "MENU", 1, 1);
	gfx_hline(0, 11, OLED_WIDTH, 1);
	for (row = 0; row < MENU_ROWS_SHOWN && first + row < MENU_COUNT; row++) {
		int idx = first + row;
		int y = MENU_FIRST_Y + row * MENU_ROW_H;
		int selected = idx == menu_cursor;

		if (selected)
			gfx_fill_rect(0, y - 1, OLED_WIDTH, MENU_ROW_H, 1);
		gfx_text(4, y + 1, menu_items[idx]->name, 1, !selected);
	}
}
