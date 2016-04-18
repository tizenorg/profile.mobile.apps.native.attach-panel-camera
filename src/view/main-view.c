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

#include "view/main-view.h"
#include "view/window.h"
#include <libexif/exif-data.h>
#include <libexif/exif-loader.h>
#include "utils/config.h"
#include "utils/logger.h"
#include "utils/ui-utils.h"

#include <device/callback.h>
#include <device/battery.h>
#include <device/power.h>
#include <dlog.h>
#include <app_control.h>
#include <media_content.h>
#include <notification.h>

#include <image_util.h>
#include <media_format.h>
#include <media_packet.h>
#include <config.h>

#define COUNTER_STR_LEN 3
#define FILE_PREFIX "IMAGE"
#define FILE_VIDEO_PREFIX "VIDEO"

#define CAM_TIME_FORMAT "u:%02u:%02u"
#define CAM_TIME_FORMAT2 "02u:%02u"
#define SECONDS_IN_HOUR			(1*60*60)
#define TIME_FORMAT_MAX_LEN		(128)
#define _EDJ(x)  (Evas_Object *)elm_layout_edje_get(x)

unsigned char* exifData = NULL;
unsigned int exif_size = 0;

#define REMOVE_TIMER(timer) \
	if (timer != NULL) {\
		ecore_timer_del(timer); \
		timer = NULL; \
	}

#define IF_FREE(p) \
	if (p != NULL) {\
		free(p); \
		p = NULL; \
	}

#define CAM_TIME_ARGS(t) \
	(uint) (t / (60*60)), \
	(uint) ((t / 60) % 60), \
	(uint) (t % 60)

#define CAM_TIME_ARGS2(t) \
	(uint) ((t / 60) % 60), \
	(uint) (t % 60)

#define startfunc   		LOGD("+-  START -------------------------");
#define endfunc     		LOGD("+-  END  --------------------------");
#define JPEG_DATA_OFFSET		2
#define JPEG_EXIF_OFFSET		4
#define EXIF_MARKER_SOI_LENGTH	2
#define EXIF_MARKER_APP1_LENGTH	2
#define EXIF_APP1_LENGTH		2

static const char *_error = "Error";
static const char *_camera_init_failed = "Camera initialization failed.";
static const char *_ok = "OK";
static const char _file_prot_str[] = "file://";

recorder_h grecord = NULL;

media_format_h fmt;
media_packet_h pkt = NULL;
media_packet_h pkt_rotate = NULL;
media_packet_h pkt_crop = NULL;
media_packet_h pkt_resize = NULL;
transformation_h handle = NULL;
transformation_h handle_rotate = NULL ;
transformation_h handle_crop = NULL ;
transformation_h handle_resize = NULL;
static main_view *viewhandle = NULL;
Evas_Object *_main_view_load_edj(Evas_Object *parent, const char *file, const char *group);
static Eina_Bool _main_view_init_camera(main_view *view, int camera_type);
static void _main_view_register_cbs(main_view *view);
/*static void _main_view_back_cb(void *data, Evas_Object *obj, void *event_info);*/
static void _main_view_pause_cb(void *data, Evas_Object *obj, void *event_info);
static void _main_view_resume_cb(void *data, Evas_Object *obj, void *event_info);

/*static size_t _main_view_get_last_file_path(char *file_path, size_t size);*/
static size_t _main_view_get_file_path(char *file_path, size_t size, CamFileType fileType);

static void _main_view_capture_cb(camera_image_data_s *image,
		camera_image_data_s *postview, camera_image_data_s *thumbnail, void *user_data);
static void _main_view_capture_completed_cb(void *data);
static Eina_Bool _main_view_start_camera_preview(camera_h camera);
static Eina_Bool _main_view_stop_camera_preview(camera_h camera);

static void _main_view_shutter_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);
/*static void _main_view_video_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);*/
static void _main_view_switch_camera_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);
static void _main_view_recorder_pause_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);
static void _main_view_recorder_stop_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);
static void _main_view_recorder_resume_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source);

static void _main_view_ug_send_result(main_view *view, char *file_path);
static bool _main_file_register(const char *filename);
static void _main_view_recorder_rec_icon_update(main_view *view);
static Eina_Bool _main_view_recorder_pause_icon_update(void *data);
static void _main_view_recorder_set_recording_time(main_view *view);

static int _main_view_get_image_orient_value_by_direction(void *data, CamTargetDirection target_direction);
static int _main_view_get_image_orient_value(void *data);
static void _main_view_show_warning_popup(Evas_Object *navi, const char *caption, const char *text, const char *button_text, void *data);
static void _main_view_popup_close_cb(void *data, Evas_Object *obj, void *event_info);
static void __recording_view_progressbar_create(main_view *view);
static Eina_Bool _main_view_util_lcd_lock();
static Eina_Bool _main_view_util_lcd_unlock();
int error_type = 0;

static void set_edje_path(main_view *view)
{
	RETM_IF(!view, "mainview is null");
	if (view->edje_path == NULL) {
		view->edje_path = (char *)malloc(sizeof(char)*1024);
		snprintf(view->edje_path, 1024, "%s", SELF_CAMERA_LAYOUT);
/*
		char *path = app_get_resource_path();
		if(path == NULL) {
			DBG("path is null");
			return ;
		}
		view->edje_path = (char *)malloc(sizeof(char)*1024);
		snprintf(view->edje_path, 1024, "%s%s/%s", path, "edje", SELF_CAMERA_LAYOUT);
		DBG("edje_path path = %s",view->edje_path);
		free(path);
*/
	}
}

static void __cam_interrupted_cb(camera_policy_e policy, camera_state_e previous, camera_state_e current, void *data)
{
	main_view *view = (main_view *)data;
	RETM_IF(!view, "mainview is null");

	DBG("policy is [%d]", policy);
	switch (policy) {
	case CAMERA_POLICY_SOUND:
		break;
	case CAMERA_POLICY_SOUND_BY_CALL:
	case CAMERA_POLICY_SOUND_BY_ALARM:
		//edje_object_part_text_set(_EDJ(view->cameraview_layout), "content_text", string_msg);
		view->low_battery_layout = _main_view_load_edj(view->cameraview_layout,
									view->edje_path,
		                           "battery_low_layout");
		elm_object_part_content_set(view->cameraview_layout, "battery_low",
		                            view->low_battery_layout);
		elm_object_domain_translatable_part_text_set(view->low_battery_layout, "low_text",
		        CAM_PACKAGE, "IDS_CAM_POP_UNABLE_TO_START_CAMERA_DURING_CALL");
		break;
	default:
		break;
	}
}
static Eina_Bool _main_view_util_lcd_lock()
{
	int ret = 0;

	ret = device_power_request_lock(POWER_LOCK_DISPLAY, 0);
	RETVM_IF(ret != 0, FALSE, "device_power_request_lock fail [%d]", ret);

	DBG("display lock");

	return TRUE;
}

static Eina_Bool _main_view_util_lcd_unlock()
{
	int ret = 0;

	ret = device_power_release_lock(POWER_LOCK_DISPLAY);
	RETVM_IF(ret != 0, FALSE, "device_power_release_lock fail [%d]", ret);

	DBG("display unlock");

	return TRUE;
}

void toast_popup_destroy(void *data)
{
	RETM_IF(!data, "data is null");
	main_view *view = data;

	if (view->popup) {
		evas_object_del(view->popup);
	}
}

void toast_popup_create(void* data, const char *msg, void (*func)(void *data, Evas_Object *obj, void *event_info))
{
	RETM_IF(!data, "data is null");
	main_view *view = data;

	toast_popup_destroy(view);

	if (func == NULL) {
		notification_status_message_post(msg);
		return;
	}

	Evas_Object *popup = NULL;
	popup = elm_popup_add(view->layout);

	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	/* custom attributes */
	elm_object_text_set(popup, msg);
	elm_popup_timeout_set(popup, 3.0);

	evas_object_smart_callback_add(popup, "timeout", func, data);

	view->popup = popup;

	if (view->popup) {
		evas_object_show(popup);
	}
}

static Eina_Bool _main_view_send_result_after_transform(void *data)
{
	startfunc
	main_view *view = (main_view *)data;
	if (view->transformtype == CAM_TRANSFORM_CROP) {
		DBG("crop completed, Start resize");
		_main_view_ug_send_result(view, view->filename);
		_main_view_start_camera_preview(view->camera);
		view->transformtype = CAM_TRANSFORM_NONE;
	}
	return ECORE_CALLBACK_CANCEL;
	endfunc
}

bool __setUint16(void* data, unsigned short in)
{
	((unsigned char *)data)[0] = in >> 8;
	((unsigned char *)data)[1] = in & 0x00ff;

	return true;
}

