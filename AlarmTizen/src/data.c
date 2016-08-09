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

#include <efl_extension.h>
#include <app_alarm.h>

#include "alarm.h"
#include "data.h"

static struct data_info {
	int ontime_alarm_id;		/* id of the alarm set at date */
	int recurring_alarm_id;		/* id of the delayed recurring alarm */
	int recurring_alarm_count;	/* recurring alarm invocations count */
	data_alarm_callback_t ontime_alarm_callback;
	data_alarm_callback_t recurring_alarm_callback;
} s_info = {
	.ontime_alarm_id = -1,
	.recurring_alarm_id = -1,
	.recurring_alarm_count = -1,
	.ontime_alarm_callback = NULL,
	.recurring_alarm_callback = NULL,
};

#define APP_CONTROL_OPERATION_ALARM_RECURRING "http://tizen.org/appcontrol/operation/my_recurring_alarm"
#define APP_CONTROL_OPERATION_ALARM_ONTIME "http://tizen.org/appcontrol/operation/my_ontime_alarm"
#define RECURRING_ALARMS_TO_BE_INVOKED 1
#define ALARM_DELAY 1
#define ALARMS_INTERVAL 1
#define TIME_STRING_FORMAT_BUFFER_SIZE 10

static void _initialize_recurring_alarm(void);
static void _initialize_ontime_alarm(void);
static void _data_recurring_alarm_invoked(app_control_h app_control);
static void _data_ontime_alarm_invoked(app_control_h app_control);
/**
 * @brief Initialization function for data module.
 */
void data_initialize(void)
{
	/*
	 * initialization of recurring alarm.
	 */
	_initialize_recurring_alarm();

	/*
	 * initialization of on-time alarm.
	 */
	_initialize_ontime_alarm();
}

/**
 * @brief Finalization function for data module.
 */
void data_finalize(void)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "data finalize");

	if (s_info.recurring_alarm_id >= 0)
		alarm_cancel(s_info.recurring_alarm_id);

	if (s_info.ontime_alarm_id >= 0)
		alarm_cancel(s_info.ontime_alarm_id);
}
static bool _app_control_operation_equals(const char *predefined_operation, const char *app_control_operation)
{
	if (!predefined_operation && !app_control_operation)
		return true;
	if (!predefined_operation || !app_control_operation)
			return false;
	/*now we have non null strings*/
	if (strlen(predefined_operation) != strlen(app_control_operation))
		return false;
	return !strncmp(predefined_operation, app_control_operation, strlen(app_control_operation));
}

/* function is invoked by controller module (main.c)
 * to to handle app_controls call-backs from application life cycle loop
 */

void data_handle_app_control(app_control_h app_control)
{
	char *operation = NULL;
	int ret;

	dlog_print(DLOG_DEBUG, LOG_TAG, "data_handle_app_control");

	/*
	 * Get operation name bind to the app_control handle.
	 * If this function fails, then it is not possible to handle app_control
	 * call.
	 */
	ret = app_control_get_operation(app_control, &operation);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_get_operation() failed.");
		return;
	}
	dlog_print(DLOG_DEBUG, LOG_TAG, "operation: %s", operation);
	/*
	 * Based on app_control operation, then proper action is executed.
	 * In this case, only alarm related operations are supported. Below
	 * conditional statements verify if operation type match any of customly
	 * predefined alarm operation.
	 */
	if (_app_control_operation_equals(APP_CONTROL_OPERATION_ALARM_RECURRING, operation))
		_data_recurring_alarm_invoked(app_control);
	else if (_app_control_operation_equals(APP_CONTROL_OPERATION_ALARM_ONTIME, operation))
		_data_ontime_alarm_invoked(app_control);

	free(operation);
}

/**
 * Function sets callbacks that are invoked when recurring and ontime alarms are fired
 */
void data_set_alarms_callbacks(data_alarm_callback_t ontime_alarm_callback, data_alarm_callback_t recurring_alarm_callback)
{
	s_info.ontime_alarm_callback = ontime_alarm_callback;
	s_info.recurring_alarm_callback = recurring_alarm_callback;
}

/*
 * Function returns current local time in string format: HH:MM:SS
 * Obtained char buffer must be released using free() function.
 */
