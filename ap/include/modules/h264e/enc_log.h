/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : printf log info to ***.log
--
------------------------------------------------------------------------------*/
/**********************************************************
* INCLUDES
**********************************************************/
#ifndef _ENC_LOG_H
#define _ENC_LOG_H

/**********************************************************
* INCLUDES
**********************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/**
 * \defgroup api_logmsg Log System API
 *
 * @{
 */

/** Defines log control information types obtained from the environment. */
typedef enum _VCEncLogEnvControl {
  /** Information for the environment to control output files. */
  VCENC_LOG_OUTPUT = 0,
  /** Information for the environment to control the output level. */
  VCENC_LOG_LEVEL = 1,
  /** Information for the environment to control trace information for prompt and debugging. */
  VCENC_LOG_TRACE = 2,
  /** Information for the environment to control test-only information check. */
  VCENC_LOG_CHECK = 3
} VCEncLogEnvControl;

/** Defines log output modes. */
typedef enum _VCEncLogOutput {
  /** Outputs all logs to stdout. */
  LOG_STDOUT = 0,
  /** Outputs all logs to the same file.
   *  \n The log file is <tt>vcenc_trace_p${pid}.log</tt> where <tt>${pid}</tt> is the process ID. */
  LOG_ONE_FILE = 1,
  /** Outputs logs of each thread to a separate log file.
   *  \n The log files are named <tt>vcenc_trace_p${pid}t${tid}.log</tt> where <tt>${pid}</tt> is
   *  the process ID and <tt>${tid}</tt> is the thread ID. */
  LOG_BY_THREAD = 2,
  /** Outputs all logs to stderr. */
  LOG_STDERR = 3,
  LOG_COUNT = 4
} VCEncLogOutput;

/** Defines log types. */
typedef enum _VCEncLogFileType {
  /** Trace log, used to debug or analyze the status of the encoder behavior. */
  STREAM_TRACE_FILE = 0,
  /** (Reserved) Check log, which is used in tests to check whether a feature is enabled and
   *  properly configured. */
  STREAM_CHECK_FILE = 1,
  STREAM_COUNT = 2
} VCEncLogFileType;

/** Defines trace log types. */
typedef enum _VCEncLogTraceType {
  /** Encoder API call log. */
  VCENC_LOG_TRACE_API = 0,
  /** Register configuration log. */
  VCENC_LOG_TRACE_REGS = 1,
  /** EWL API call log. */
  VCENC_LOG_TRACE_EWL = 2,
  /** Memory usage log. */
  VCENC_LOG_TRACE_MEM = 3,
  /** Rate control status log. */
  VCENC_LOG_TRACE_RC = 4,
  /** Command line log. */
  VCENC_LOG_TRACE_CML = 5,
  /** Performance log. */
  VCENC_LOG_TRACE_PERF = 6,
  VCENC_LOG_TRACE_COUNT = 7
} VCEncLogTraceType;

/** (Reserved) Defines check log types. */
typedef enum _VCEncLogCheckType {
  /** Reconstructed YUV data. */
  VCENC_LOG_CHECK_RECON = 0,
  /** PSNR/SSIM for each frame. */
  VCENC_LOG_CHECK_QUALITY = 1,
  /** VBV information for rate control checking. */
  VCENC_LOG_CHECK_VBV = 2,
  /** Rate control information for rate control profiling. */
  VCENC_LOG_CHECK_RC = 3,
  /** Feature information for coverage checking. */
  VCENC_LOG_CHECK_FEATURE = 4,
  VCENC_LOG_CHECK_COUNT = 5
} VCEncLogCheckType;

/** Defines output log levels. */
typedef enum _vcenc_log_level {
  /** No logs output. */
  VCENC_LOG_QUIET = 0,
  /** The fatal level. */
  VCENC_LOG_FATAL = 1,
  /** The error level. */
  VCENC_LOG_ERROR = 2,
  /** The warning level. */
  VCENC_LOG_WARN = 3,
  /** The information level. */
  VCENC_LOG_INFO = 4,
  /** The debugging level. */
  VCENC_LOG_DEBUG = 5,
  /** All logs output. */
  VCENC_LOG_ALL = 6,
  VCENC_LOG_COUNT = 7
} VCEncLogLevel;

