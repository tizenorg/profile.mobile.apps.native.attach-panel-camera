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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <Elementary.h>
#include <ui-gadget-module.h>
#include <dlog.h>

#include "main-view.h"
#include "attach-panel-camera.h"
#define POPUP_TITLE_MAX (128)

struct ug_data *g_ugd;

#ifdef ENABLE_UG_CREATE_CB
extern int ug_create_cb(void (*create_cb)(char*,char*,char*,void*), void *user_data);
#endif

bool __attachPanelCamera_get_extra_data_cb(app_control_h service, const char *key, void *user_data);

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ATTACH_PANEL_CAMERA"

struct ug_data {
	Evas_Object *base;
	Evas_Object *pu;
	ui_gadget_h ug;
	ui_gadget_h sub_ug;
	app_control_h service;
	bool is_caller_attach_panel;
	unsigned long long size_limit;
};

#ifdef ENABLE_UG_CREATE_CB
static void create_cb(char *ug, char* mem, char* parent, void* user_data)
{
	LOGD("ug : %s, mem : %s, parent : %s, data : %p", ug, mem, parent, user_data);
}
#endif

static Evas_Object *__attachPanelCamera_create_content(Evas_Object *parent, struct ug_data *ugd)
{
	LOGD("__attachPanelCamera_create_content");
	Evas_Object *view = main_view_add(parent, ugd->ug, ugd->size_limit);
	return view;
}

bool __attachPanelCamera_get_extra_data_cb(app_control_h service, const char *key, void * user_data)
{
	char *value;
	int ret;

	ret = app_control_get_extra_data(service, key, &value);
	if (ret) {
		LOGE("__get_extra_data_cb: error get data(%d)\n", ret);
		return false;
	}

	LOGD("extra data : %s, %s\n", key, value);
	free(value);

	return true;
}

static Evas_Object *__attachPanelCamera_create_fullview(Evas_Object *parent, struct ug_data *ugd)
{
	LOGE("__attachPanelCamera_create_fullview");
	Evas_Object *base;

	base = elm_layout_add(parent);
	if (!base) {
		LOGE("base not there so return null");
		return NULL;
	}
	elm_layout_theme_set(base, "layout", "application", "default");
	elm_win_indicator_mode_set(parent, ELM_WIN_INDICATOR_SHOW);
	return base;
}

static Evas_Object *__attachPanelCamera_create_frameview(Evas_Object *parent, struct ug_data *ugd)
{
	Evas_Object *base;

	base = elm_layout_add(parent);
	if (!base) {
		LOGD("Error Failed to add layout");
		return NULL;
	}
	elm_layout_theme_set(base, "standard", "window", "integration");
	edje_object_signal_emit(_EDJ(base), "elm,state,show,content", "elm");

	return base;
}

static void *__attachPanelCamera_on_create(ui_gadget_h ug, enum ug_mode mode, app_control_h service,
		       void *priv)
{
	Evas_Object *parent = NULL;
	Evas_Object *content = NULL;
	struct ug_data *ugd = NULL;
	char *operation = NULL;

	if (!ug || !priv) {
		LOGD("Invalid ug or priv");
		return NULL;
	}

	LOGD("__attachPanelCamera_on_create start");

	ugd = priv;
	ugd->ug = ug;
	ugd->service = service;
	g_ugd = ugd;

	app_control_get_operation(service, &operation);

	if (operation == NULL) {
		/* ug called by ug_create */
		LOGD("ug called by ug_create\n");
	} else {
		/* ug called by service request */
		LOGD("ug called by service request :%s\n", operation);
		free(operation);
	}

	app_control_foreach_extra_data(service, __attachPanelCamera_get_extra_data_cb, NULL);

	parent = (Evas_Object *)ug_get_parent_layout(ug);
	if (!parent) {
		LOGD("Invalid parent");
		return NULL;
	}

	/* size limit */
	char *val = NULL;
	app_control_get_extra_data(service, "http://tizen.org/appcontrol/data/total_size", (char **)&val);
	if (val) {
		ugd->size_limit = atoi(val)/1024;
	} else {
		ugd->size_limit = 0;
	}
	LOGD("ugd->size_limit set to = %llu", ugd->size_limit);

	char *contact_id = NULL;
	app_control_get_extra_data(service, "__CALLER_PANEL__", &contact_id);
	if (contact_id && !strcmp(contact_id, "attach-panel")) {
		if (ugd) {
			ugd->is_caller_attach_panel = TRUE;
		}
	} else {
		ugd->is_caller_attach_panel = FALSE;
	}

	mode = ug_get_mode(ug);
	ugd->base = __attachPanelCamera_create_fullview(parent, ugd);
	if (ugd->base) {
	 	content = __attachPanelCamera_create_content(ugd->base, ugd);
		elm_object_part_content_set(ugd->base, "elm.swallow.content", content);
	}

	return ugd->base;
}

static void __attachPanelCamera_on_start(ui_gadget_h ug, app_control_h service, void *priv)
{
#if 0
	int i;
	pthread_t p_thread[10];
	int thr_id;
	int status;
	int a = 1;

	for (i=0; i<10; i++) {
		thr_id = pthread_create(&p_thread[i], NULL, __attachPanelCamera_start_t_func, (void*)&a);
		if (thr_id < 0) {
			perror("thread create error: ");
			exit(0);
		}
		pthread_detach(p_thread[i]);
	}
#endif
}