static char* _get_current_time(void)
{
	char *time_buff = (char *)calloc(TIME_STRING_FORMAT_BUFFER_SIZE, sizeof(char));
	time_t current_time;

	time(&current_time);
	strftime(time_buff, TIME_STRING_FORMAT_BUFFER_SIZE, "%H:%M:%S", localtime(&current_time));

	return time_buff;
}

/*
 * Function creates new recurring alarm using alarm_schedule_after_delay()
 * function. If the alarm is created successfully, then it will be fired
 * with delay of ALARM_DELAY seconds and with interval of ALARMS_INTERVAL
 * seconds.
 */
static void _initialize_recurring_alarm(void)
{
	int ret;
	app_control_h app_control;

	/*
	 * Creating new app_control handle.
	 * If this function fails, the alarm will not be scheduled.
	 */
	ret = app_control_create(&app_control);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_create() failed.");
		return;
	}

	/*
	 * Bind operation to created app_control handle.
	 * Custom operation name will be used to identify the alarm invocation
	 * within app_control callback and distinguish alarm types.
	 * If this function fails, the app_control handle must be released
	 * and the alarm will not be scheduled.
	 */
	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_ALARM_RECURRING);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_set_operation() failed.");
		app_control_destroy(app_control);
		return;
	}

	/*
	 * Bind package name to created app_control handle.
	 * Based on provided package name, the alarm will be invoked within
	 * the context of the application referenced by provided package name.
	 * If this function fails, the app_control handle must be released
	 * and the alarm will not be scheduled.
	 */
	ret = app_control_set_app_id(app_control, PACKAGE);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_set_app_id() failed.");
		app_control_destroy(app_control);
		return;
	}

	/*
	 * Set the alarm for created app_control handle.
	 * The alarm will be scheduled with ALARM_DELAY delay and ALARMS_INTERVAL
	 * period. In result, the first alarm invokation will occur ofter
	 * ALARM_DELAY seconds. Subsequent alarms invokations will occur
	 * with ALARMS_INTERVAL seconds. If ALARMS_INTERVAL == 0, then the alarm
	 * will be invoked only once after ALARM_DELAY seconds.
	 * If function succeeds, the alarm ID is returned and stored in appdata_s
	 * structure.
	 */
	ret = alarm_schedule_after_delay(app_control, ALARM_DELAY, ALARMS_INTERVAL, &s_info.recurring_alarm_id);
	if (ret != ALARM_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "Function alarm_schedule_after_delay() failed.");

	/*
	 * Finally, the app_control handle is released.
	 */
	ret = app_control_destroy(app_control);
	if (ret != APP_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_destroy() failed.");
	else
		dlog_print(DLOG_DEBUG, LOG_TAG, "Set recurring alarm with id: %i", s_info.recurring_alarm_id);
}

/*
 * Function creates new alarm set on time, using alarm_schedule_at_date()
 * function. If the alarm is created successfully, then it will be fired
 * at specified date and time.
 */
static void _initialize_ontime_alarm(void)
{
	int ret;
	app_control_h app_control;
	time_t t_alarm;
	/*
	 * Creating new app_control handle.
	 * If this function fails, the alarm will not be scheduled.
	 */
	ret = app_control_create(&app_control);
	if (ret != 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_create() failed.");
		return;
	}

	/*
	 * Bind operation to created app_control handle.
	 * Custom operation name will be used to identify the alarm invokation
	 * within app_control callback and distinguish alarm types.
	 * If this function fails, the app_control handle must be released
	 * and the alarm will not be scheduled.
	 */
	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_ALARM_ONTIME);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_set_operation() failed.");
		app_control_destroy(app_control);
		return;
	}

	/*
	 * Bind package name to created app_control handle.
	 * Based on provided package name, the alarm will be invoked within
	 * the context of the application referenced by provided package name.
	 * If this function fails, the app_control handle must be released
	 * and the alarm will not be scheduled.
	 */
	ret = app_control_set_app_id(app_control, PACKAGE);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_set_app_id() failed.");
		app_control_destroy(app_control);
		return;
	}

	/*
	 * Compute the time of on-time alarm invocation.
	 */
	t_alarm = time(NULL) + ALARM_DELAY + ALARMS_INTERVAL * RECURRING_ALARMS_TO_BE_INVOKED;;

	/*
	 * Set the alarm for created app_control handle.
	 * The alarm will be scheduled on time t_alarm without recurrency.
	 * If function succeeds, the alarm ID is returned and stored in appdata_s
	 * structure.
	 */
	ret = alarm_schedule_at_date(app_control, localtime(&t_alarm), 0, &s_info.ontime_alarm_id);
	if (ret != ALARM_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "Function alarm_schedule_at_date() failed.");

	/*
	 * Finally, the app_control handle is released.
	 */
	ret = app_control_destroy(app_control);
	if (ret != APP_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_destroy() failed.");
	else
		dlog_print(DLOG_DEBUG, LOG_TAG, "Set ontime alarm with id: %i", s_info.ontime_alarm_id);
}

