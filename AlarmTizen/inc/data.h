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

#if !defined(_DATA_H)
#define _DATA_H

#include <app_control.h>

typedef void (*data_alarm_callback_t)(void);

void data_initialize(void);
void data_finalize(void);
void data_handle_app_control(app_control_h app_control);
void data_set_alarms_callbacks(data_alarm_callback_t ontime_alarm_callback, data_alarm_callback_t recurring_alarm_callback);

#endif
