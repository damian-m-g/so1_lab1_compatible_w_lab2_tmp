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

#ifndef PROM_PROESS_FDS_I_INCLUDED
#define PROM_PROESS_FDS_I_INCLUDED

#include "../include/prom_metric.h"
int ppc_fds_new(prom_metric_t *m[], const char **label_keys);
int ppc_fds_update(const char *path, prom_metric_t *m[], const char **label_vals);

#endif  // PROM_PROESS_FDS_I_INCLUDED
