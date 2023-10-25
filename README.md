# Overview

Welcome to lOSt, An Open-Source Semi-Modular Operating System designed and written by [Kimi](https://github.com/notsomeidiot123/notsomeidiot123)

lOSt focuses on trying to be 3 main things

- Lightweight: lOSt has very low minimum specs especially when compared to other modern operating systems, however, at this stage of development, that's not really a fair comparison, but it still manages to use < 2 MB of RAM, with the majority of what's being used only being reserved, rather than actually used
- Modular: lOSt allows easy access to hardware, without having to jump through hoops by allowing certain processes to set up special interrupts to be used as if they were set up by the kernel itself, simplifying and speeding up IPC for more important things, such as drivers, and servers
- Customizability: This sort of goes hand-in-hand with modularity, however, it is slightly different. lOSt's design allows not only for extreme customization in the code itself, by the pure fact of being OSS, but also allows for things like the swapping of shells and major software by editing only a couple of settings. By default, lOSt ships with the Kernel, basic drivers, and a raw, bash-like terminal shell, with only a few built-in commands that allow you to create your own powerful suite for any use you'd like, whether that be development, as a daily driver, or even gaming
- Compatibility: While lOSt has it's own interrupt table, we strive for compatiblity of software with other operating systems hosted inside the kernel itself. Together with the ability to load custom interrupt handlers, anyone can help improve compatibility, even without re-installing the operating system!