static void _main_view_copy_exifdata_to_buffer(unsigned char* inData, unsigned int inSize, unsigned char** outData, unsigned int* outSize)
{
	startfunc
	unsigned char *data = NULL;
	unsigned int dataSize = 0;
	unsigned int exifSize = exif_size;
	unsigned short head[2] = {0,};
	unsigned short headLen = 0;
	int jpegOffset = JPEG_DATA_OFFSET;

	DBG("Exif Data is : %s", exifData);
	dataSize = EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH + EXIF_APP1_LENGTH + exif_size + (inSize - jpegOffset);

	__setUint16(&head[0], 0xffd8);
	__setUint16(&head[1], 0xffe1);
	__setUint16(&headLen, (unsigned short)(exifSize + 2));

	if (head[0] == 0 || head[1] == 0 || headLen == 0) {
		DBG("Set header failed");
		return;
	}

	data = (unsigned char*)malloc(dataSize);
	RETM_IF(!data, "data is null");

	/* Complete JPEG+EXIF */
	/* SOI marker */
	memcpy(data, &head[0], EXIF_MARKER_SOI_LENGTH);

	/*APP1 marker*/
	memcpy(data + EXIF_MARKER_SOI_LENGTH, &head[1], EXIF_MARKER_APP1_LENGTH);

	/*length of APP1*/
	memcpy(data + EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH, &headLen, EXIF_APP1_LENGTH);

	/*EXIF*/
	memcpy(data + EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH + EXIF_APP1_LENGTH, exifData, exifSize);

	/*IMAGE*/
	memcpy(data + EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH + EXIF_APP1_LENGTH + exifSize, inData + jpegOffset, inSize - jpegOffset);

	if (data != NULL) {
		*outData = data;
		*outSize = dataSize;
	}
	IF_FREE(exifData);
	endfunc
}

void _main_view_transform_completed(media_packet_h *dst, int error_code, void *user_data)
{
	startfunc
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "main_view is NULL");
	int ret = 0;
	void* src_ptr;
	uint64_t size = 0;
	media_format_h dst_fmt;
	media_format_mimetype_e dst_mimetype;
	int dst_width, dst_height, dst_avg_bps, dst_max_bps;
	if (media_packet_get_format(*dst, &dst_fmt) != MEDIA_PACKET_ERROR_NONE) {
		DBG("media_packet_get_format failed");
	}
	if (media_format_get_video_info(dst_fmt, &dst_mimetype, &dst_width,
	                                &dst_height, &dst_avg_bps, &dst_max_bps) != MEDIA_FORMAT_ERROR_NONE) {
		DBG("media_format_get_video_info failed");
	}

	DBG("Result packet dst_width =%d , dst_height =%d ,", dst_width, dst_height);
	ret = media_packet_get_buffer_size(*dst, &size);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		DBG("media_packet_get_buffer_size failed ret =%d", ret);
	}

	ret = media_packet_get_buffer_data_ptr(*dst, &src_ptr);
	if (ret != MEDIA_PACKET_ERROR_NONE) {
		DBG("media_packet_get_buffer_data_ptr failed");
	}

	if (view->transformtype == CAM_TRANSFORM_CROP) {
		unsigned char *buffer = NULL ;
		unsigned char *finalBuffer = NULL;
		int bufferSize = 0 ;
		unsigned int finalBufferSize = 0;

		ret = image_util_encode_jpeg_to_memory(src_ptr,  dst_width, dst_height, IMAGE_UTIL_COLORSPACE_RGB888, 100, &buffer, (unsigned int *)&bufferSize);
		if (ret == IMAGE_UTIL_ERROR_NONE) {
			_main_view_copy_exifdata_to_buffer(buffer, bufferSize, &finalBuffer, &finalBufferSize);
			IF_FREE(buffer);
			FILE *file = fopen(view->filename, "w+");
			if (!file) {
				DBG("Failed to open file");
				IF_FREE(finalBuffer);
				return;
			}

			size = fwrite(finalBuffer, finalBufferSize, 1, file);
			WARN_IF(size != 1, "failed to write file");
			fclose(file);
			file = NULL;
			IF_FREE(finalBuffer);

			_main_file_register(view->filename);
			REMOVE_TIMER(view->crop_timer);
			view->crop_timer = ecore_timer_add(0.01, _main_view_send_result_after_transform, (void *)view);
		} else {
			DBG("image_util_encode_jpeg failed ret = %d", ret);
		}
	}
	media_packet_destroy(*dst);
	endfunc
}

int media_packet_finalize_cb_img_util(media_packet_h packet, int error_code, void *user_data)
{
	startfunc
	return MEDIA_PACKET_FINALIZE;
}

static Eina_Bool cam_utils_check_battery_critical_low(void)
{
	int err = 0;
	device_battery_level_e level_status = -1;
	Eina_Bool ret = FALSE;

	err = device_battery_get_level_status(&level_status);
	if (err != DEVICE_ERROR_NONE) {
		ERR("device_battery_get_level_status fail - [%d]", err);
	}

	DBG("level_status = [%d]", level_status);
	if ((level_status == DEVICE_BATTERY_LEVEL_EMPTY) || (level_status == DEVICE_BATTERY_LEVEL_CRITICAL)) {
		ret = TRUE;
	}

	return ret;
}

static void __cam_app_battery_level_changed_cb(device_callback_e type, void *value, void *user_data)
{
	main_view *view = (main_view *)user_data;
	RETM_IF(!view, "main_view is NULL");

	device_battery_level_e battery_level = (device_battery_level_e)value;
	camera_state_e cur_state = CAMERA_STATE_NONE;
	DBG("battery_level is [%d]", battery_level);

	int res = camera_get_state(view->camera, &cur_state);
	DBG("current camera state= %d", cur_state);

	if (battery_level <= DEVICE_BATTERY_LEVEL_CRITICAL) {
		view->battery_status = LOW_BATTERY_CRITICAL_STATUS;
		_main_view_stop_camera_preview(view->camera);
		view->low_battery_layout = _main_view_load_edj(view->cameraview_layout, view->edje_path, "battery_low_layout");
		elm_object_part_content_set(view->cameraview_layout, "battery_low", view->low_battery_layout);
		elm_object_domain_translatable_part_text_set(view->low_battery_layout, "low_text", CAM_PACKAGE, "IDS_CAM_TPOP_BATTERY_POWER_LOW");
	} else if (battery_level == DEVICE_BATTERY_LEVEL_LOW) {
		view->battery_status = LOW_BATTERY_WARNING_STATUS;
		elm_object_part_content_unset(view->cameraview_layout, "battery_low");
		if (CAMERA_ERROR_NONE == res) {
			if (cur_state != CAMERA_STATE_PREVIEW) {
				if (!_main_view_start_camera_preview(view->camera)) {
					ERR("Failed start preview");
				}
			}
		}
		if (view->low_battery_layout != NULL) {
			evas_object_del(view->low_battery_layout);
		}
	} else {
		view->battery_status = NORMAL_BATTERY_STATUS;
		elm_object_part_content_unset(view->cameraview_layout, "battery_low");
		if (view->low_battery_layout != NULL) {
			evas_object_del(view->low_battery_layout);
		}
		if (CAMERA_ERROR_NONE == res) {
			if (cur_state != CAMERA_STATE_PREVIEW) {
				if (!_main_view_start_camera_preview(view->camera)) {
					ERR("Failed start preview");
				}
			}
		}
	}
}

void _main_view_media_create(char * filename, CamTransformType transformtype)
{
	startfunc
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "main_view is NULL");
	const image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
	unsigned char *img_source = NULL;
	int ret = 0;
	int width = 0, height = 0;
	unsigned int size_decode = 0;
	uint64_t size = 0;
	void* src_ptr;
	DBG("file name is %s", filename);
	if (pkt) {
		DBG("Destroy the media packet");
		media_packet_destroy(pkt);
	}

	if (handle) {
		ret = image_util_transform_destroy(handle);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			DBG("image_util_transform_destroy failed %d ", ret);
		}
	}

	ret = image_util_decode_jpeg(filename, colorspace, &img_source, &width, &height, &size_decode);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		DBG("image_util_decode_jpeg failed %d ", ret);
	}
	DBG("Decoded jpeg buffer width =%d ,height = %d ,size_decode = %d", width, height, size_decode);
	if (media_format_create(&fmt) == MEDIA_FORMAT_ERROR_NONE) {
		ret = media_format_set_video_mime(fmt,  MEDIA_FORMAT_RGB888);
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			DBG("media_format_set_video_mime failed %d ", ret);
		}
		ret = media_format_set_video_width(fmt, width);
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			DBG("media_format_set_video_width failed %d ", ret);
		}
		ret = media_format_set_video_height(fmt, height);
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			DBG("media_format_set_video_height failed %d ", ret);
		}
		ret = media_format_set_video_avg_bps(fmt, 2000000);
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			DBG("media_format_set_video_height failed %d ", ret);
		}
		ret = media_format_set_video_max_bps(fmt, 15000000);
		if (ret != MEDIA_FORMAT_ERROR_NONE) {
			DBG("media_format_set_video_height failed %d ", ret);
		}
		ret = media_packet_create_alloc(fmt, (media_packet_finalize_cb) media_packet_finalize_cb_img_util, img_source, &pkt);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			DBG("media_packet_create_alloc failed %d ", ret);
		}
		media_format_unref(fmt);
		ret = media_packet_set_buffer_size(pkt, size_decode);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			DBG("media_packet_get_buffer_size failed %d ", ret);
		}
		ret = media_packet_get_buffer_size(pkt, &size);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			DBG("media_packet_get_buffer_size failed %d ", ret);
		}
		ret = media_packet_get_buffer_data_ptr(pkt, &src_ptr);
		if (ret != MEDIA_PACKET_ERROR_NONE) {
			DBG("media_packet_get_buffer_data_ptr failed %d ", ret);
		}
		memcpy(src_ptr, img_source, size);
		DBG("Media packet creation success,image_util_transform_create start");

		ret = image_util_transform_create(&handle);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			DBG("image_util_transform_create failed %d ", ret);
		}
		ret = image_util_transform_set_hardware_acceleration(handle, false);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			DBG("image_util_transform_set_hardware_acceleration failed");
		}
		if (transformtype == CAM_TRANSFORM_CROP) {
			DBG("image_util_transform_set_crop_area");
			if (CAM_WIDTH == 1600) {
				DBG("RESOLUTION 1600X1200");
				//ret = image_util_transform_set_crop_area(handle, 0, 490, 1200, 1110);
				ret = image_util_transform_set_crop_area(handle, 500, 0, 1100, 1200);//490, 0, 1110, 1200);  // 1100,500
			} else {
				DBG("RESOLUTION 640X480");
				ret = image_util_transform_set_crop_area(handle, 205, 0, 435, 480);
			}
			if (ret != IMAGE_UTIL_ERROR_NONE) {
				DBG("image_util_transform_set_crop_area failed %d ", ret);
			}
		} else {
			DBG("image_util_transform_set_RESIZE");
			ret = image_util_transform_set_resolution(handle, CAM_WIDTH, CAM_HEIGHT);
		}
		DBG("image_util_transform_run START");
		ret = image_util_transform_run(handle, pkt, (image_util_transform_completed_cb)_main_view_transform_completed, handle);//(void*)img_source);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			DBG("image_util_transform_run failed");
		}
	} else {
		DBG("media_format_create failed");
	}
	endfunc
}

