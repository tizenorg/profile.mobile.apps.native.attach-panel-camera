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

#include "utils/logger.h"
#include "utils/ui-utils.h"

#include <Elementary.h>

Evas_Object *ui_utils_navi_add(Evas_Object *parent)
{
    Evas_Object *navi = elm_naviframe_add(parent);
    RETVM_IF(!navi, NULL, "elm_naviframe_add() failed");
    elm_naviframe_prev_btn_auto_pushed_set(navi, EINA_FALSE);

    return navi;
}

Evas_Object *ui_utils_layout_add(Evas_Object *parent, Evas_Object_Event_Cb destroy_cb, void *cb_data)
{
    Evas_Object *layout = elm_layout_add(parent);
    RETVM_IF(!layout, NULL, "elm_layout_add() failed");

    elm_layout_theme_set(layout, "layout", "application", "default");

    if (destroy_cb) {
        evas_object_event_callback_add(layout, EVAS_CALLBACK_FREE, destroy_cb, cb_data);
    }

    return layout;
}