/** \brief Contains the log settings read from the environment or specified during initialization. */
typedef struct _VCEncLogSetting {
  /** \brief The log output mode. */
  VCEncLogOutput out_dir;
  /** \brief The lowest log level to be output.
   *  \n Logs of the specified level and higher are output. */
  VCEncLogLevel out_level;
  /** \brief A bitmap where each bit indicates whether to output each type of trace logs.
   *  \n <tt>0</tt>: do not output.
   *  \n <tt>1</tt>: output.
   *  \n For the mappings between bit indexes and trace log types, see Section
   *  <em> \ref VCEncLogTraceType</em>.
   *  \n The values are parsed from the <tt>trace_map</tt> parameter in <tt>VCEncLogInit()</tt>. */
  unsigned int k_trace_map;
  /** \brief (Reserved) A bitmap where each bit indicates whether to output each type of check logs.
   *  \n <tt>0</tt>: do not output.
   *  \n <tt>1</tt>: output.
   *  \n For the mappings between bit indexes and check log types, see Section
   *  <em> \ref VCEncLogCheckType</em>.
   *  \n The values are parsed from the <tt>check_map</tt> parameter in <tt>VCEncLogInit()</tt>. */
  unsigned int k_check_map;
} VCEncLogSetting;

#ifdef VCE_LOGMSG
/** Initializes the log system.
 *
 * Parameter settings are prioritized in ascending order as follows: default values < environment
 * settings < command line settings.
 *
 * \param [in] out_dir The log output mode.
 * \param [in] out_level The lowest log level to be output.
 *                       \n Logs of the specified level and higher are output.
 * \param [in] trace_map A bitmap where each bit controls whether to output each type of trace logs.
 *                       \n <tt>0</tt>: do not output.
 *                       \n <tt>1</tt>: output.
 *                       \n For the mappings between bit indexes and trace log types, see Section
 *                       <em> \ref VCEncLogTraceType</em>.
 * \param [in] check_map (Reserved) A bitmap where each bit controls whether to output each type of
 *                       check logs.
 *                       \n <tt>0</tt>: do not output.
 *                       \n <tt>1</tt>: output.
 *                       \n For the mappings between bit indexes and check log types, see Section
 *                       <em> \ref VCEncLogCheckType</em>.
 */
int VCEncLogInit(unsigned int out_dir, unsigned int out_level,
                 unsigned int trace_map, unsigned int check_map);

/** Prints trace logs of a given type and levels.
 *
 * \param [in] inst The instance for which trace logs are to be printed.
 * \param [in] level The lowest log level to be output.
 *                   \n Logs of the specified level and higher are output.
 * \param [in] log_trace_mask A bitmap where each bit controls whether to print each type of trace logs.
 *                            \n <tt>0</tt>: do not print.
 *                            \n <tt>1</tt>: print.
 *                            \n For the mappings between bit indexes and trace log types, see Section
 *                            <em> \ref VCEncLogTraceType</em>.
 * \param [in] fmt A string in the printf format.
 */
void VCEncTraceMsg(const void *inst, VCEncLogLevel level,
                   unsigned int log_trace_mask, const char *fmt, ...);

/** (Reserved) Prints check logs of a given type and levels. */
void VCEncCheckMsg(const void *inst, VCEncLogLevel level,
                   unsigned int log_check_mask, const char *fmt, ...);

/** Releases resources allocated to the log system. */
int VCEncLogDestory(void);

/** (Reserved) Outputs register information. */
void EncTraceRegs(const void *ewl, unsigned int readWriteFlag,
                  unsigned int mbNum, unsigned int *regs);

/** Queries the parameters for controlling the log system.
 *
 * \param [in] env_log The log settings read from the environment or specified during initialization.
 */
void VCEncLogGetEnvSetting(VCEncLogSetting *env_log);
#else
#define VCEncLogInit(...)
#define VCEncTraceMsg(...)
#define VCEncCheckMsg(...)
#define VCEncLogDestory()
#define EncTraceRegs(...)
#define VCEncLogGetEnvSetting(...)
#endif