Evas_Object *_main_view_load_edj(Evas_Object *parent, const char *file,
		const char *group)
{
	Evas_Object *eo = NULL;
	int r = 0;

	eo = elm_layout_add(parent);
	if (eo) {
		r = elm_layout_file_set(eo, file, group);
		if (!r) {
			evas_object_del(eo);
			eo = NULL;
			return NULL;
		}

		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(eo);
	}

	return eo;
}

void main_view_camera_view_add(main_view *view)
{
	startfunc
	RETM_IF(!view, "main_view is NULL");

	elm_object_part_content_unset(view->layout, "main_view");
	if (view->recorderview_layout) {
		evas_object_del(view->recorderview_layout);
	}
	double scale = elm_config_scale_get();
	if (scale == 2.6) {
		view->cameraview_layout = _main_view_load_edj(view->layout, view->edje_path, "camera_layout");
	} else {
		view->cameraview_layout = _main_view_load_edj(view->layout, view->edje_path, "camera_layout_WVGA");
	}

	elm_object_part_content_set(view->layout, "main_view", view->cameraview_layout);
	_main_view_register_cbs(view);
}

Evas_Object *main_view_add(Evas_Object *navi, ui_gadget_h ug_handle, unsigned long long size_limit)
{
	startfunc
	main_view *view = calloc(1, sizeof(main_view));
	RETVM_IF(!view, NULL, "failed to allocate main_view");
	char *id = NULL;
	view->ug_handle = ug_handle;
	view->self_portrait = FALSE;
	app_get_id(&id);
	set_edje_path(view);
	//bindtextdomain(CAM_PACKAGE, CAM_LOCALESDIR);
	DBG("app id = %s", id);
	if (id != NULL) {
		if ((!strcmp(id, "org.tizen.message")) || (!strcmp(id, "msg-composer-efl"))) {
			view->is_size_limit = TRUE;
		} else {
			view->is_size_limit = FALSE;
		}
		free(id);
		id = NULL;
	}
	view->layout = ui_utils_layout_add(navi, NULL, view);
	elm_layout_file_set(view->layout, view->edje_path, "main_layout");
	evas_object_size_hint_align_set(view->layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	view->preview_canvas = evas_object_image_filled_add(evas_object_evas_get(view->layout));
	if (!view->preview_canvas) {
		ERR("failed to create preview_canvas");
		evas_object_del(view->layout);
		return NULL;
	}

	elm_object_part_content_set(view->layout, "elm.swallow.content", view->preview_canvas);
	double scale = elm_config_scale_get();
	if (scale == 2.6) {
		view->cameraview_layout = _main_view_load_edj(view->layout, view->edje_path, "camera_layout");
	} else {
		view->cameraview_layout = _main_view_load_edj(view->layout, view->edje_path, "camera_layout_WVGA");
	}
	elm_object_part_content_set(view->layout, "main_view", view->cameraview_layout);

	_main_view_set_data(view);
	view->camera_enabled = _main_view_init_camera(view, CAMERA_DEVICE_CAMERA0);
	if (view->camera_enabled) {
		if (!_main_view_start_camera_preview(view->camera)) {
			ERR("Failed start preview");
		}
	}
	if (error_type == CAMERA_ERROR_SOUND_POLICY_BY_CALL) {
		ERR("_main_view_start_preview failed");
		//edje_object_part_text_set(_EDJ(view->cameraview_layout), "content_text", "IDS_CAM_POP_UNABLE_TO_START_CAMERA_DURING_CALL");
		view->low_battery_layout = _main_view_load_edj(view->cameraview_layout, view->edje_path, "battery_low_layout");
		elm_object_part_content_set(view->cameraview_layout, "battery_low", view->low_battery_layout);
		elm_object_domain_translatable_part_text_set(view->low_battery_layout, "low_text", CAM_PACKAGE, "IDS_CAM_POP_UNABLE_TO_START_CAMERA_DURING_CALL");
		// _main_view_show_warning_popup(navi, _error, "Unable to open camera during call", _ok, view);
	} else if (!view->camera_enabled && !cam_utils_check_battery_critical_low()) {
		ERR("_main_view_start_preview failed");
		_main_view_show_warning_popup(navi, _error, _camera_init_failed, _ok, view);
	} else {
		// edje_object_part_text_set(_EDJ(view->cameraview_layout), "content_text", "");
		elm_object_part_content_unset(view->cameraview_layout, "battery_low");
		evas_object_del(view->low_battery_layout);
	}

	_main_view_register_cbs(view);
	camera_error_e e;
	e = camera_set_interrupted_cb(view->camera, __cam_interrupted_cb, (void *)view);
	if (e != CAMERA_ERROR_NONE) {
		DBG("camera_set_interrupted_cb failed - code[%x]", e);
		return FALSE;
	}
	_main_view_set_data(view);
	evas_object_show(view->layout);
	if (view->target_direction == CAM_TARGET_DIRECTION_PORTRAIT) {
		view->cam_view_type = CAM_COMPACT_VIEW;
	} else {
		view->cam_view_type = CAM_FULL_VIEW;
	}

	if (size_limit != 0) {
		view->size_limit = size_limit;
	} else if (view->is_size_limit == TRUE) {
		view->size_limit = CAM_REC_MMS_MAX_SIZE;
	} else {
		view->size_limit = CAM_REC_NORMAL_MAX_SIZE;
	}

	DBG("view->size_limit set to = %llu", view->size_limit);

	device_add_callback(DEVICE_CALLBACK_BATTERY_LEVEL, __cam_app_battery_level_changed_cb, view);
	endfunc
	return view->layout;
}

void _main_view_destroy()
{
	startfunc
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "main_view is NULL");
	int ret = 0;
	_main_view_stop_camera_preview(view->camera);

	if (grecord != NULL) {
		recorder_destroy(grecord);
		grecord = NULL;
	}
	if (pkt) {
		media_packet_destroy(pkt);
		DBG("Destroy the packet if already available");
	} else {
		DBG("packet is not available to destroy");
	}
	if (handle) {
		ret = image_util_transform_destroy(handle);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			DBG("image_util_transform_destroy failed");
		} else {
			DBG("image_util_transform_destroy success");
		}
	}
	if (view->camera != NULL) {
		camera_destroy(view->camera);
		view->camera = NULL;
	}

	evas_object_smart_callback_del(view->layout, EVENT_PAUSE, _main_view_pause_cb);
	evas_object_smart_callback_del(view->layout, EVENT_RESUME, _main_view_resume_cb);

	elm_object_part_content_unset(view->layout, "main_view");
	if (view->cameraview_layout != NULL) {
		evas_object_del(view->cameraview_layout);
	}

	if (view->recorderview_layout != NULL) {
		evas_object_del(view->recorderview_layout);
	}

	if (view->edje_path != NULL) {
		free(view->edje_path);
		view->edje_path = NULL;
	}

	//free(view);
	IF_FREE(exifData);
	_main_view_set_data(NULL);
	endfunc
}

void _main_view_set_data(void *data)
{
	main_view *view = (main_view *)data;
	/*RETVM_IF(!view, NULL, "failed to allocate main_view");*/
	viewhandle = view;
}

void*  _main_view_get_data()
{
	if (viewhandle) {
		return viewhandle;
	}
	return NULL;
}
static void _main_view_recorder_set_recording_size(main_view *view)
{
	RETM_IF(!view, "main_view is NULL");

	char str[32] = "";
	double rec_filesize_in_mega = 0.0;

	if (view->rec_filesize < 1024) {
		snprintf(str, sizeof(str), "%lldK", view->rec_filesize);
	} else {
		rec_filesize_in_mega = (double)view->rec_filesize / (double)1024;
		snprintf(str, sizeof(str), "%.1fM", rec_filesize_in_mega);
	}
	if (view->is_size_limit == TRUE) {
		char str1[10] = "";
		double pbar_position = 0.0;
		double size_limit_in_mega = 0.0;

		if (view->size_limit < 1024) {
			snprintf(str1, sizeof(str1), "%lldK", view->size_limit);
		} else {
			size_limit_in_mega = (double)view->size_limit / (double)1024;
			snprintf(str1, sizeof(str1), "%.1fM", size_limit_in_mega);
		}

		edje_object_part_text_set(_EDJ(view->progressbar_layout), "right_text_val", str1);
		edje_object_part_text_set(_EDJ(view->progressbar_layout), "left_text_val", str);

		DBG("view->rec_filesize = %lld", view->rec_filesize);
		pbar_position = view->rec_filesize / (double)view->size_limit;
		elm_progressbar_value_set(view->progressbar, pbar_position);
	} else {
		edje_object_part_text_set(_EDJ(view->recorderview_layout), "recording_size", str);
	}
}

