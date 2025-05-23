cmake_minimum_required(VERSION 3.22)

if (DEFINED IN
    AND DEFINED OUT
    AND DEFINED CMD)

    function (execute_locked)
        file(LOCK "${OUT}.lock" GUARD FUNCTION)

        if ("${IN}" IS_NEWER_THAN "${OUT}")
            message(STATUS "Executing ${CMD}")
            execute_process(COMMAND ${CMD} COMMAND_ERROR_IS_FATAL ANY)
        endif ()

    endfunction ()

    execute_locked()

    file(REMOVE "${OUT}.lock")

endif ()
