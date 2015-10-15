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



#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <Evas.h>
#include <Ecore_Evas.h>

typedef struct _window window;

/*
 * @brief Create application main window
 * @return Window on success, otherwise NULL
 */
window *window_create();

/*
 * @brief Lower application window to hide application without exiting
 * @param[in]   win     Application window
 */
void window_destroy(window *win);

/*
 * @brief Destroy application main window
 * @param[in]   win     Application window
 */
void window_lower(window *win);

/*
 * @brief Set content to be displayed in window
 * @param[in]   win     Application window
 * @param[in]   content Window content
 */
void window_content_set(window *win, Evas_Object *layout);

/*
 * @brief Get window layout to use as a parent for window content
 * @param[in]   win     Application window
 * @return Window layout
 */
Evas_Object *window_layout_get(const window *win);

#endif /* __WINDOW_H__ */