void _main_view_recorder_update_time(main_view *view)
{
	RETM_IF(!view, "main_view is NULL");
	RETM_IF(!grecord, "Invalid recorder handle");

	int state = 0;
	int ret = RECORDER_ERROR_NONE;

	ret = recorder_get_state(grecord, (recorder_state_e *)&state);
	if (ret != RECORDER_ERROR_NONE) {
		ERR("recorder_get_state error code %d", ret);
	}

	if (state == RECORDER_STATE_RECORDING) {
		_main_view_recorder_rec_icon_update(view);
		_main_view_recorder_set_recording_time(view);
		_main_view_recorder_set_recording_size(view);
	} else {
		DBG("recorder state = %d && file size = %lld", state, view->rec_filesize);
	}
}
/*
static void _main_view_recording_status_cb(unsigned long long elapsed_time, unsigned long long file_size, void *user_data)
{
	main_view *view = (main_view*)user_data;
	unsigned long long elapsed = elapsed_time / 1000;

	if (view->rec_elapsed < elapsed) {
		view->rec_elapsed = elapsed;
		_main_view_recorder_update_time(view);
	}

	if (view->rec_filesize < file_size) {
		view->rec_filesize = file_size;
	}
}
*/
static void _main_view_recorder_set_recording_time(main_view *view)
{
	RETM_IF(!view, "main_view is NULL");

	char time_text[TIME_FORMAT_MAX_LEN] = "";
	unsigned long long rec_time = view->rec_elapsed;

	if (view->rec_elapsed < SECONDS_IN_HOUR) {
		snprintf(time_text, sizeof(time_text), "%" CAM_TIME_FORMAT2 "", CAM_TIME_ARGS2(rec_time));
	} else {
		snprintf(time_text, sizeof(time_text), "%" CAM_TIME_FORMAT "", CAM_TIME_ARGS(rec_time));
	}

	edje_object_part_text_set(_EDJ(view->recorderview_layout), "recording_time", time_text);
}

static void _main_view_recorder_rec_icon_update(main_view *view)
{
	RETM_IF(!view, "main_view is NULL");
	RETM_IF(!grecord, "Invalid recorder handle");

	int state = 0;
	int ret = RECORDER_ERROR_NONE;

	ret = recorder_get_state(grecord, (recorder_state_e *)&state);
	if (ret != RECORDER_ERROR_NONE) {
		ERR("recorder_get_state error code %d", ret);
	}

	if (state == RECORDER_STATE_PAUSED) {
		if (!(view->pause_time % 2) == 0) {
			edje_object_signal_emit(_EDJ(view->recording_icon), "recording_icon,pause,portrait", "prog");
		} else {
			edje_object_signal_emit(_EDJ(view->recording_icon), "recording_icon,hide", "prog");
		}
	} else {
		if ((view->rec_elapsed % 2) == 0) {
			edje_object_signal_emit(_EDJ(view->recording_icon), "recording_icon,recording", "prog");
		} else {
			edje_object_signal_emit(_EDJ(view->recording_icon), "recording_icon,hide", "prog");
		}
	}
}

static void _main_view_recorder_rec_icon_create(main_view *view)
{
	RETM_IF(!view, "main_view is NULL");

	if (view->recording_icon) {
		elm_object_part_content_unset(view->recorderview_layout, "recording_icon");
		evas_object_del(view->recording_icon);
	}
	double scale = elm_config_scale_get();
	if (scale == 2.6) {
		view->recording_icon = _main_view_load_edj(view->recorderview_layout, view->edje_path, "recording_icon");
	} else {
		view->recording_icon = _main_view_load_edj(view->recorderview_layout, view->edje_path, "recording_icon_WVGA");
	}
	elm_object_part_content_set(view->recorderview_layout, "recording_icon", view->recording_icon);
	_main_view_recorder_rec_icon_update(view);
}

