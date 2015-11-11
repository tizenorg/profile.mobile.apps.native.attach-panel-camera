/*
* Copyright (c) 2000-2015 Samsung Electronics Co., Ltd All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include "view/window.h"

#include "utils/logger.h"

#include <Elementary.h>

#define APP_NAME "camera"

struct _window {
	Evas_Object *win;
	Evas_Object *conform;
};

window *window_create()
{
	window *win = calloc(1, sizeof(window));
	RETVM_IF(!win, NULL, "calloc() failed");

	win->win = elm_win_add(NULL, APP_NAME, ELM_WIN_BASIC);
	elm_win_indicator_mode_set(win->win, ELM_WIN_INDICATOR_HIDE);
	elm_win_conformant_set(win->win, EINA_TRUE);
	elm_app_base_scale_set(1.8);
	evas_object_show(win->win);

	win->conform = elm_conformant_add(win->win);
	evas_object_size_hint_weight_set(win->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win->win, win->conform);
	evas_object_show(win->conform);

	Evas_Object *bg = elm_bg_add(win->conform);
	elm_object_part_content_set(win->conform, "elm.swallow.bg", bg);

	return win;
}

void window_destroy(window *win)
{
	RETM_IF(!win, "win is NULL");
	evas_object_del(win->win);
	free(win);
}

void window_lower(window *win)
{
	RETM_IF(!win, "win is NULL");
	elm_win_lower(win->win);
}

void window_content_set(window *win, Evas_Object *content)
{
	RETM_IF(!win, "win is NULL");
	RETM_IF(!content, "content is NULL");
	elm_object_part_content_set(win->conform, "elm.swallow.content", content);
}

Evas_Object *window_layout_get(const window *win)
{
	RETVM_IF(!win, NULL, "win is NULL");
	return win->conform;
}
