/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <tizen.h>
#include <Elementary.h>

#include "alarm.h"
#include "view.h"
#include "view_defines.h"

static struct view_info {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *layout;
} s_info = {
	.win = NULL,
	.conform = NULL,
	.layout = NULL,
};

#define ALARM_MESSAGE_TIMEOUT 1.0

static void _delete_win_request_cb(void *data, Evas_Object *obj, void *event_info);
static void _win_back_cb(void *data, Evas_Object *obj, void *event_info);
static void _get_app_resource(const char *edj_file_in, char *edj_path_out);
static Eina_Bool _hide_recurring_alarm_message_cb(void *data);

/**
 * Function is invoked by controller when ontime alarm fires
 */
void view_handle_ontime_alarm(void)
{
	elm_object_signal_emit(s_info.layout, SIGNAL_ALARM_ON, PART_ONTIME_ALARM_STATE_TEXT);
	elm_object_signal_emit(s_info.layout, SIGNAL_ALARM_ON, PART_ALARM_IMAGE);
}

/**
 * Function is invoked by controller when recurring alarm fires
 */
void view_handle_recurring_alarm(void)
{
	elm_object_signal_emit(s_info.layout, SIGNAL_ALARM_ON, PART_RECURRING_ALARM_STATE_TEXT);
	elm_object_signal_emit(s_info.layout, SIGNAL_ALARM_ON, PART_ALARM_IMAGE);
	/*
	 * Timer is created to hide displayed alarm image and text message
	 */
	ecore_timer_add(ALARM_MESSAGE_TIMEOUT, _hide_recurring_alarm_message_cb, NULL);
}

/**
 * @brief Creates essential objects: window, conformant and layout.
 */
Eina_Bool view_create(void)
{
	char edj_path[PATH_MAX] = {0, };

	/* Create the window */
	s_info.win = view_create_win(PACKAGE);
	if (s_info.win == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a window.");
		return EINA_FALSE;
	}

	/* Create the conformant */
	s_info.conform = view_create_conformant_without_indicator(s_info.win);
	if (s_info.conform == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a conformant");
		return EINA_FALSE;
	}

	/* Create Layout */
	_get_app_resource(MAIN_EDJ, edj_path);
	s_info.layout = view_create_layout(s_info.win, edj_path, MAIN_GRP);
	if (s_info.layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a layout");
		return EINA_FALSE;
	}

	elm_object_content_set(s_info.conform, s_info.layout);

	/* Show the window after main view is set up */
	evas_object_show(s_info.win);

	return EINA_TRUE;
}

/**
 * @brief Creates a basic window named package.
 * @param[in] pkg_name Name of the window
 */
Evas_Object *view_create_win(const char *pkg_name)
{
	Evas_Object *win = NULL;

	/*
	 * Window
	 * Create and initialize elm_win.
	 * elm_win is mandatory to manipulate the window.
	 */
	win = elm_win_util_standard_add(pkg_name, pkg_name);
	elm_win_conformant_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);

	evas_object_smart_callback_add(win, "delete,request", _delete_win_request_cb, NULL);

	return win;
}

/**
 * @brief Creates a layout to target parent object with edje file
 * @param[in] parent The object to which you want to add this layout
 * @param[in] file_path File path of EDJ file will be used
 * @param[in] group_name Name of group in EDJ you want to set to
 * @param[in] cb_function The function will be called when back event is detected
 * @param[in] user_data The user data to be passed to the callback functions
 */
Evas_Object *view_create_layout(Evas_Object *parent, const char *file_path, const char *group_name)
{
	Evas_Object *layout = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return NULL;
	}

	/* Create layout using EDC(an edje file) */
	layout = elm_layout_add(parent);
	if (!elm_layout_file_set(layout, file_path, group_name))  {
		dlog_print(DLOG_ERROR, LOG_TAG, "Couldn't load layout from: %s", file_path);
		return NULL;
	}

	/* Layout size setting */
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	eext_object_event_callback_add(layout, EEXT_CALLBACK_BACK, _win_back_cb, NULL);

	evas_object_show(layout);

	return layout;
}

/**
 * @brief Creates a conformant without indicator for wearable app.
 * @param[in] win The object to which you want to set this conformant
 * Conformant is mandatory for base GUI to have proper size
 */
Evas_Object *view_create_conformant_without_indicator(Evas_Object *win)
{
	/*
	 * Conformant
	 * Create and initialize elm_conformant.
	 * elm_conformant is mandatory for base GUI to have proper size
	 * when indicator or virtual keypad is visible.
	 */
	Evas_Object *conform = NULL;

	conform = elm_conformant_add(win);
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, conform);

	evas_object_show(conform);

	return conform;
}

/**
 * @brief Destroys window and frees its resources.
 */
void view_destroy(void)
{
	if (s_info.win == NULL)
		return;

	evas_object_del(s_info.win);
}

/**
 * @brief Function will be operated when registered event is triggered.
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 * @param[in] event_info The system event information
 */
static void _delete_win_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	/*
	 * Write your code here for smart callback.
	 */
	ui_app_exit();
}

/**
 * Key back click puts window in background
 */
static void _win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	elm_win_lower(s_info.win);
}

/**
 * Conveniently return absolute edje path
 */
static void _get_app_resource(const char *edj_file_in, char *edj_path_out)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(edj_path_out, PATH_MAX, "%s%s", res_path, edj_file_in);
		free(res_path);
	}
}
/*
 * Callback function called by elapsing timer. In result, the text message
 * and an image hides.
 */
static Eina_Bool _hide_recurring_alarm_message_cb(void *data)
{
	elm_object_signal_emit(s_info.layout, SIGNAL_ALARM_OFF, PART_RECURRING_ALARM_STATE_TEXT);
	elm_object_signal_emit(s_info.layout, SIGNAL_ALARM_OFF, PART_ALARM_IMAGE);

	return ECORE_CALLBACK_CANCEL;
}