/*
 * Function is called if alarm control arrives and is identified
 * as recurring alarm.
 */
static void _data_recurring_alarm_invoked(app_control_h app_control)
{
	int ret;
	char *alarm_data = NULL;
	char *time_str = NULL;

	dlog_print(DLOG_DEBUG, LOG_TAG, "recurring alarm invoked by appcontrol");

	/*
	 * Get data attached to the app_control handle by using common alarm data
	 * identifier APP_CONTROL_DATA_ALARM_ID. The extracted data contains alarm
	 * identifier stored as char buffer.
	 * If this function fails, the alarm can not be properly verified.
	 */
	ret = app_control_get_extra_data(app_control, APP_CONTROL_DATA_ALARM_ID, &alarm_data);

	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_get_extra_data() failed.");
		return;
	}

	if (alarm_data == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "alarm_data == NULL");
		return;
	}
	/*
	 * Verify if obtained alarm identifier is equal to the identifier
	 * of created alarm. If IDs does not match, then the alarm invocation
	 * belongs to other recurring alarm.
	 */
	if (atoi(alarm_data) != s_info.recurring_alarm_id)
		return;

	/*
	 * Count number of recurring alarm invocations
	 */
	s_info.recurring_alarm_count++;

	/*
	 * Inform controller about firing an alarm.
	 */
	if (s_info.recurring_alarm_callback)
		s_info.recurring_alarm_callback();

	time_str = _get_current_time();
	dlog_print(DLOG_INFO, LOG_TAG, "Recurring alarm #%d invoked at %s", s_info.recurring_alarm_count, time_str);
	free(time_str);

	/*
	 * If number of recurring alarm invocations is less then expected number,
	 * then the function ends its execution and the alarm will be invoked again.
	 */
	if (s_info.recurring_alarm_count < RECURRING_ALARMS_TO_BE_INVOKED)
		return;

	/*
	 * If recurring alarm is invoked expected number of times, then it is
	 * canceled. If this function fails, then the recurring alarm will not
	 * be stopped.
	 */
	ret = alarm_cancel(s_info.recurring_alarm_id);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function alarm_cancel() failed.");
		return;
	}

	dlog_print(DLOG_INFO, LOG_TAG, "Recurring alarm canceled");
}

/*
 * Function is called if alarm control arrives and is identified as on-time alarm.
 */
static void _data_ontime_alarm_invoked(app_control_h app_control)
{
	int ret;
	char *alarm_data = NULL;
	char *time_str = NULL;

	dlog_print(DLOG_DEBUG, LOG_TAG, "ontime alarm invoked by appcontrol");
	/*
	 * Get data attached to the app_control handle by using common alarm data
	 * identifier APP_CONTROL_DATA_ALARM_ID. The extracted data contains alarm
	 * identifier stored as char buffer.
	 * If this function fails, the alarm can not be properly verified.
	 */
	ret = app_control_get_extra_data(app_control, APP_CONTROL_DATA_ALARM_ID, &alarm_data);
	if (ret != APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Function app_control_get_extra_data() failed.");
		return;
	}

	if (alarm_data == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "alarm_data empty");
		return;
	}

	/*
	 * Verify if obtained alarm identifier is equal to the identifier of
	 * created alarm. If IDs does not match, then the alarm invocation belongs
	 * to other on-time alarm.
	 */
	if (atoi(alarm_data) != s_info.ontime_alarm_id)
		return;

	/*
	 * Inform controller about firing an alarm.
	 */
	if (s_info.recurring_alarm_callback)
		s_info.recurring_alarm_callback();

	time_str = _get_current_time();
	dlog_print(DLOG_INFO, LOG_TAG, "Ontime alarm invoked at %s", time_str);
	free(time_str);

	/*
	 * If the alarm is not set as recurring, then it must not call
	 * alarm_cancel() function. In this case, the on-tim alarm is automatically
	 * released.
	 */
}