static void __attachPanelCamera_on_pause(ui_gadget_h ug, app_control_h service, void *priv)
{
	LOGD("%s : called\n", __func__);
	_main_view_app_pause();
}

static void __attachPanelCamera_on_resume(ui_gadget_h ug, app_control_h service, void *priv)
{
	LOGD("%s : called\n", __func__);
	_main_view_app_resume();
}

static void __attachPanelCamera_on_destroy(ui_gadget_h ug, app_control_h service, void *priv)
{
	struct ug_data *ugd;
	LOGD("%s : called\n", __func__);
	if (!ug || !priv) {
		LOGD("Invalid input arguments");
		return;
	}

	ugd = priv;
	evas_object_del(ugd->base);
	ugd->base = NULL;
}

static void __attachPanelCamera_on_message(ui_gadget_h ug, app_control_h msg, app_control_h service,
		      void *priv)
{

	if (!ug || !priv) {
		LOGD("Invalid ug or priv");
		return;
	}

	struct ug_data *ugd = NULL;
	ugd = priv;
	char *display_mode = NULL;
	char *showcontent = NULL ;

	if (ugd->is_caller_attach_panel) {
		LOGD("called by attach panel ");
		app_control_get_extra_data(msg, APP_CONTROL_DATA_SELECTION_MODE, &display_mode);
		if (display_mode) {
			if(!strcmp(display_mode, "single")) {
				//change to compact view
				LOGD("compact view ");
				_main_view_set_cam_view_layout(CAM_COMPACT_VIEW);
			} else if (display_mode && !strcmp(display_mode, "multiple")) {
				//change to full view
				LOGD("full view");
				_main_view_set_cam_view_layout(CAM_FULL_VIEW);
			} else {
				LOGD("invalid mode: %s", display_mode);
			}
		}

		app_control_get_extra_data(msg, "__ATTACH_PANEL_SHOW_CONTENT_CATEGORY__", &showcontent);
		if (showcontent) {
			if (!strcmp(showcontent, "true")) {
				_main_view_set_show_content(TRUE);
			} else {
				_main_view_set_show_content(FALSE);
			}
		}
	}
}

static void __attachPanelCamera_on_event(ui_gadget_h ug, enum ug_event event, app_control_h service,
		    void *priv)
{
	struct ug_data *ugd = priv;

	switch (event) {
	case UG_EVENT_LOW_MEMORY:
		break;
	case UG_EVENT_LOW_BATTERY:
		break;
	case UG_EVENT_LANG_CHANGE:
		break;
	case UG_EVENT_ROTATE_PORTRAIT:
		elm_layout_theme_set(ugd->base, "layout", "application", "default");
		_main_view_set_target_direction(CAM_TARGET_DIRECTION_PORTRAIT);
		break;
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		elm_layout_theme_set(ugd->base, "layout", "application", "default");
		_main_view_set_target_direction(CAM_TARGET_DIRECTION_PORTRAIT_INVERSE);
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
		elm_layout_theme_set(ugd->base, "layout", "application", "noindicator");
		_main_view_set_target_direction(CAM_TARGET_DIRECTION_LANDSCAPE);
		break;
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		elm_layout_theme_set(ugd->base, "layout", "application", "noindicator");
		_main_view_set_target_direction(CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE);
		break;
	case UG_EVENT_REGION_CHANGE:
		break;
	default:
		break;
	}

}

static void __attachPanelCamera_on_key_event(ui_gadget_h ug, enum ug_key_event event,
			 app_control_h service, void *priv)
{
	if (!ug) {
		LOGD("Invalid ug handle");
		return;
	}

	switch (event) {
	case UG_KEY_EVENT_END:
		ug_destroy_me(ug);
		break;
	default:
		break;
	}
}

static void __attachPanelCamera_on_destroying(ui_gadget_h ug, app_control_h service, void *priv)
{
	LOGD("%s : called\n", __func__);
	_main_view_destroy();
}

UG_MODULE_API int UG_MODULE_INIT(struct ug_module_ops *ops)
{
	struct ug_data *ugd = NULL;

	if (!ops) {
		LOGD("Invalid ug_module_ops");
		return -1;
	}

	ugd = calloc(1, sizeof(struct ug_data));
	if (!ugd) {
		LOGD("Error Failed to create ugd");
		return -1;
	}

	ops->create = __attachPanelCamera_on_create;
	ops->start = __attachPanelCamera_on_start;
	ops->pause = __attachPanelCamera_on_pause;
	ops->resume = __attachPanelCamera_on_resume;
	ops->destroy = __attachPanelCamera_on_destroy;
	ops->message = __attachPanelCamera_on_message;
	ops->event = __attachPanelCamera_on_event;
	/*ops->key_event = __attachPanelCamera_on_key_event;*/
	ops->destroying = __attachPanelCamera_on_destroying;
	ops->priv = ugd;

	return 0;
}

UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	struct ug_data *ugd = NULL;

	if (!ops) {
		LOGD("Invalid ug_module_ops");
		return;
	}

	ugd = ops->priv;
	if (ugd) {
		free(ugd);
	}
}