static void __recording_view_progressbar_create(main_view *view)
{
	RETM_IF(!view, "main_view is NULL");
	double scale = elm_config_scale_get();
	if (scale == 2.6) {
		view->progressbar_layout = _main_view_load_edj(view->recorderview_layout, view->edje_path, "progressbar_layout");
	} else {
		view->progressbar_layout = _main_view_load_edj(view->recorderview_layout, view->edje_path, "progressbar_layout_WVGA");
	}
	RETM_IF(view->progressbar_layout == NULL, "edj load failed");
	elm_object_part_content_set(view->recorderview_layout, "progressbar_area", view->progressbar_layout);

	view->progressbar = elm_progressbar_add(view->progressbar_layout);
	RETM_IF(view->progressbar == NULL, "elm_progressbar_add failed");
	elm_progressbar_horizontal_set(view->progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(view->progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(view->progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_content_set(view->progressbar_layout, "progressbar", view->progressbar);
	evas_object_show(view->progressbar);

	char str[10] = "";
	char str2[10] = "";
	double size_limit_in_mega = 0.0;
	double rec_filesize_in_mega = 0.0;

	if (view->size_limit < 1024) {
		snprintf(str, sizeof(str), "%lldK", view->size_limit);
	} else {
		size_limit_in_mega = (double)view->size_limit / (double)1024;
		snprintf(str, sizeof(str), "%.1fM", size_limit_in_mega);
	}

	if (view->rec_filesize < 1024) {
		snprintf(str2, sizeof(str2), "%lldK", view->rec_filesize);
	} else {
		rec_filesize_in_mega = (double)view->rec_filesize / (double)1024;
		snprintf(str2, sizeof(str2), "%.1fM", rec_filesize_in_mega);
	}

	edje_object_part_text_set(_EDJ(view->progressbar_layout), "right_text_val", str);
	edje_object_part_text_set(_EDJ(view->progressbar_layout), "left_text_val", str2);
}

void _main_view_recorder_view_add(main_view *view)
{
	startfunc
	RETM_IF(!view, "main_view is NULL");

	elm_object_part_content_unset(view->layout, "main_view");
	evas_object_del(view->cameraview_layout);
	_main_view_set_target_direction(view->target_direction);
	double scale = elm_config_scale_get();
	if (scale == 2.6) {
		view->recorderview_layout = _main_view_load_edj(view->layout, view->edje_path, "recorder_layout");
	} else {
		view->recorderview_layout = _main_view_load_edj(view->layout, view->edje_path, "recorder_layout_WVGA");
	}
	elm_object_part_content_set(view->layout, "main_view", view->recorderview_layout);
	elm_object_signal_callback_add(view->recorderview_layout, "rec_stop_button_clicked", "*",
	                               _main_view_recorder_stop_button_cb, view);
	elm_object_signal_callback_add(view->recorderview_layout, "rec_pause_button_clicked", "*",
	                               _main_view_recorder_pause_button_cb, view);
	elm_object_signal_callback_add(view->recorderview_layout, "rec_resume_button_clicked", "*",
	                               _main_view_recorder_resume_button_cb, view);

	_main_view_recorder_rec_icon_create(view);
	_main_view_recorder_set_recording_time(view);
	if (view->is_size_limit == TRUE) {
		__recording_view_progressbar_create(view);
	}
	evas_object_show(view->layout);
	endfunc
}
/*
static Eina_Bool _main_view_create_recorder(camera_h camera)
{
	startfunc
	recorder_error_e re = RECORDER_ERROR_NONE;
	re = recorder_create_videorecorder(camera, &grecord);
	if (re != RECORDER_ERROR_NONE) {
		ERR( "[ERROR] recorder_create - error(%d)", re);
		recorder_destroy(grecord);
		grecord = NULL;
		return EINA_FALSE;
	}
	endfunc
	return EINA_TRUE;
}
*/
static Eina_Bool _main_view_start_camera_preview(camera_h camera)
{
	startfunc
	main_view *view = (main_view *)_main_view_get_data();
	RETVM_IF(!view, EINA_FALSE, "main_view is NULL");
	camera_state_e cur_state = CAMERA_STATE_NONE;
	int width = 0;
	int height = 0;
	if (cam_utils_check_battery_critical_low()) {
		ERR("Battery critically low");
		view->low_battery_layout = _main_view_load_edj(view->cameraview_layout, view->edje_path, "battery_low_layout");
		elm_object_part_content_set(view->cameraview_layout, "battery_low", view->low_battery_layout);
		elm_object_domain_translatable_part_text_set(view->low_battery_layout, "low_text", CAM_PACKAGE, "IDS_CAM_TPOP_BATTERY_POWER_LOW");
		return EINA_FALSE;
	}

	int res = camera_get_state(camera, &cur_state);
	DBG("current camera state= %d", cur_state);
	if (CAMERA_ERROR_NONE == res) {
		if (cur_state != CAMERA_STATE_CAPTURED) {
			camera_start_focusing(camera, TRUE);
			res = camera_attr_enable_tag(camera, TRUE);
			if (res != CAMERA_ERROR_NONE) {
				ERR("camera_attr_is_enabled_tag failed - code[%x]", res);
				return EINA_FALSE;
			}
			res = camera_set_capture_resolution(camera, CAM_WIDTH, CAM_HEIGHT);
			if (res != CAMERA_ERROR_NONE) {
				ERR("camera_set_capture_resolution failed - code[%x]", res);
				return EINA_FALSE;
			}

			res = camera_get_recommended_preview_resolution(camera, &width, &height);
			if (res != CAMERA_ERROR_NONE) {
				ERR("camera_get_recommended_preview_resolution failed - code[%x]", res);
				return EINA_FALSE;
			}

			ERR("camera_set_preview_resolution width = %d - height[%d]", width, height);
			res = camera_set_preview_resolution(camera, width, height);
			if (res != CAMERA_ERROR_NONE) {
				ERR("camera_set_preview_resolution failed - code[%x]", res);
				return EINA_FALSE;
			}
		}
		if (cur_state != CAMERA_STATE_PREVIEW) {
			res = camera_start_preview(camera);
			if (res != CAMERA_ERROR_NONE) {
				error_type = res;
				ERR("unable to open during call");
			}  else {
				error_type = 0;
			}
			_main_view_util_lcd_lock();
			return EINA_TRUE;
		}
	} else {
		ERR("Cannot get camera state. Error: %d", res);
	}
	endfunc
	return EINA_FALSE;
}

static Eina_Bool _main_view_stop_camera_preview(camera_h camera)
{
	startfunc
	camera_state_e cur_state = CAMERA_STATE_NONE;
	int res = camera_get_state(camera, &cur_state);
	if (CAMERA_ERROR_NONE == res) {
		if (cur_state == CAMERA_STATE_PREVIEW) {
			camera_stop_preview(camera);
			_main_view_util_lcd_unlock();
			return EINA_TRUE;
		}
	} else {
		ERR("Cannot get camera state. Error: %d", res);
	}
	endfunc
	return EINA_FALSE;
}

static Eina_Bool _main_view_init_camera(main_view *view, int camera_type)
{
	startfunc
	RETVM_IF(!view, EINA_FALSE, "main_view is NULL");
	view->device_type = camera_type;
	int result = camera_create(camera_type, &view->camera);
	if (CAMERA_ERROR_NONE == result) {
		if (view->preview_canvas) {
			result = camera_set_display(view->camera, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(view->preview_canvas));
			if (result != CAMERA_ERROR_NONE) {
				ERR("camera_set_display failed -- code[%x]", result);
			}
			if (CAMERA_ERROR_NONE == result) {
				camera_set_display_mode(view->camera, CAMERA_DISPLAY_MODE_CROPPED_FULL);
				camera_set_display_visible(view->camera, true);
				camera_attr_set_stream_flip(view->camera, CAMERA_FLIP_NONE);

				if (camera_type == CAMERA_DEVICE_CAMERA0) {
					result = camera_set_display_flip(view->camera, CAMERA_FLIP_NONE);
				} else {
					result = camera_set_display_flip(view->camera, CAMERA_FLIP_VERTICAL);
				}
				if (CAMERA_ERROR_NONE != result) {
					ERR("Failed to set display flip");
				}
#ifndef CAMERA_MACHINE_I686
				result = camera_set_display_rotation(view->camera, CAMERA_ROTATION_270);
#else
				result = camera_set_display_rotation(view->camera, CAMERA_ROTATION_NONE);
#endif
				if (result != CAMERA_ERROR_NONE) {
					ERR("camera_set_display_rotation failed - code[%x]", result);
				}

				//	return _main_view_start_camera_preview(view->camera);
			}
		}
	}
	endfunc
	return !result;
}
/*
static Eina_Bool _stop_cb(void *data)
{
	main_view *view = data;
	view->timer_count++;

	if (view->timer_count == 1) {
		int error_code = 0;
		error_code = recorder_commit(grecord);

		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_commit error code %d", error_code);
		}

		error_code = recorder_unprepare(grecord);
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_unprepare error code %d", error_code);
		}

		_main_file_register(view->filename);

		if (view->pause_state == FALSE) {
			_main_view_ug_send_result(view, view->filename);
		}

		error_code = recorder_destroy(grecord);
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_destroy error code %d", error_code);
		}
		grecord = NULL;
		main_view_camera_view_add(view);
		toast_popup_create(view, "Maximum recording size reached.", NULL);

		if (view->pause_state == FALSE) {
			 _main_view_start_camera_preview (view->camera);
		}
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}

static void __recording_limit_reached_cb(recorder_recording_limit_type_e type, void *user_data)
{
	DBG("recording limit reached - [%d]", type);
	RETM_IF(!user_data, "data is NULL");
	RETM_IF(!grecord, "Invalid recorder handle");

	main_view *view = user_data;

	_main_view_recorder_update_time(view);

	view->timer_count = 0;
	Ecore_Timer *stop_cb_delay;
	stop_cb_delay = ecore_timer_add(1.0, _stop_cb, view);
}

static Eina_Bool _main_view_init_recorder(main_view *view)
{
	startfunc
	DBG("_main_view_init_recorder");
	RETVM_IF(!grecord, EINA_FALSE, "Invalid recorder handle");

	int error_code = 0;
	Eina_Bool ret = EINA_TRUE;
	int video_codec = 0;
	int audio_codec = 0;
	int sample_rate = 0;
	int file_format = RECORDER_FILE_FORMAT_MP4;
	int channel = 0;
	int a_bitrate = 0;
	int v_bitrate = 0;
	int res = 0;
	size_t size = _main_view_get_file_path(view->filename, sizeof(view->filename), CAM_FILE_VIDEO);
	RETVM_IF(0 == size, EINA_FALSE, "failed to get file path")
	DBG("%s", view->filename);

	if (view->is_size_limit == TRUE) {
		video_codec = RECORDER_VIDEO_CODEC_H263;
		audio_codec = RECORDER_AUDIO_CODEC_AMR;
		sample_rate = 8000;
		file_format = RECORDER_FILE_FORMAT_3GP;
		channel = 1;
		a_bitrate = 12200;
		v_bitrate = 96100;
		res = CAM_RESOLUTION_QCIF;
	} else {
		video_codec = RECORDER_VIDEO_CODEC_MPEG4 ;
		audio_codec = RECORDER_AUDIO_CODEC_AAC;
		sample_rate = 48000;
		file_format = RECORDER_FILE_FORMAT_MP4;
		channel = 2;
		a_bitrate = 288000;
		v_bitrate = 3078000;
		res = CAM_RESOLUTION_VGA;
		DBG(" video_codec = %d ,RECORDER_VIDEO_CODEC_MPEG4 = %d" , video_codec, RECORDER_VIDEO_CODEC_MPEG4);
		DBG("audio_codec = %d ,RECORDER_AUDIO_CODEC_AAC = %d" , audio_codec, RECORDER_AUDIO_CODEC_AAC);
		DBG("file_format = %d ,RECORDER_FILE_FORMAT_MP4 = %d" , file_format, RECORDER_FILE_FORMAT_MP4);
	}
	if (view->is_size_limit == TRUE) {
#ifdef CAMERA_MACHINE_I686
		error_code = camera_attr_set_preview_fps(view->camera, 30);
#else
		error_code = camera_attr_set_preview_fps(view->camera, 25);
#endif
		if (error_code != CAMERA_ERROR_NONE) {
			ERR("camera_attr_set_preview_fps failed - code[%x]", ret);
			ret =  EINA_FALSE;
		}
		error_code = recorder_attr_set_recording_motion_rate(grecord, 1.0);
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_attr_set_recording_motion_rate error code %d", error_code);
			ret = EINA_FALSE;
		}
		error_code = camera_set_capture_resolution(view->camera, 640, 480);
		if (error_code != CAMERA_ERROR_NONE) {
			ERR("camera_set_capture_resolution failed - code[%x]", ret);
			ret =  EINA_FALSE;
		}
		error_code = camera_set_preview_resolution(view->camera, 176, 144);
		if (error_code != CAMERA_ERROR_NONE) {
			ERR("camera_set_preview_resolution failed - code[%x]", ret);
			ret =  EINA_FALSE;
		}
		error_code =recorder_set_video_resolution(grecord, CAM_RESOLUTION_W(res), CAM_RESOLUTION_H(res));
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_set_video_resolution error code %d", error_code);
			ret = EINA_FALSE;
		}
#ifndef CAMERA_MACHINE_I686
		error_code = camera_attr_enable_video_stabilization(view->camera, FALSE);
		if (error_code != CAMERA_ERROR_NONE) {
			ERR("camera_attr_enable_video_stabilization failed - code[%x]", error_code);
			ret =  EINA_FALSE;
		}
#endif
		error_code = camera_set_capture_format(view->camera, CAMERA_PIXEL_FORMAT_JPEG);
		if (error_code != CAMERA_ERROR_NONE) {
			ERR("camera_set_capture_format failed - code[%x]", error_code);
			ret =  EINA_FALSE;
		}
#ifdef CAMERA_MACHINE_I686
		error_code = camera_set_preview_format(view->camera, CAMERA_PIXEL_FORMAT_I420);
#else
		error_code = camera_set_preview_format(view->camera, CAMERA_PIXEL_FORMAT_NV12);
#endif
		if (error_code != CAMERA_ERROR_NONE) {
			ERR("camera_set_preview_format failed - code[%x]", error_code);
			ret =  EINA_FALSE;
		}
	}
	error_code = recorder_set_video_encoder(grecord, video_codec);
	if (error_code != RECORDER_ERROR_NONE) {
		ERR("recorder_set_video_encoder error code %d", error_code);
		ret = EINA_FALSE;
	}

	error_code = recorder_set_audio_encoder(grecord, audio_codec);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_set_video_encoder error code %d", error_code);
		ret = EINA_FALSE;
	}

	error_code = recorder_attr_set_audio_samplerate(grecord, sample_rate);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_attr_set_audio_samplerate error code %d", error_code);
		ret = EINA_FALSE;
	}

	error_code = recorder_attr_set_audio_channel(grecord, channel);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_attr_set_audio_channel error code %d", error_code);
		ret = EINA_FALSE;
	}

	//Set the bitrate of the video encoder
	error_code = recorder_attr_set_video_encoder_bitrate(grecord, v_bitrate);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_attr_set_video_encoder_bitrate error code %d", error_code);
		ret = EINA_FALSE;
	}

	error_code = recorder_attr_set_audio_encoder_bitrate(grecord, a_bitrate);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_attr_set_audio_encoder_bitrate error code %d", error_code);
		ret = EINA_FALSE;
	}

	//Set the recording file format
	error_code = recorder_set_file_format(grecord, file_format);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_set_file_format error code %d", error_code);
		ret = EINA_FALSE;
	}

	// Set the file path to store the recorded data
	error_code = recorder_set_filename(grecord, view->filename);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_set_filename error code %d", error_code);
		ret = EINA_FALSE;
	}

	int size_limit = view->size_limit;
	error_code = recorder_attr_set_size_limit(grecord, size_limit);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_attr_set_size_limit failed - code[%x]", error_code);
		return EINA_FALSE;
	}

	error_code = recorder_set_recording_limit_reached_cb(grecord, __recording_limit_reached_cb, view);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_set_recording_limit_reached_cb failed - code[%x]", error_code);
		return EINA_FALSE;
	}

	endfunc
	return ret;
}
*/
static void _main_view_register_cbs(main_view *view)
{
	/*evas_object_smart_callback_add(view->layout, EVENT_BACK, _main_view_back_cb, view);*/
	evas_object_smart_callback_add(view->layout, EVENT_PAUSE, _main_view_pause_cb, view);
	evas_object_smart_callback_add(view->layout, EVENT_RESUME, _main_view_resume_cb, view);
	elm_object_signal_callback_add(view->cameraview_layout, "shutter_button_clicked", "*",
	                               _main_view_shutter_button_cb, view);
	/*elm_object_signal_callback_add(view->cameraview_layout, "video_button_clicked", "*",
	        _main_view_video_button_cb, view);*/
#ifndef CAMERA_MACHINE_I686
	elm_object_signal_callback_add(view->cameraview_layout, "change_camera_button_clicked", "*",
	                               _main_view_switch_camera_button_cb, view);
#endif
}
/*
static void _main_view_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    RETM_IF(!data, "data is NULL");
    main_view *view = data;
}*/

static void _main_view_pause_cb(void *data, Evas_Object *obj, void *event_info)
{
	RETM_IF(!data, "data is NULL");
	main_view *view = data;

	_main_view_stop_camera_preview(view->camera);
}

static void _main_view_resume_cb(void *data, Evas_Object *obj, void *event_info)
{
	RETM_IF(!data, "data is NULL");
	main_view *view = data;

	if (!_main_view_start_camera_preview(view->camera)) {
		ERR("Failed start preview");
		return;
	}
}

bool _main_view_check_phone_dir()
{
	DIR *internal_dcim_dir = NULL;
	DIR *internal_file_dir = NULL;
	int ret = -1;

	internal_dcim_dir = opendir(INTERNAL_DCIM_PATH);
	if (internal_dcim_dir == NULL) {
		ret = mkdir(INTERNAL_DCIM_PATH, 0777);
		if (ret < 0) {
			DBG("mkdir [%s] failed - [%d]", INTERNAL_DCIM_PATH, errno);
			if (errno != ENOSPC) {
				goto ERROR;
			}
		}
	}

	internal_file_dir = opendir(CAMERA_DIRECTORY);
	if (internal_file_dir == NULL) {
		ret = mkdir(CAMERA_DIRECTORY, 0777);
		if (ret < 0) {
			DBG("mkdir [%s] failed - [%d]", CAMERA_DIRECTORY, errno);
			if (errno != ENOSPC) {
				goto ERROR;
			}
		}
	}

	if (internal_file_dir) {
		closedir(internal_file_dir);
		internal_file_dir = NULL;
	}

	if (internal_dcim_dir) {
		closedir(internal_dcim_dir);
		internal_dcim_dir = NULL;
	}

	return TRUE;

ERROR:
	if (internal_dcim_dir) {
		closedir(internal_dcim_dir);
		internal_dcim_dir = NULL;
	}

	return FALSE;
}

static size_t _main_view_get_file_path(char *file_path, size_t size, CamFileType fileType)
{
	RETVM_IF(!file_path, 0, "file_path is NULL");

	struct tm localtime = {0,};
	time_t rawtime = time(NULL);
	main_view *view = (main_view *)_main_view_get_data();
	RETVM_IF(!view, 0, "view is NULL");

	if (_main_view_check_phone_dir() == FALSE) {
		return 0;
	}
	if (localtime_r(&rawtime, &localtime) == NULL) {
		return 0;
	}
	if (fileType == CAM_FILE_IMAGE) {
		return snprintf(file_path, size, "%s/%s_%04i-%02i-%02i_%02i:%02i:%02i.jpg",
		                CAMERA_DIRECTORY, FILE_PREFIX,
		                localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
		                localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
	} else {
		if (view->is_size_limit == TRUE) {
			return snprintf(file_path, size, "%s/%s_%04i-%02i-%02i_%02i:%02i:%02i.3gp",
			                CAMERA_DIRECTORY, FILE_VIDEO_PREFIX,
			                localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
			                localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
		} else {
			return snprintf(file_path, size, "%s/%s_%04i-%02i-%02i_%02i:%02i:%02i.mp4",
			                CAMERA_DIRECTORY, FILE_VIDEO_PREFIX,
			                localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
			                localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
		}
	}
}

void _main_view_app_pause()
{
	int state = 0;
	int ret = RECORDER_ERROR_NONE;
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "view is NULL");

	view->pause_state = TRUE;
	if (grecord != NULL) {
		ret = recorder_get_state(grecord, (recorder_state_e *)&state);
		if (ret == RECORDER_ERROR_NONE) {
			ERR("recorder_get_state error code %d", ret);
			if ((state == RECORDER_STATE_RECORDING)
			        || (state == RECORDER_STATE_PAUSED)) {
				_main_view_recorder_stop_button_cb(view, NULL, NULL, NULL);
			}
		}
	}
	_main_view_stop_camera_preview(view->camera);

	//edje_object_part_text_set(_EDJ(view->cameraview_layout), "content_text", "");
	elm_object_part_content_unset(view->cameraview_layout, "battery_low");
	evas_object_del(view->low_battery_layout);
	view->pause_state = FALSE;
}

void _main_view_app_resume()
{
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "view is NULL");
	camera_state_e cur_state = CAMERA_STATE_NONE;
	int res = camera_get_state(view->camera, &cur_state);
	if (res < 0) {
		ERR("camera_get_state error code %d", res);
		return;
	}
	if (cur_state != CAMERA_STATE_PREVIEW) {
		_main_view_start_camera_preview(view->camera);
	}
	if (error_type == CAMERA_ERROR_SOUND_POLICY_BY_CALL) {
		ERR("Interrupted by call");
		view->low_battery_layout = _main_view_load_edj(view->cameraview_layout,
		                           view->edje_path,
		                           "battery_low_layout");
		elm_object_part_content_set(view->cameraview_layout, "battery_low",
		                            view->low_battery_layout);
		elm_object_domain_translatable_part_text_set(view->low_battery_layout,
		        "low_text", CAM_PACKAGE, "IDS_CAM_TPOP_BATTERY_POWER_LOW");
	}
}

static void _main_view_ug_send_result(main_view *view, char *file_path)
{
	startfunc
	int ret = 0;
	app_control_h app_control = NULL;
	char **path_array = NULL;

	DBG("result filepath is %s", file_path);
	ret = app_control_create(&app_control);
	if (ret == APP_CONTROL_ERROR_NONE) {
		if (file_path == NULL) {
			ret = app_control_add_extra_data(app_control, "__ATTACH_PANEL_FULL_MODE__", "enable");
			if (ret != APP_CONTROL_ERROR_NONE) {
				LOGD("Enabling Full mode  failed!");
			}
		} else {
			path_array = (char**)calloc(1, sizeof(char *));
			if (path_array == NULL) {
				LOGD("Failed to allocate memory");
				app_control_destroy(app_control);
				return;
			}
			path_array[0] = strdup(file_path);
			ret = app_control_add_extra_data_array(app_control, APP_CONTROL_DATA_PATH, (const char **)path_array, 1);
			ret = app_control_add_extra_data_array(app_control, APP_CONTROL_DATA_SELECTED, (const char **)path_array, 1);
			if (ret != APP_CONTROL_ERROR_NONE) {
				LOGD("Add selected path failed!");
			}
		}
		ret = ug_send_result(view->ug_handle, app_control);
		if (ret < 0) {
			LOGD("ug_send_result failed");
		}
		app_control_destroy(app_control);
		if (path_array != NULL) {
			if (path_array[0] != NULL) {
				free(path_array[0]);
			}
			free(path_array);
		}
		/*ug_destroy_me(view->ug_handle);*/
	} else {
		DBG("Failed to create app control\n");
	}
	endfunc
}

bool _main_file_register(const char *filename)
{
	startfunc
	int err_code = 0;
	media_info_h info = NULL;

	char *register_file = strdup(filename);
	if (register_file == NULL) {
		LOGD("Failed to allocate memory");
	}

	err_code = media_info_insert_to_db(register_file, &info);
	if (err_code != MEDIA_CONTENT_ERROR_NONE) {
		DBG("failed to media_file_register() : [%s], [%d]", register_file, err_code);
		media_info_destroy(info);
		IF_FREE(register_file);
		return FALSE;
	}

	media_info_destroy(info);
	IF_FREE(register_file);

	endfunc
	return TRUE;
}

static void _main_view_capture_cb(camera_image_data_s *image,
		camera_image_data_s *postview, camera_image_data_s *thumbnail, void *user_data)
{
	startfunc
	RETM_IF(!user_data, "user_data is NULL");
	main_view *view = user_data;

	if (!view->camera_enabled) {
		ERR("Camera hasn't been initialized.");
		return;
	}

	if (image->format == CAMERA_PIXEL_FORMAT_JPEG) {
		DBG("got JPEG data - data [%p], length [%d], width [%d], height [%d]",
		    image->data, image->size, image->width, image->height);

		size_t size = _main_view_get_file_path(view->filename, sizeof(view->filename), CAM_FILE_IMAGE);
		RETM_IF(0 == size, "_main_view_get_filename() failed");
		DBG("%s", view->filename);

		FILE *file = fopen(view->filename, "w+");
		RETM_IF(!file, "Failed to open file");
		/*Exif data backup from the captured image*/
		ExifData* exif = NULL;
		ExifLoader *exifLoader = NULL;
		/*save exif data*/
		exifLoader = exif_loader_new();
		exif_loader_write(exifLoader, image->data, image->size);
		exif = exif_loader_get_data(exifLoader);
		exif_loader_unref(exifLoader);
		exif_data_save_data(exif, &exifData, &exif_size);
		DBG("Exif SIze: %d Exif data is: %s", exif_size, exifData);
		exif_data_unref(exif);

		size = fwrite(image->data, image->size, 1, file);
		WARN_IF(size != 1, "failed to write file");
		fclose(file);
		file = NULL;
		if (view->cam_view_type != CAM_COMPACT_VIEW) {
			_main_file_register(view->filename);
		} else {
			_main_view_media_create(view->filename, CAM_TRANSFORM_CROP);
		}
	}
	endfunc
}

static void _main_view_capture_completed_cb(void *data)
{
	startfunc
	RETM_IF(!data, "data is NULL");
	main_view *view = data;

	if (!view->camera_enabled) {
		ERR("Camera hasn't been initialized.");
		return;
	}
	if (view->cam_view_type != CAM_COMPACT_VIEW) {
		_main_view_ug_send_result(view, view->filename);
		_main_view_start_camera_preview(view->camera);
	}
	endfunc
}

static int _main_view_get_image_orient_value_by_direction(void *data, CamTargetDirection target_direction)
{
	RETVM_IF(!data, 0, "data is NULL");
	main_view *view = data;

	int orient_value = 0;
#ifdef CAMERA_MACHINE_I686
	DBG(" CAMERA_ATTR_TAG_ORIENTATION_TOP_LEFT");
	orient_value = 0;
#else
	switch (target_direction) {
	case CAM_TARGET_DIRECTION_PORTRAIT:
		if (view->self_portrait == TRUE) {
			orient_value = 8;
		} else {
			orient_value = 6;
		}
		break;
	case CAM_TARGET_DIRECTION_PORTRAIT_INVERSE:
		if (view->self_portrait == TRUE) {
			orient_value = 6;
		} else {
			orient_value = 8;
		}
		break;
	case CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE:
		orient_value = 3;
		break;
	case CAM_TARGET_DIRECTION_LANDSCAPE:
		orient_value = 1;
		break;
	default:
		break;
	}
#endif
	DBG("target_direction=%d orient_value=%d", target_direction, orient_value);

	return orient_value;
}

void _main_view_emit_signal_layout(CamTargetDirection target_direction)
{
	startfunc
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "view is NULL");
	double scale = elm_config_scale_get();
	switch (target_direction) {
	case CAM_TARGET_DIRECTION_PORTRAIT:
		if (scale == 2.6) {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "portrait", "camera_layout");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "portrait", "recorder_layout");
		} else {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "portrait", "camera_layout_WVGA");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "portrait", "recorder_layout_WVGA");
		}
		break;
	case CAM_TARGET_DIRECTION_PORTRAIT_INVERSE:
		if (scale == 2.6) {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "portrait_inverse", "camera_layout");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "portrait_inverse", "recorder_layout");
		} else {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "portrait_inverse", "camera_layout_WVGA");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "portrait_inverse", "recorder_layout_WVGA");
		}
		break;
	case CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE:
		if (scale == 2.6) {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "landscape_inverse", "camera_layout");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "landscape_inverse", "recorder_layout");
		} else {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "landscape_inverse", "camera_layout_WVGA");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "landscape_inverse", "recorder_layout_WVGA");
		}
		break;
	case CAM_TARGET_DIRECTION_LANDSCAPE:
		if (scale == 2.6) {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "landscape", "camera_layout");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "landscape", "recorder_layout");
		} else {
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "landscape", "camera_layout_WVGA");
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "landscape", "recorder_layout_WVGA");
		}
		break;
	default:
		break;
	}
}

