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


#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <app.h>
#include <linux/limits.h>
#include <tzplatform_config.h>

#define SELF_CAMERA_LAYOUT  "attach-panel-camera/attach-panel-camera.edj"
#define SOUND_COUNT         "sounds/sounds_count.mp3"

#define POPUP_BUTTON_STYLE  "popup_button/default"
#define POPUP_BUTTON_PART   "button1"
inline char * get_path(char *string1, char *string2){
			char path[1024] = {};
			snprintf(path, 1024,"%s%s", string1, string2);
			return path;
}
/* storage path */
#define INTERNAL_DCIM_PATH	get_path(tzplatform_getenv(TZ_USER_CONTENT),"/DCIM")
#define CAMERA_DIRECTORY    get_path(INTERNAL_DCIM_PATH,"/Camera")
#define IMAGE_MIME_TYPE     "image/*"


/**
 * @brief Get resources folder absolute path
 * @return Absolute path to resources folder
 */
static inline const char *get_res_path()
{
    static char res_folder_path[PATH_MAX] = { '\0' };
    if(res_folder_path[0] == '\0')
    {
        char *resource_path_buf = app_get_resource_path();
        strncpy(res_folder_path, resource_path_buf, PATH_MAX-1);

    }
    return res_folder_path;
}

/**
 * @brief Get resource absolute path
 * @param[in]   file_path   Resource path relative to resources directory
 * @return Resource absolute path
 */
static inline const char *get_resource_path(const char *file_path)
{
    static char res_path[PATH_MAX] = { '\0' };
    snprintf(res_path, PATH_MAX, "%s%s", get_res_path(), file_path);
    return res_path;
}

#endif /* __CONFIG_H__ */
