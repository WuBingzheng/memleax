#ifndef MLD_DEBUG_FILE_H
#define MLD_DEBUG_FILE_H

/**
 * @brief debug_try_init
 * @param path
 * @param exe_self
 */
void debug_try_init(const char *path, int exe_self);

/**
 * @brief debug_try_get
 * @return
 */
const char *debug_try_get(void);

#endif