void _main_view_set_target_direction(CamTargetDirection target_direction)
{
	startfunc
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "view is NULL");
	int ret = 0;
	int rotate;
	CamTargetDirection display_direction = CAM_TARGET_DIRECTION_INVAILD;
	camera_rotation_e rotationVal = CAMERA_ROTATION_NONE;
	ret = camera_get_display_rotation(view->camera, (camera_rotation_e *)&rotate);
	if (ret == CAMERA_ERROR_NONE) {
		switch (rotate) {
		case CAMERA_ROTATION_NONE:
			display_direction = CAM_TARGET_DIRECTION_LANDSCAPE;
			break;
		case CAMERA_ROTATION_90:
			display_direction = CAM_TARGET_DIRECTION_PORTRAIT_INVERSE;
			break;
		case CAMERA_ROTATION_180:
			display_direction = CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE;
			break;
		case CAMERA_ROTATION_270:
			display_direction = CAM_TARGET_DIRECTION_PORTRAIT;
			break;
		default:
			DBG("invaild type");
			break;
		}

#ifdef CAMERA_MACHINE_I686
		display_direction = view->target_direction;
#endif
	} else {
		DBG("camera_get_display_rotation failed - code[%x]", ret);
		return ;
	}
	view->target_direction = target_direction;
	DBG(" target direction = %d , display_direction =%d", target_direction, display_direction);

	if (display_direction != view->target_direction) {
		switch (view->target_direction) {
		case CAM_TARGET_DIRECTION_PORTRAIT:
#ifndef CAMERA_MACHINE_I686
			rotationVal = CAMERA_ROTATION_270;
#endif
			ret = camera_set_display_rotation(view->camera, rotationVal);
			break;
		case CAM_TARGET_DIRECTION_PORTRAIT_INVERSE:
#ifndef CAMERA_MACHINE_I686
			rotationVal = CAMERA_ROTATION_90;
#endif
			ret = camera_set_display_rotation(view->camera, rotationVal);
			break;
		case CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE:
#ifndef CAMERA_MACHINE_I686
			rotationVal = CAMERA_ROTATION_180;
#endif
			ret = camera_set_display_rotation(view->camera, rotationVal);
			break;
		case CAM_TARGET_DIRECTION_LANDSCAPE:
#ifndef CAMERA_MACHINE_I686
			rotationVal = CAMERA_ROTATION_NONE;
#endif
			ret = camera_set_display_rotation(view->camera, rotationVal);
			break;
		default:
			break;
		}
		if (ret != CAMERA_ERROR_NONE) {
			ERR("camera_set_display_rotation failed - code[%x]", ret);
		}
	}
	_main_view_emit_signal_layout(view->target_direction);
	if (view->device_type == CAMERA_DEVICE_CAMERA1) {
		if ((target_direction == CAM_TARGET_DIRECTION_LANDSCAPE)) {
			camera_set_display_rotation(view->camera, CAMERA_ROTATION_180);
		}
		if ((target_direction == CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE)) {
			camera_set_display_rotation(view->camera, CAMERA_ROTATION_NONE);
		}
	}
	endfunc
}