/* Tracing macro for API */
/** \brief The function that obtains error logs from the encoder API module. */
#define APITRACEERR(fmt, ...)                                              \
  VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_API, "[%s:%d]" fmt, \
                __FUNCTION__, __LINE__, ##__VA_ARGS__)
/** \brief The function that obtains warning logs from the encoder API module. */
#define APITRACEWRN(fmt, ...)                              \
  VCEncTraceMsg(NULL, VCENC_LOG_WARN, VCENC_LOG_TRACE_API, \
                "[%s:%d]Warning: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the encoder API module.
 *  \n This function is used for tracing API calls. */
#define APITRACE(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the encoder API module.
 *  \n This function is used for tracing API pointer-type parameters. */
#define APITRACEPARAM_X(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the encoder API module.
 *  \n This function is used for tracing API integer-type parameters. */
#define APITRACEPARAM(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)

/* Tracing macro for API with Inst */
/** \brief The function that obtains error logs from the encoder API module, with the instance
 *  specified. */
#define APITRACE_ERR(inst, fmt, ...)                                        \
  VCEncTraceMsg(inst, VCENC_LOG_ERROR, VCENC_LOG_TRACE_API, "[%s:%d]" fmt, \
                __FUNCTION__, __LINE__, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the encoder API module, with the
 *  instance specified. */
#define APITRACE_INFO(inst, fmt, ...) \
  VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the encoder API module, with the
 *  instance specified.
 *  \n This function is used for tracing API integer-type parameters. */
#define APITRACE_PARAM(inst, fmt, ...) \
  VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)

/* Tracing macro for PERF */
/** \brief The function that obtains information logs for tracing performance. */
#define PERFTRACE(inst, fmt, ...) \
  VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_PERF, fmt, ##__VA_ARGS__)

/* Tracing macro for EWL */
// #define PTRACE_E( fmt, ... )
//     VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, "[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
/** \brief The function that obtains error logs from the EWL module.
 *  \n This function is used by EWL for hardware implementation. */
#define PTRACE_E(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, fmt, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the EWL module.
 *  \n This function is used by EWL for hardware implementation. */
#define PTRACE_I(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, fmt, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the EWL module. */
#define PTRACE(...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, ##__VA_ARGS__)
/** \brief The function that obtains information logs from the EWL module.
 *  \n This function is used by EWL for C-Model implementation. */
#define EWLTRACE_I(inst, fmt, ...) \
  VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, fmt, ##__VA_ARGS__)
/** \brief The function that obtains error logs from the EWL module.
 *  \n This function is used by EWL for C-Model implementation. */
#define EWLTRACE_E(inst, fmt, ...) \
  VCEncTraceMsg(inst, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, fmt, ##__VA_ARGS__)

/* Tracing macro for MEM */
/** \brief The function that obtains information logs for tracing memory usage. */
#define MEM_LOG_I(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_MEM, fmt, ##__VA_ARGS__)

/* Tracing macro for CWL */
/** \brief The function that obtains information logs for tracing command lines used to set
 *  up the test bench. */
#define CMLTRACE(...)                                                        \
  do {                                                                       \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_CML, ##__VA_ARGS__); \
  } while (0)

/* Tracing macro for RC */
/** \brief The function that obtains information logs for tracing the rate control process. */
#define RC_LOG_I(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_RC, fmt, ##__VA_ARGS__)
/** \brief The function that obtains error logs for tracing the rate control process. */
#define RC_LOG_E(fmt, ...)                                                \
  VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_RC, "[%s:%d]" fmt, \
                __FUNCTION__, __LINE__, ##__VA_ARGS__)
/** \brief The function that obtains debugging logs for tracing the rate control process. */
#define RC_LOG_D(fmt, ...) \
  VCEncTraceMsg(NULL, VCENC_LOG_DEBUG, VCENC_LOG_TRACE_RC, fmt, ##__VA_ARGS__)

/** @} */

#ifdef SUPPORT_MEM_STATISTIC
void StackStatInit();
void StackConsumption(const char *, unsigned int lineNum);
#else
#define StackStatInit()
#define StackConsumption(...)
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _ENC_LOG_H */
