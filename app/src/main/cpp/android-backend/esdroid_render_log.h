/*
 * Copyright 2026 F² Cyanic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ESDROID_RENDER_LOG_H
#define ESDROID_RENDER_LOG_H

// Render logging is disabled for performance. These macros compile out so
// the render path does no logging at all.

#define ESLOG(...) ((void)0)
#define ESLOG_ENTER() ((void)0)
#define ESLOG_EXIT()  ((void)0)
#define ESLOG_STEP(n) ((void)0)
#define ESLOG_PTR(name, p) ((void)0)

#endif