static int _main_view_get_image_orient_value(void *data)
{
	RETVM_IF(!data, 0, "data is NULL");
	main_view *view = data;

	DBG("current device orientation %d", view->target_direction);
	return _main_view_get_image_orient_value_by_direction(view, view->target_direction);
}

static void _main_view_shutter_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
	startfunc
	RETM_IF(!data, "data is NULL");
	main_view *view = data;
	int orient = 0;
	int res = 0;

	if (view->cam_view_type == CAM_COMPACT_VIEW) {
		view->transformtype = CAM_TRANSFORM_CROP;
	}
	orient =  _main_view_get_image_orient_value(view);
	res = camera_attr_set_tag_orientation(view->camera, (camera_attr_tag_orientation_e)orient);
	if (res != CAMERA_ERROR_NONE) {
		ERR("camera_attr_set_tag_orientation failed - code[%x]", res);
	}

	if (view->camera_enabled) {
		camera_start_capture(view->camera, _main_view_capture_cb, _main_view_capture_completed_cb, view);
	} else {
		ERR("Camera hasn't been initialized.");
	}
	endfunc
}

static void _main_view_recorder_stop_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
	RETM_IF(!data, "data is NULL");
	RETM_IF(!grecord, "Invalid recorder handle");

	main_view *view = data;
	int error_code = 0;

	if (view->show_content == FALSE) {
		error_code = recorder_cancel(grecord);
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_cancel error code %d", error_code);
		}
	} else {
		error_code = recorder_commit(grecord);
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_commit error code %d", error_code);
		}
	}

	error_code = recorder_unprepare(grecord);
	if (error_code != RECORDER_ERROR_NONE) {
		ERR("recorder_unprepare error code %d", error_code);
	}

	if (view->show_content  == TRUE) {
		_main_file_register(view->filename);

		if (view->pause_state == FALSE) {
			_main_view_ug_send_result(view, view->filename);
		}
	}
	error_code = recorder_destroy(grecord);
	if (error_code != RECORDER_ERROR_NONE) {
		ERR("recorder_destroy error code %d", error_code);
	}
	grecord = NULL;
	main_view_camera_view_add(view);
	_main_view_set_target_direction(view->target_direction);
	if (view->pause_state == FALSE) {
		_main_view_start_camera_preview(view->camera);
	}
}

