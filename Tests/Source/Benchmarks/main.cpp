#include "../Common/TestsShell.h"

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

int main(int argc, char** argv)
{
	// Initialize and run doctest
	doctest::Context doctest_context;

	doctest_context.applyCommandLine(argc, argv);

	int doctest_result = doctest_context.run();

	if (doctest_context.shouldExit())
		return doctest_result;

	// RmlUi is initialized during doctest run above as necessary.
	// Clean everything up here.
	TestsShell::ShutdownShell();

	return doctest_result;
}
