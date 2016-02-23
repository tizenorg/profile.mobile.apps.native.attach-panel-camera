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


#ifndef __MAIN_VIEW_H__
#define __MAIN_VIEW_H__

#include <Evas.h>
#include <Ecore_Evas.h>
#include <ui-gadget-module.h>
#include <Elementary.h>
#include <camera.h>
#include <recorder.h>
#include <device/power.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
 * @brief Adding new view to parent object
 * @param[in]   parent  Parent naviframe
 * @return Main view layout on success, otherwise NULL
 */

/* Macros
 */
#define HIWORD(x)					((x) >> 16)
#define LOWORD(y)					(((y) << 16) >> 16)
#define MAKE_DWORD(x, y)				((x) << 16 | (y))

#define CAM_RESOLUTION(w, h)				MAKE_DWORD(w, h)
#define CAM_RESOLUTION_W(r)				HIWORD(r)
#define CAM_RESOLUTION_H(r)				LOWORD(r)

#if 0
/*For 2Mp resolution*/
#define CAM_WIDTH				1600
#define CAM_HEIGHT				1200
#else
/*For 640X480 resolution*/
#define CAM_WIDTH				640
#define CAM_HEIGHT				480
#endif
#define CAM_RESOLUTION_QCIF				CAM_RESOLUTION(176, 144)
#define CAM_RESOLUTION_VGA				CAM_RESOLUTION(640, 480)

#define CAM_REC_MMS_MAX_SIZE	(295)	/* kbyte */
#define CAM_REC_NORMAL_MAX_SIZE	(4*1024*1024)	/* kbyte */

#define CAM_PACKAGE						"org.tizen.camera-app"

typedef enum _CamTargetDirection {
	CAM_TARGET_DIRECTION_INVAILD = -1,
	CAM_TARGET_DIRECTION_PORTRAIT = 0,
	CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE = 90,
	CAM_TARGET_DIRECTION_PORTRAIT_INVERSE = 180,
	CAM_TARGET_DIRECTION_LANDSCAPE = 270,
	CAM_TARGET_DIRECTION_MAX,
} CamTargetDirection;

typedef enum _CamViewType{
	CAM_FULL_VIEW,
	CAM_COMPACT_VIEW,
}CamViewType;

typedef enum _CamTransformType {
	CAM_TRANSFORM_NONE,
	CAM_TRANSFORM_CROP ,
	CAM_TRANSFORM_RESIZE,
}CamTransformType;

enum {
	NORMAL_BATTERY_STATUS = 0,
	LOW_BATTERY_WARNING_STATUS,
	LOW_BATTERY_CRITICAL_STATUS,
};

typedef struct
{
	Evas_Object *navi;
	Elm_Object_Item *navi_item;
	Evas_Object *layout;
	Evas_Object *cameraview_layout;
	Evas_Object *recorderview_layout;
	Evas_Object *popup;
	Evas_Object *preview_canvas;
	Evas_Object *recording_icon;
	Eina_Bool pause_state;
	Evas_Object *progressbar_layout;
	Evas_Object *low_battery_layout;
	Evas_Object *progressbar;

	/*check out low battery */
	Eina_Bool battery_status;

	camera_h camera;
	recorder_h hrec;
	Eina_Bool camera_enabled;
	Eina_Bool self_portrait;
	Eina_Bool is_size_limit;
	Eina_Bool show_content;
	int photo_resolution;
	int video_resolution;
	int device_type;
	CamTargetDirection target_direction;
	CamTransformType transformtype;

	unsigned long long rec_elapsed;
	unsigned long long pause_time;
	unsigned long long rec_filesize;
	unsigned long long size_limit;

	Ecore_Timer *timer;
	Ecore_Timer *pause_timer;
	Ecore_Timer *crop_timer;
	char filename[PATH_MAX];
	int timer_count;
	ui_gadget_h ug_handle;
	CamViewType cam_view_type;
	char *edje_path;
} main_view;

Evas_Object *main_view_add(Evas_Object *navi, ui_gadget_h ug_handle, unsigned long long size_limit);
void _main_view_set_data(void *data);
void *_main_view_get_data();
void _main_view_app_pause();
void _main_view_app_resume();
void _main_view_set_target_direction(CamTargetDirection target_direction);
void _main_view_destroy();
void _main_view_set_cam_view_layout(CamViewType cam_view_type);
void _main_view_set_show_content(Eina_Bool showContent);
typedef enum _CamFileType {
	CAM_FILE_IMAGE ,
	CAM_FILE_VIDEO ,
} CamFileType;

#endif /* __MAIN_VIEW_H__ */
