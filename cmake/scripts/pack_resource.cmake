if (DEFINED IN
    AND DEFINED OUT
    AND DEFINED CMD)

    function (execute_locked)
        file(LOCK "${OUT}.lock" GUARD FUNCTION)

        if ("${IN}" IS_NEWER_THAN "${OUT}")
            message(STATUS "Executing ${CMD}")
            execute_process(COMMAND ${CMD})
        endif ()

    endfunction ()

    execute_locked()

    file(REMOVE "${OUT}.lock")

endif ()
