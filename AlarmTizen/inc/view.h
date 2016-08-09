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

#if !defined(_VIEW_H)
#define _VIEW_H

#include <efl_extension.h>

Eina_Bool view_create(void);
Evas_Object *view_create_win(const char *pkg_name);
Evas_Object *view_create_layout(Evas_Object *parent, const char *file_path, const char *group_name);
Evas_Object *view_create_conformant_without_indicator(Evas_Object *win);
void view_handle_ontime_alarm(void);
void view_handle_recurring_alarm(void);
void view_destroy(void);

#endif
