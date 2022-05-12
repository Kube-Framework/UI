/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Benchmark of UI
 */

#include <benchmark/benchmark.h>

static void UI_Dummy(benchmark::State &state)
{
    for (auto _ : state) {
    }
}
BENCHMARK(UI_Dummy);
