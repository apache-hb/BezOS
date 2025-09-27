# Tracking for which files need updating or replacing

sources/kernel/include
```
=== ACPI related stuff needs better testing, theres also some memory leaks ===

├── acpi
│   ├── acpi.hpp
│   ├── address.hpp
│   ├── aml.hpp
│   ├── fadt.hpp
│   ├── header.hpp
│   ├── hpet.hpp
│   ├── madt.hpp
│   └── mcfg.hpp

=== Most of the allocators can be deleted ===
├── allocator
│   ├── allocator.hpp
│   ├── synchronized.hpp
│   └── tlsf.hpp

=== Needs better documentation and some more tests ===
├── apic.hpp

=== Theres a few ununused files that can be deleted ===
├── arch
│   ├── abi.hpp
│   ├── arch.hpp
│   ├── cr0.hpp
│   ├── cr3.hpp
│   ├── cr4.hpp
│   ├── debug.hpp
│   ├── generic
│   │   ├── intrin.hpp
│   │   ├── machine.hpp
│   │   ├── mmu.hpp
│   │   └── stack.hpp
│   ├── hosted
│   │   └── intrin.hpp
│   ├── intrin.hpp
│   ├── isr.hpp
│   ├── machine.hpp
│   ├── mmu.hpp
│   ├── msr.hpp
│   ├── msrbits.hpp
│   ├── paging.hpp

=== Will be used once theres a sparc port ===
│   ├── sparcv9
│   │   ├── intrin.hpp
│   │   ├── machine.hpp
│   │   └── mmu.hpp

=== Could do with some more documentation and testing ==
│   ├── x86_64
│   │   ├── asm.h
│   │   ├── intrin.hpp
│   │   ├── machine.hpp
│   │   ├── mmu.hpp
│   │   └── stack.hpp
│   ├── xcr0.hpp
│   └── xsave.hpp

=== Can be deleted ===
├── audit
│   └── limit.hpp

=== Very poor encapsulation, should be improved and given tests ===
├── boot.hpp

=== Also needs tests, and should be made generic to ease testing ===
├── can.hpp

=== Should be moved somewhere else ===
├── clock.hpp

=== Should be moved somewhere else ===
├── cmos.hpp

=== Needs some updating to fit style guide ===
├── crt.hpp

=== Does not inspire joy, feels too complex ===
├── debug
│   ├── debug.hpp
│   └── packet.hpp

=== Needs to be easier to test and better documented ===
├── delay.hpp

=== Mostly fine ===
├── devices
│   ├── ddi.hpp
│   ├── hid.hpp
│   ├── log.hpp
│   ├── pcifs.hpp
│   ├── runfs.hpp
│   ├── stream.hpp
│   └── sysfs.hpp

=== Should be moved somewhere else, and try to reduce code duplication ===
├── display.hpp

=== Needs tests, and is kinda unused ===
├── drivers
│   ├── block
│   │   ├── driver.hpp
│   │   ├── ramblk.hpp
│   │   └── virtio.hpp
│   └── fs
│       ├── driver.hpp
│       ├── fat32.hpp
│       └── tmp.hpp

=== Don't like this code, needs replacing and more tests ===
├── elf.hpp

=== Needs better encapsulation, documentation, and much more testing ===
├── fs
│   ├── base.hpp
│   ├── device.hpp
│   ├── file.hpp
│   ├── folder.hpp
│   ├── identify.hpp
│   ├── interface.hpp
│   ├── iterator.hpp
│   ├── link.hpp
│   ├── node.hpp
│   ├── path.hpp
│   ├── query.hpp
│   ├── ramfs.hpp
│   ├── stream.hpp
│   ├── tarfs.hpp
│   ├── utils.hpp
│   └── vfs.hpp

=== These should be somewhere else ===
├── gdt.h
├── gdt.hpp

=== Very meh, need to flesh out event bus to make this better. Also some correctness issues when stuff is invoked from an interrupt ===
├── hid
│   ├── hid.hpp
│   └── ps2.hpp

=== Should be somewhere else ===
├── hypervisor.hpp

=== Update to fix style guide ===
├── isr
│   ├── isr.hpp
│   ├── runtime.hpp
│   └── startup.hpp

=== Biggest sin of the kernel, all the methods here should not be global ===
├── kernel.hpp

=== Document better ===
├── logger
│   ├── appender.hpp
│   ├── categories.hpp
│   ├── e9_appender.hpp
│   ├── logger.hpp
│   ├── queue.hpp
│   ├── serial_appender.hpp
│   └── vga_appender.hpp


├── memory

=== Kill this with fire, real mess ===
│   ├── address_space.hpp

=== Rename this file ===
│   ├── allocator.hpp

=== Document these and add more tests ===
│   ├── detail
│   │   ├── control_block.hpp
│   │   ├── pool.hpp
│   │   ├── table_list.hpp
│   │   └── tlsf.hpp

=== Document more, also probably more tests for the templated versions of the tlsf heap ===
│   ├── heap.hpp

=== Document more, also add tests ===
│   ├── heap_command_list.hpp

=== Update to match style guide, more comments, more tests ===
│   ├── layout.hpp
│   ├── memory.dox
│   ├── memory.hpp
│   ├── page_allocator.hpp
│   ├── page_allocator_command_list.hpp
│   ├── page_mapping_request.hpp

=== Needs to be more flexible to support dynamically adding and releasing pte pages ===
│   ├── page_tables.hpp
│   ├── paging.hpp
│   ├── pmm_heap.hpp
│   ├── pt_command_list.hpp
│   ├── range.hpp
│   ├── stack_mapping.hpp
│   ├── table_allocator.hpp
│   ├── tables.hpp
│   └── vmm_heap.hpp

=== Probably needs to be rewritten ===
├── memory.hpp

=== Needs documentation, testing, and to be usable from an interrupt ===
├── notify.hpp

=== Currently unused, either use or delete ===
├── objects
│   ├── event.hpp
│   ├── generic
│   │   └── event.hpp
│   ├── hosted
│   │   └── event.hpp
│   └── kernel
│       └── event.hpp

=== Update to fit style guide ===
├── panic.hpp
├── pat.hpp

=== Comment and test ===
├── pci
│   ├── config.hpp
│   ├── pci.hpp
│   └── vendor.hpp
├── processor.hpp
├── setup.hpp
├── smbios.hpp
├── smp.hpp
├── std
│   ├── aligned.hpp
│   ├── concurrency.hpp
│   ├── container
│   │   └── btree.hpp
│   ├── detail
│   │   ├── counted.hpp
│   │   ├── rcu_base.hpp
│   │   ├── retire_slots.hpp
│   │   └── sticky_counter.hpp
│   ├── fixed_deque.hpp
│   ├── fixed_vector.hpp
│   ├── forward_list.hpp
│   ├── function.hpp
│   ├── funqual.hpp
│   ├── hazard.hpp
│   ├── inlined_vector.hpp
│   ├── layout.hpp
│   ├── memory.hpp
│   ├── mutex.h
│   ├── queue.hpp

=== The rcu stuff needs some real testing, im still unsure if its 100% correct ===
│   ├── rcu
│   │   ├── atomic.hpp
│   │   ├── base.hpp
│   │   ├── shared.hpp
│   │   ├── snapshot.hpp
│   │   ├── weak.hpp
│   │   ├── weak_atomic.hpp
│   │   └── weak_snapshot.hpp
│   ├── rcu.hpp
│   ├── rcuptr.hpp
│   ├── recursive_mutex.hpp
│   ├── ringbuffer.hpp
│   ├── shared.hpp
│   ├── shared_spinlock.hpp
│   ├── span.hpp
│   ├── spinlock.hpp
│   ├── static_string.hpp
│   ├── static_vector.hpp
│   ├── std.hpp
│   ├── string.hpp
│   ├── string_view.hpp
│   ├── traits.hpp
│   └── vector.hpp

=== Very middling code, probably needs rewriting ===
├── syscall.hpp

=== This all needs to go, real catastrophe ===
├── system
│   ├── access.hpp
│   ├── base.hpp
│   ├── create.hpp
│   ├── detail
│   │   ├── address_segment.hpp
│   │   └── range_table.hpp
│   ├── device.hpp
│   ├── event.hpp
│   ├── handle.hpp
│   ├── invoke.hpp
│   ├── isolate.hpp
│   ├── mutex.hpp
│   ├── node.hpp
│   ├── pmm.hpp
│   ├── process.hpp
│   ├── query.hpp
│   ├── sanitize.hpp
│   ├── schedule.hpp
│   ├── system.hpp
│   ├── thread.hpp
│   ├── transaction.hpp
│   ├── vm
│   │   ├── file.hpp
│   │   └── memory.hpp
│   ├── vmem.hpp
│   ├── vmm.hpp
│   └── vmm_map_request.hpp

=== Also needs to go ===
├── system2
│   ├── process.hpp
│   ├── system.hpp
│   └── thread.hpp

=== Mostly fine, needs support for destroying the current thread ===
├── task
│   ├── mutex.hpp
│   ├── runtime.hpp
│   ├── scheduler.hpp
│   └── scheduler_queue.hpp

=== Rename this ===
├── thread.hpp

=== Needs testing ===
├── timer
│   ├── apic_timer.hpp
│   ├── hpet.hpp
│   ├── pit.hpp
│   ├── tick_source.hpp
│   └── tsc_timer.hpp

=== Probably remove this ===
├── tsc.hpp

=== Should probably go somewhere else ===
├── uart.hpp

=== Needs deleting ===
├── units.hpp

=== Another mess, try and rip all this out ===
├── user
│   ├── context.hpp
│   ├── sysapi.hpp
│   └── user.hpp

=== See what can be pulled out of here ===
├── util
│   ├── absl.hpp
│   ├── combine.hpp
│   ├── cpuid.hpp
│   ├── defer.hpp
│   ├── digit.hpp
│   ├── endian.hpp
│   ├── format
│   │   └── specifier.hpp
│   ├── format.hpp
│   ├── memory.hpp
│   ├── signature.hpp
│   ├── table.hpp
│   ├── util.hpp
│   └── uuid.hpp
└── xsave.hpp


sources/kernel/src
├── acpi
│   ├── acpi.cpp
│   ├── aml.cpp
│   └── format.cpp
├── allocator
│   └── bitmap.cpp
├── apic.cpp
├── arch
│   ├── cr.cpp
│   ├── dr.cpp
│   ├── sparcv9
│   │   └── machine.cpp
│   └── x86_64
│       └── setjmp.S
├── audit
│   └── limit.cpp
├── boot
│   ├── hyper.cpp
│   └── limine.cpp
├── boot.cpp
├── canvas.cpp
├── check.cpp
├── clock.cpp
├── cmos.cpp
├── crt.cpp
├── crt_freestanding.cpp
├── cxxrt.cpp
├── debug.cpp
├── delay.cpp
├── devices
│   ├── ddi.cpp
│   ├── hid.cpp
│   ├── stream.cpp
│   └── sysfs.cpp
├── drivers
│   ├── block
│   │   ├── driver.cpp
│   │   ├── ramblk.cpp
│   │   └── virtio_blk.cpp
│   └── fs
│       └── driver.cpp
├── elf
│   ├── elf.cpp
│   └── launch.cpp
├── font.hpp
├── fs
│   ├── device.cpp
│   ├── folder.cpp
│   ├── handle.cpp
│   ├── identify.cpp
│   ├── iterator.cpp
│   ├── node.cpp
│   ├── path.cpp
│   ├── query.cpp
│   ├── ramfs.cpp
│   ├── tarfs.cpp
│   ├── utils.cpp
│   └── vfs.cpp
├── gdt.cpp
├── hid
│   ├── hid.cpp
│   └── ps2.cpp
├── hypervisor.cpp
├── ipl.cpp
├── isr
│   ├── default.cpp
│   ├── isr.S
│   ├── isr.cpp
│   └── tables.cpp
├── log.cpp
├── logger
│   ├── e9_appender.cpp
│   ├── global_logger.cpp
│   ├── logger.cpp
│   ├── serial_appender.cpp
│   └── vga_appender.cpp
├── main.cpp
├── memory
│   ├── address_space.cpp
│   ├── allocator.cpp
│   ├── detail
│   │   ├── control_block.cpp
│   │   └── table_list.cpp
│   ├── heap.cpp
│   ├── heap_command_list.cpp
│   ├── layout.cpp
│   ├── memory.cpp
│   ├── page_allocator.cpp
│   ├── page_allocator_command_list.cpp
│   ├── page_mapping_request.cpp
│   ├── page_tables.cpp
│   ├── pte_command_list.cpp
│   ├── stack_mapping.cpp
│   ├── table_allocator.cpp
│   └── tables.cpp
├── memory.cpp
├── notify.cpp
├── pat.cpp
├── pci
│   ├── config.cpp
│   └── pci.cpp
├── processor.cpp
├── sanitizer
│   ├── asan.cpp
│   ├── stack.cpp
│   └── ubsan.cpp
├── setup.cpp
├── smbios.cpp
├── smp.S
├── smp.cpp
├── std
│   ├── rcu.cpp
│   └── rcuptr.cpp
├── syscall.cpp
├── system
│   ├── detail
│   │   └── address_segment.cpp
│   ├── device.cpp
│   ├── handle.cpp
│   ├── mutex.cpp
│   ├── node.cpp
│   ├── pmm.cpp
│   ├── process.cpp
│   ├── runtime.cpp
│   ├── runtime2.cpp
│   ├── sanitize.cpp
│   ├── schedule.S
│   ├── schedule.cpp
│   ├── system.cpp
│   ├── thread.cpp
│   ├── transaction.cpp
│   ├── vm
│   │   └── mapping.cpp
│   ├── vmm.cpp
│   └── vmm_map_request.cpp
├── task
│   ├── mutex.cpp
│   ├── runtime.S
│   ├── runtime.cpp
│   ├── scheduler.cpp
│   └── scheduler_queue.cpp
├── terminal
│   ├── buffered.cpp
│   └── terminal.cpp
├── thread.cpp
├── timer
│   ├── apic_timer.cpp
│   ├── hpet.cpp
│   ├── pit.cpp
│   ├── sleep.cpp
│   └── tsc.cpp
├── uart.cpp
├── user
│   ├── context.cpp
│   ├── os.cpp
│   ├── sysapi
│   │   ├── clock.cpp
│   │   ├── device.cpp
│   │   ├── handle.cpp
│   │   ├── mutex.cpp
│   │   ├── node.cpp
│   │   ├── process.cpp
│   │   ├── thread.cpp
│   │   └── vmem.cpp
│   └── user.cpp
├── user.S
├── util
│   ├── cpuid.cpp
│   ├── format.cpp
│   ├── memory.cpp
│   └── uuid.cpp
└── xsave.cpp

30 directories, 142 files
```