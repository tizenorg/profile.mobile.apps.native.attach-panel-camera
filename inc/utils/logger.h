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


#ifndef __LOGGER_H__
#define __LOGGER_H__

#define LOG_TAG "ATTACH_PANEL_CAMERA"

#include <dlog.h>

#define INF(fmt, arg...) dlog_print(DLOG_INFO, LOG_TAG, fmt,  ##arg)
#define DBG(fmt, arg...) dlog_print(DLOG_DEBUG, LOG_TAG, fmt,  ##arg)
#define ERR(fmt, arg...) dlog_print(DLOG_ERROR, LOG_TAG, fmt,  ##arg)

#define WARN_IF(expr, fmt, arg...) \
{ \
    if (expr) \
    { \
        ERR(fmt, ##arg); \
    } \
}

#define RETM_IF(expr, fmt, arg...) \
{ \
    if (expr) \
    { \
        ERR(fmt, ##arg); \
        return; \
    } \
}

#define RETVM_IF(expr, val, fmt, arg...) \
{ \
    if (expr) \
    { \
        ERR(fmt, ##arg); \
        return (val); \
    } \
}

#endif /* __LOGGER_H__ */
