#include "../../../main/stdafx.h"

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
//#define NK_INCLUDE_STANDARD_IO
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION


#include "GLGUIManager.h"

GLGUIManager::GLGUIManager()
{
}

GLGUIManager::~GLGUIManager()
{
}

void GLGUIManager::initialize()
{
	ctx = nk_glfw3_init(GLWindowManager::getInstance().getWindow(), NK_GLFW3_INSTALL_CALLBACKS);
	nk_glfw3_font_stash_begin(&atlas);
	//struct nk_font *freeSans = nk_font_atlas_add_from_file(atlas, "../res/fonts/fonts/DroidSans.ttf", 14, 0);
	/*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
	/*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
	/*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
	/*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
	/*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
	nk_glfw3_font_stash_end();
	background = nk_rgb(28, 48, 62);
}

void GLGUIManager::update()
{
	nk_glfw3_new_frame();
	if (nk_begin(ctx, "Properties", nk_rect(50, 50, 230, 250),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
		NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{

		nk_layout_row_dynamic(ctx, 30, 2);
		nk_layout_row_static(ctx, 30, 100, 1);

		nk_layout_row_dynamic(ctx, 20, 1);
		nk_label(ctx, "background:", NK_TEXT_LEFT);

		nk_layout_row_dynamic(ctx, 25, 1);
		if (nk_combo_begin_color(ctx, background, nk_vec2(nk_widget_width(ctx), 400))) {
			nk_layout_row_dynamic(ctx, 120, 1);
			nk_color canvasBackground = background;
			canvasBackground = nk_color_picker(ctx, canvasBackground, NK_RGBA);
			nk_layout_row_dynamic(ctx, 25, 1);
			background.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, canvasBackground.r, 255, 1, 1);
			background.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, canvasBackground.g, 255, 1, 1);
			background.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, canvasBackground.b, 255, 1, 1);
			background.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, canvasBackground.a, 255, 1, 1);

			
			nk_combo_end(ctx);
		}
	}
	nk_end(ctx);

	float bg[4];
	nk_color_fv(bg, background);
	nk_glfw3_render(NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
}

void GLGUIManager::shutdown()
{
	nk_glfw3_shutdown();
}
