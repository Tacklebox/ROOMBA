typedef enum error_code_type {
    TEST = 1,
    TEST2,
    PUSH_TASK_ERROR,
    KERNEL_REQUEST_ERROR,
    EXCEED_MAXTHREAD_ERROR,
    TIMING_CONFLICT_ERROR,
    WCET_LARGER_THAN_PERIOD_ERROR,
    PID_NOT_FOUND_ERROR,
    EXCEED_MAXMESSAGES_ERROR
} error_code;

void throw_error(error_code err);