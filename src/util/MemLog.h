#pragma once

// Heap measurement helper, ported from crosspoint-reader-cjk's low-memory
// crash investigation (docs/dev-notes/mem-investigation.md). Measurement only
// -- emits a single CSV-style [MEM] log line per call and changes no behavior.
//
// Compiled in only when -DENABLE_MEMLOG is set (dev builds); otherwise every
// call site collapses to a no-op with zero code/flash cost.
//
// Output format (parseable by scripts/debugging_monitor.py):
//   [MEM] phase,free,minfree,largest,internal
// where:
//   free     = esp_get_free_heap_size()                         (total free)
//   minfree  = esp_get_minimum_free_heap_size()                 (min since boot)
//   largest  = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) (fragmentation)
//   internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL)     (internal DRAM)
namespace MemLog {
#ifdef ENABLE_MEMLOG
void log(const char* phase);
#else
inline void log(const char*) {}
#endif
}  // namespace MemLog
