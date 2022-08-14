#pragma once

// the page fault handler. This function should not return to caller but
// return to user space if it's a recoverable page fault.
struct InterruptFrame;
void pfhandler(InterruptFrame* framePtr);
