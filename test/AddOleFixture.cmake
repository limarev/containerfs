# ----- helpers to declare and build an OLE test tree -----
function(add_ole_fixture)
    # Usage:
    # add_ole_fixture(
    #   NAME <shortname>
    #   OLE_FILE <test.ole>
    #   ROOT_DIR <root>
    #   SPEC
    #     d:root
    #     d:root/A
    #     f:root/A/alpha.txt
    #     d:root/B/C
    #     f:root/B/C/data.bin
    #   [BIND_TEST <ctest-name>]
    #   [DISABLE_CLEANUP]              # optional; for debugging; defaults to cleanup always
    #   [GSF_EXE </path/to/gsf>]       # optional; defaults to variable 'gsf' if defined
    # )

    set(optionalArg DISABLE_CLEANUP)
    set(oneValueArgs NAME OLE_FILE ROOT_DIR BIND_TEST GSF_EXE)
    set(multiValueArgs SPEC)
    cmake_parse_arguments(AOF "${optionalArg}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(req NAME OLE_FILE ROOT_DIR)
        if(NOT AOF_${req})
            message(FATAL_ERROR "add_ole_fixture: missing required argument ${req}")
        endif()
    endforeach()

    # Set cleanup policy
    set(_DISABLE_CLEANUP FALSE)
    if(AOF_DISABLE_CLEANUP)
        set(_DISABLE_CLEANUP TRUE)
    endif ()

    # Resolve gsf executable
    set(_GSF "${AOF_GSF_EXE}")
    if(NOT _GSF)
        if(DEFINED gsf)
            set(_GSF "${gsf}")
        else()
            message(FATAL_ERROR "add_ole_fixture: provide GSF_EXE or define variable 'gsf' from find_program().")
        endif()
    endif()

    # Split SPEC into dirs/files
    set(_DIRS "")
    set(_FILES "")
    foreach(item IN LISTS AOF_SPEC)
        if(item MATCHES "^d:(.+)")
            list(APPEND _DIRS "${AOF_ROOT_DIR}/${CMAKE_MATCH_1}")
        elseif(item MATCHES "^f:(.+)")
            list(APPEND _FILES "${AOF_ROOT_DIR}/${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR "add_ole_fixture: SPEC item must begin with 'd:' or 'f:'. Got: ${item}")
        endif()
    endforeach()

    # Upper-cased, readable fixture names
    string(TOUPPER "${AOF_NAME}" _NAME_UP)
    set(_FX_TREE   "${_NAME_UP}_SETUP_TREE")
    set(_FX_OLE    "${_NAME_UP}_SETUP_OLE")

    # Emit a tiny CMake script that creates the tree in the binary dir
    set(_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/${AOF_NAME}_mkfs.cmake")
    file(WRITE "${_SCRIPT}" "\
cmake_minimum_required(VERSION 3.30)
set(DIRS ${_DIRS})
set(FILES ${_FILES})
foreach(d IN LISTS DIRS)
  file(MAKE_DIRECTORY \${d})
endforeach()
foreach(f IN LISTS FILES)
  file(WRITE \${f} \"\")
endforeach()
")
    # Setup: create tree
    add_test(NAME ${AOF_NAME}_setup_tree
             COMMAND ${CMAKE_COMMAND} -P "${_SCRIPT}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${AOF_NAME}_setup_tree PROPERTIES
                         FIXTURES_SETUP "${_FX_TREE}")

    # Setup: create OLE from tree
    add_test(NAME ${AOF_NAME}_setup_ole
             COMMAND "${_GSF}" createole "${AOF_OLE_FILE}" "${AOF_ROOT_DIR}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${AOF_NAME}_setup_ole PROPERTIES
                         FIXTURES_REQUIRED "${_FX_TREE}"
                         FIXTURES_SETUP    "${_FX_OLE}")

    # Cleanup: remove OLE (disabled by default)
    add_test(NAME ${AOF_NAME}_cleanup_ole
             COMMAND ${CMAKE_COMMAND} -E rm -f "${AOF_OLE_FILE}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${AOF_NAME}_cleanup_ole PROPERTIES
                         FIXTURES_REQUIRED "${_FX_TREE}"
                         FIXTURES_CLEANUP  "${_FX_OLE}"
                         DISABLED ${_DISABLE_CLEANUP})

    # Cleanup: remove root dir (disabled by default)
    add_test(NAME ${AOF_NAME}_cleanup_tree
             COMMAND ${CMAKE_COMMAND} -E rm -r "${AOF_ROOT_DIR}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${AOF_NAME}_cleanup_tree PROPERTIES
                         FIXTURES_CLEANUP "${_FX_TREE}"
                         DISABLED ${_DISABLE_CLEANUP})

    # Optionally attach fixtures directly to a test
    if(AOF_BIND_TEST)
        set_tests_properties(${AOF_BIND_TEST} PROPERTIES
                             FIXTURES_REQUIRED "${_FX_TREE};${_FX_OLE}")
    endif()

    # Export fixture names to parent scope in case you want to wire them manually
    set(${AOF_NAME}_FIXTURE_TREE "${_FX_TREE}" PARENT_SCOPE)
    set(${AOF_NAME}_FIXTURE_OLE  "${_FX_OLE}"  PARENT_SCOPE)
endfunction()
# ----- end helpers -----
