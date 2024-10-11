/**
 * Copyright 2019-2020 DigitalOcean Inc.
 * Copyright 2021 Jens Elkner <jel+libprom@cs.uni-magdeburg.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROM_COLLECTOR_T_H
#define PROM_COLLECTOR_T_H

#include "../include/prom_collector.h"
#include "prom_map_t.h"
#include "../include/prom_string_builder.h"

struct prom_collector {
	const char *name;
	prom_map_t *metrics;
	prom_collect_fn *collect_fn;
	psb_t *string_builder;
	void *data;
	prom_collector_free_data_fn *free_data_fn;
};

#endif  // PROM_COLLECTOR_T_H
