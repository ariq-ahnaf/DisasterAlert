#include <tizen.h>
#include <service_app.h>

#include "gpsservice.h"
#include "geolocation_manager.h"

bool
__create_service_app(void *data)
{
    return geolocation_manager_init();
}


void
__terminate_service_app(void *data)
{
	geolocation_manager_destroy_service();
}


void
__control_service_app(app_control_h app_control, void *data)
{
    return;
}


static void
__service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
}


static void
__service_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
}


static void
__service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}


static void
__service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
	geolocation_manager_stop_service();
}


static void
__service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}


int
main(int argc, char *argv[])
{
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = __create_service_app;
	event_callback.terminate = __terminate_service_app;
	event_callback.app_control = __control_service_app;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, __service_app_low_battery, NULL);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, __service_app_low_memory, NULL);
	service_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, __service_app_orient_changed, NULL);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, __service_app_lang_changed, NULL);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, __service_app_region_changed, NULL);

	return service_app_main(argc, argv, &event_callback, NULL);
}
