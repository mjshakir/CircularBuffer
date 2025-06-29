# ─────────────────────────────────────────────────────────────────
# 1) Where to put results
# ─────────────────────────────────────────────────────────────────
set(VERIF_RESULTS "${CMAKE_BINARY_DIR}/formal_results")
file(MAKE_DIRECTORY "${VERIF_RESULTS}")

# ─────────────────────────────────────────────────────────────────
# 2) Find the tools we need
# ─────────────────────────────────────────────────────────────────
find_program(VALGRIND_EXECUTABLE NAMES valgrind)
find_program(ESBMC_EXECUTABLE    NAMES esbmc)

# ─────────────────────────────────────────────────────────────────
# 3) Umbrella "verify" target
# ─────────────────────────────────────────────────────────────────
add_custom_target(verify
  COMMENT "Running all formal and dynamic checks"
)

# ─────────────────────────────────────────────────────────────────
# 4) Helgrind (Valgrind)  
# ─────────────────────────────────────────────────────────────────
if (VALGRIND_EXECUTABLE)
  add_custom_target(helgrind
    COMMENT "Running Helgrind"
    COMMAND ${VALGRIND_EXECUTABLE}
      --tool=helgrind
      --log-file=${VERIF_RESULTS}/helgrind.log
      $<TARGET_FILE:${CIRCULARBUFFER_EXAMPLE_NAME}>
    DEPENDS ${CIRCULARBUFFER_EXAMPLE_NAME}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
  add_dependencies(verify helgrind)

  add_custom_target(drd
    COMMENT "Running DRD"
    COMMAND ${VALGRIND_EXECUTABLE}
      --tool=drd
      --log-file=${VERIF_RESULTS}/drd.log
      $<TARGET_FILE:${CIRCULARBUFFER_EXAMPLE_NAME}>
    DEPENDS ${CIRCULARBUFFER_EXAMPLE_NAME}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
  add_dependencies(verify drd)
else()
  message(WARNING "Valgrind not found; skipping helgrind/drd targets")
endif()

# ─────────────────────────────────────────────────────────────────
# 5) ESBMC (C++20 + pthreads)  
# ─────────────────────────────────────────────────────────────────
if (ESBMC_EXECUTABLE)
  add_custom_target(esbmc
    COMMENT "ESBMC (C++20 + threads) on the example"
    COMMAND ${ESBMC_EXECUTABLE}
      --std c++20
      --pthread
      --context-bound 2
      -I ${CMAKE_SOURCE_DIR}/include
      $<TARGET_FILE:${CIRCULARBUFFER_EXAMPLE_NAME}>
      --log ${VERIF_RESULTS}/esbmc.log
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(verify esbmc)
else()
  message(WARNING "ESBMC not found; skipping esbmc target")
endif()
