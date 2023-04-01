/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_H_

/* This header file keeps all macros and functions needed for creating logging
 * messages (macros like @ref LOG_ERR).
 */
#define LOG_LEVEL_NONE 0U
#define LOG_LEVEL_ERR  1U
#define LOG_LEVEL_WRN  2U
#define LOG_LEVEL_INF  3U
#define LOG_LEVEL_DBG  4U

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logging
 * @defgroup logging Logging
 * @{
 * @}
 */

/**
 * @brief Logger API
 * @defgroup log_api Logging API
 * @ingroup logger
 * @{
 */

/**
 * @brief Writes an ERROR level message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_ERR(...)
#else
#define LOG_ERR(...)    Z_LOG(LOG_LEVEL_ERR, __VA_ARGS__)
#endif

/**
 * @brief Writes a WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_WRN(...)
#else
#define LOG_WRN(...)   Z_LOG(LOG_LEVEL_WRN, __VA_ARGS__)
#endif

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_INF(...)
#else
#define LOG_INF(...)   Z_LOG(LOG_LEVEL_INF, __VA_ARGS__)
#endif

/**
 * @brief Writes a DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_DBG(...)
#else
#define LOG_DBG(...)    Z_LOG(LOG_LEVEL_DBG, __VA_ARGS__)
#endif

/**
 * @brief Unconditionally print raw log message.
 *
 * The result is same as if printk was used but it goes through logging
 * infrastructure thus utilizes logging mode, e.g. deferred mode.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_PRINTK(...)
#else
#define LOG_PRINTK(...) Z_LOG_PRINTK(0, __VA_ARGS__)
#endif

/**
 * @brief Create module-specific state and register the module with Logger.
 *
 * This macro normally must be used after including <zephyr/logging/log.h> to
 * complete the initialization of the module.
 *
 * Module registration can be skipped in two cases:
 *
 * - The module consists of more than one file, and another file
 *   invokes this macro. (LOG_MODULE_DECLARE() should be used instead
 *   in all of the module's other files.)
 * - Instance logging is used and there is no need to create module entry. In
 *   that case LOG_LEVEL_SET() should be used to set log level used within the
 *   file.
 *
 * Macro accepts one or two parameters:
 * - module name
 * - optional log level. If not provided then default log level is used in
 *  the file.
 *
 * Example usage:
 * - LOG_MODULE_REGISTER(foo, CONFIG_FOO_LOG_LEVEL)
 * - LOG_MODULE_REGISTER(foo)
 *
 *
 * @note The module's state is defined, and the module is registered,
 *       only if LOG_LEVEL for the current source file is non-zero or
 *       it is not defined and CONFIG_LOG_DEFAULT_LEVEL is non-zero.
 *       In other cases, this macro has no effect.
 * @see LOG_MODULE_DECLARE
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_MODULE_REGISTER(...)
#else
#define LOG_MODULE_REGISTER(...)					\
	COND_CODE_1(							\
		Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__),			\
		(_LOG_MODULE_DATA_CREATE(GET_ARG_N(1, __VA_ARGS__),	\
				      _LOG_LEVEL_RESOLVE(__VA_ARGS__))),\
		() \
	)								\
	LOG_MODULE_DECLARE(__VA_ARGS__)
#endif

/**
 * @brief Macro for declaring a log module (not registering it).
 *
 * Modules which are split up over multiple files must have exactly
 * one file use LOG_MODULE_REGISTER() to create module-specific state
 * and register the module with the logger core.
 *
 * The other files in the module should use this macro instead to
 * declare that same state. (Otherwise, LOG_INF() etc. will not be
 * able to refer to module-specific state variables.)
 *
 * Macro accepts one or two parameters:
 * - module name
 * - optional log level. If not provided then default log level is used in
 *  the file.
 *
 * Example usage:
 * - LOG_MODULE_DECLARE(foo, CONFIG_FOO_LOG_LEVEL)
 * - LOG_MODULE_DECLARE(foo)
 *
 * @note The module's state is declared only if LOG_LEVEL for the
 *       current source file is non-zero or it is not defined and
 *       CONFIG_LOG_DEFAULT_LEVEL is non-zero.  In other cases,
 *       this macro has no effect.
 * @see LOG_MODULE_REGISTER
 */
#if defined(_MSC_VER) || defined(__NuttX__)	/* #CUSTOM@NDRS */
#define LOG_MODULE_DECLARE(...)
#else
#define LOG_MODULE_DECLARE(...)						      \
	extern const struct log_source_const_data			      \
			Z_LOG_ITEM_CONST_DATA(GET_ARG_N(1, __VA_ARGS__));     \
	extern struct log_source_dynamic_data				      \
			LOG_ITEM_DYNAMIC_DATA(GET_ARG_N(1, __VA_ARGS__));     \
									      \
	static const struct log_source_const_data *			      \
		__log_current_const_data __unused =			      \
			Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__) ?		      \
			&Z_LOG_ITEM_CONST_DATA(GET_ARG_N(1, __VA_ARGS__)) :   \
			NULL;						      \
									      \
	static struct log_source_dynamic_data *				      \
		__log_current_dynamic_data __unused =			      \
			(Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__) &&	      \
			IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) ?	      \
			&LOG_ITEM_DYNAMIC_DATA(GET_ARG_N(1, __VA_ARGS__)) :   \
			NULL;						      \
									      \
	static const uint32_t __log_level __unused =			      \
					_LOG_LEVEL_RESOLVE(__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_H_ */
