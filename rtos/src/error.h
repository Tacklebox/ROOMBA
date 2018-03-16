typedef enum error_code_type {
    TEST = 1,
    TEST2,
    PUSH_TASK_ERROR,
    KERNEL_REQUEST_ERROR
} error_code;

void throw_error(error_code err);