#include <tizen.h>
#include <message_port.h>
#include "gpsservice-consumer.h"
#include "view_manager.h"

#define LOCAL_PORT_NAME "gps-consumer-port"
#define MESSAGE_TYPE_STR "msg_type"
#define MESSAGE_TYPE_SATELLITES_UPDATE "SATELLITES_UPDATE"
#define MESSAGE_TYPE_POSITION_UPDATE "POSITION_UPDATE"
#define MESSAGE_SATELLITES_COUNT_STR "satellites_count"
#define MESSAGE_LATITUDE_STR "latitude"
#define MESSAGE_LONGITUDE_STR "longitude"
#define MESSAGE_CONTAINS_STR "contains"

static void __update_position(char *latitude_str, char *longitude_str);
static void __update_satellites(char *satellites_count_str);
static void __msg_port_cb(int local_port_id,
							 const char *remote_app_id,
							 const char *remote_port,
							 bool trusted, bundle *message,
							 void *user_data);
static bool __get_error_check(bundle *message,
								 char *msg_type,
								 char **message_content);

static bool
__create_app(void *data)
{
	/* Create GUI */
	if (!view_manager_create_base_gui()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create base gui");
		return false;
	}

	/* Register a local port to receive messages from gps-service */
	int local_port_id = message_port_register_local_port(LOCAL_PORT_NAME, __msg_port_cb, NULL);
	if (local_port_id < 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to register port, error: %d", local_port_id);
		return false;
	}

	dlog_print(DLOG_INFO, LOG_TAG, "Registered local port, port id: %d", local_port_id);

	return true;
}

static void
__control_app(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
__pause_app(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
__resume_app(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
__terminate_app(void *data)
{
	/* Release all resources. */
	view_manager_destroy();
}

static void
__ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
__ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
__ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
__ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
__ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = __create_app;
	event_callback.terminate = __terminate_app;
	event_callback.pause = __pause_app;
	event_callback.resume = __resume_app;
	event_callback.app_control = __control_app;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, __ui_app_low_battery, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, __ui_app_low_memory, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, __ui_app_orient_changed, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, __ui_app_lang_changed, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, __ui_app_region_changed, NULL);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, NULL);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}

static void
__update_position(char *latitude_str, char *longitude_str)
{
	double latitude = 70;
	double longitude = 133;

	if (!latitude_str || !longitude_str) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid parameter");
		return;
	}

	latitude = strtod(latitude_str, NULL);
	longitude = strtod(longitude_str, NULL);

	view_manager_update_map_position(longitude, latitude);
}

static void
__update_satellites(char *satellites_count_str)
{
	view_manager_update_satellites_count(satellites_count_str);
}

static void
__msg_port_cb(int local_port_id, const char *remote_app_id, const char *remote_port, bool trusted, bundle *message, void *user_data)
{
	char *msg_type = NULL;
	char *latitude_str = NULL;
	char *longitude_str = NULL;
	char *satellites_count_str = NULL;

	if (!__get_error_check(message, MESSAGE_TYPE_STR, &msg_type)) {
		return;
	}

	if (!strncmp(msg_type, MESSAGE_TYPE_SATELLITES_UPDATE, strlen(msg_type))) {
		if (!__get_error_check(message, MESSAGE_SATELLITES_COUNT_STR, &satellites_count_str)) {
			return;
		}
		dlog_print(DLOG_INFO, LOG_TAG, "Received message from %s: satellites_count %s", remote_app_id, satellites_count_str);
		__update_satellites(satellites_count_str);
	} else if (!strncmp(msg_type, MESSAGE_TYPE_POSITION_UPDATE, strlen(msg_type))) {
		if (!__get_error_check(message, MESSAGE_LATITUDE_STR, &latitude_str) ||
				!__get_error_check(message, MESSAGE_LONGITUDE_STR, &longitude_str)) {
			return;
		}
		dlog_print(DLOG_INFO, LOG_TAG, "Received message from %s: position data: %s %s", remote_app_id, latitude_str, longitude_str);
		__update_position(latitude_str, longitude_str);
	}
}

static bool
__get_error_check(bundle *message, char *msg_type, char **message_content)
{
	if (bundle_get_str(message, msg_type, message_content) != BUNDLE_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get message: %s", msg_type);
		return false;
	}

	return true;
}