static Eina_Bool _main_view_recorder_pause_icon_update(void *data)
{
	RETVM_IF(!data, ECORE_CALLBACK_CANCEL, "data is NULL");
	RETVM_IF(!grecord, ECORE_CALLBACK_CANCEL, "Invalid recorder handle");

	int state = 0;
	int ret = RECORDER_ERROR_NONE;
	main_view *view = (main_view *)data;
	view->pause_time++;

	ret = recorder_get_state(grecord, (recorder_state_e *)&state);
	if (ret != RECORDER_ERROR_NONE) {
		ERR("recorder_get_state error code %d", ret);
	}

	if (state == RECORDER_STATE_PAUSED) {
		if (!(view->pause_time % 2) == 0) {
			edje_object_signal_emit(_EDJ(view->recording_icon), "recording_icon,pause,portrait", "prog");
			return ECORE_CALLBACK_RENEW;
		} else {
			edje_object_signal_emit(_EDJ(view->recording_icon), "recording_icon,hide", "prog");
			return ECORE_CALLBACK_RENEW;
		}
	} else {
		return ECORE_CALLBACK_CANCEL;
	}
}

static void _main_view_recorder_pause_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
	RETM_IF(!data, "data is NULL");
	RETM_IF(!grecord, "Invalid recorder handle");

	main_view *view = data;
	int state = 0;
	int error_code = 0;
	int ret = RECORDER_ERROR_NONE;

	ret = recorder_get_state(grecord, (recorder_state_e *)&state);
	if (ret != RECORDER_ERROR_NONE) {
		ERR("recorder_get_state error code %d", ret);
	}

	if (state == RECORDER_STATE_RECORDING) {
		error_code = recorder_pause(grecord);
		if (error_code != RECORDER_ERROR_NONE) {
			ERR("recorder_pause error code %d", error_code);
		}
	}
	_main_view_recorder_rec_icon_update(view);

	REMOVE_TIMER(view->pause_timer);
	view->pause_timer = ecore_timer_add(1.0, _main_view_recorder_pause_icon_update, (void *)view);
}

static void _main_view_recorder_resume_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
	RETM_IF(!data, "data is NULL");
	RETM_IF(!grecord, "Invalid recorder handle");

	int state = 0;
	int error_code = 0;
	int ret = RECORDER_ERROR_NONE;

	ret = recorder_get_state(grecord, (recorder_state_e *)&state);
	if (ret != RECORDER_ERROR_NONE) {
		ERR("recorder_get_state error code %d", ret);
	}

	if ((state == RECORDER_STATE_READY)
	        || (state == RECORDER_STATE_PAUSED)) {
		error_code = recorder_start(grecord);
		ERR("recorder_pause error code %d", error_code);
	}
}

static void _main_view_switch_camera_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
	RETM_IF(!data, "data is NULL");

	Eina_Bool ret = EINA_TRUE;
	main_view *view = (main_view *)data;

	ret = _main_view_stop_camera_preview(view->camera);
	if (ret == EINA_TRUE) {
		camera_destroy(view->camera);
		view->camera = NULL;
	} else {
		ERR("Failed to stop camera preview");
		return;
	}

	if (grecord) {
		ret = recorder_destroy(grecord);
		grecord = NULL;
	}

	if (view->self_portrait == EINA_TRUE) {
		view->self_portrait = EINA_FALSE;
		view->camera_enabled = _main_view_init_camera(view, CAMERA_DEVICE_CAMERA0);
		if (view->camera_enabled) {
			if (!_main_view_start_camera_preview(view->camera)) {
				ERR("Failed start preview");
				return;
			}
		}
		_main_view_set_target_direction(view->target_direction);
	} else {
		view->self_portrait = EINA_TRUE;
		view->camera_enabled = _main_view_init_camera(view, CAMERA_DEVICE_CAMERA1);
		if (view->camera_enabled) {
			if (!_main_view_start_camera_preview(view->camera)) {
				ERR("Failed start preview");
				return;
			}
		}

		if (view->device_type == CAMERA_DEVICE_CAMERA1) {
			if ((view->target_direction == CAM_TARGET_DIRECTION_LANDSCAPE)) {
				camera_set_display_rotation(view->camera, CAMERA_ROTATION_180);
			}
			if ((view->target_direction == CAM_TARGET_DIRECTION_LANDSCAPE_INVERSE)) {
				camera_set_display_rotation(view->camera, CAMERA_ROTATION_NONE);
			}
		}
	}
}
/*
static void _main_view_video_button_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
	Eina_Bool ret = EINA_TRUE;
	int error_code = 0;
	RETM_IF(!data, "data is NULL");
	main_view *view = (main_view *)data;

	if (view->cam_view_type == CAM_COMPACT_VIEW) {
		_main_view_ug_send_result(view, NULL);
		DBG("send to attachpanel for full view");
	}
	ret = _main_view_create_recorder(view->camera);
	if (!ret) {
		ERR("recorder creation failed");
		return;
	}
	_main_view_stop_camera_preview(view->camera);
	recorder_set_recording_status_cb(grecord, _main_view_recording_status_cb, view);
	ret = _main_view_init_recorder(view);
	if (!ret) {
		ERR("recorder init failed");
	}

	// Prepare the recorder, resulting in setting the recorder state to RECORDER_STATE_READY
	error_code = recorder_prepare(grecord);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_prepare error code %d", error_code);
		recorder_unset_recording_status_cb(grecord);
		recorder_destroy(grecord);
		grecord = NULL;
		_main_view_start_camera_preview(view->camera);
		return;
	}

	// Start the recorder
	error_code = recorder_start(grecord);
	if (error_code != RECORDER_ERROR_NONE) {
		DBG("recorder_start error code %d", error_code);
		recorder_unset_recording_status_cb(grecord);
		recorder_unprepare(grecord);
		recorder_destroy(grecord);
		grecord = NULL;
		_main_view_start_camera_preview(view->camera);
		return;
	}

	view->rec_elapsed = 0;
	view->rec_filesize = 0;
	_main_view_recorder_view_add(view);
}
*/
static void _main_view_show_warning_popup(Evas_Object *navi, const char *caption, const char *text, const char *button_text, void *data)
{
	RETM_IF(!data, "data is null");
	main_view *view = data;

	Evas_Object *popup = elm_popup_add(navi);
	RETM_IF(!popup, "Failed to create popup");

	elm_object_part_text_set(popup, "title,text", caption);
	elm_object_text_set(popup, text);
	evas_object_show(popup);

	Evas_Object *button = elm_button_add(popup);
	RETM_IF(!button, "Failed to add button");

	elm_object_style_set(button, POPUP_BUTTON_STYLE);
	elm_object_text_set(button, button_text);
	elm_object_part_content_set(popup, POPUP_BUTTON_PART, button);
	evas_object_smart_callback_add(button, "clicked", _main_view_popup_close_cb, view);


	view->popup = popup;
}

static void _main_view_popup_close_cb(void *data, Evas_Object *obj, void *event_info)
{
	RETM_IF(!data, "data is null");
	main_view *view = data;

	if (view->popup) {
		evas_object_del(view->popup);
		view->popup = NULL;
	}
}

void _main_view_set_show_content(Eina_Bool showContent)
{
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "view is NULL");
	int ret = 0;
	int state = 0;

	DBG("show content = %d", showContent);
	view->show_content = showContent;
	if (showContent == FALSE) {
		_main_view_app_pause();
	} else {
		_main_view_app_resume();
	}
	RETM_IF(!grecord, "Invalid recorder handle");

	ret = recorder_get_state(grecord, (recorder_state_e *)&state);
	if (ret == RECORDER_ERROR_NONE) {
		if ((state == RECORDER_STATE_RECORDING)
		        || (state == RECORDER_STATE_PAUSED)) {
			_main_view_recorder_stop_button_cb(view, NULL, NULL, NULL);
		}
	}
}

void _main_view_set_cam_view_layout(CamViewType cam_view_type)
{
	main_view *view = (main_view *)_main_view_get_data();
	RETM_IF(!view, "view is NULL");
	view->cam_view_type = cam_view_type;
	if (view->target_direction == CAM_TARGET_DIRECTION_PORTRAIT) {
		double scale = elm_config_scale_get();
		if (scale == 2.6) {
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "portrait", "recorder_layout");
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "portrait", "camera_layout");
		} else {
			edje_object_signal_emit(_EDJ(view->recorderview_layout), "portrait", "recorder_layout_WVGA");
			edje_object_signal_emit(_EDJ(view->cameraview_layout), "portrait", "camera_layout_WVGA");
		}
	}
}

