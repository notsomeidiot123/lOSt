//change read/write to be loaded as helper processes with highest priority when we switch back to multitasking
#include "storage.h"
#include "../cpu/io.h"
#include "../graphics/vga.h"
#include "../cpu/idt.h"
#include "timer.h"