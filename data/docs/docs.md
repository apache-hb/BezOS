# Jeff BezOS Documentation

I have made a few unorthodox decisions when designing this kernel, I will document them here.

## Paths are not strings

Paths are not strings, this is true on all platforms but others pretend that `parent/file` is a single string
when in reality it is two segments of `parent` and `file`. In BezOS I've made the concious decision to use
the null terminator as a path separator and require begin and end pointers for all functions accepting paths.

## Syscalls are unstable

Syscall function numbers are intentionally unstable, I will eventually find a way to randomize them during build time.
Invoking syscall directly marries you to a very low level abi that is hard to change retroactively without breaking
userspace. Linux made this decision and it seems to be a thorn in their side. Forcing all syscalls through a shared
library allows for simpler behavioural changes, allows better tooling via replacing of that dll with debug variants,
and reduces the burden on the kernel to maintain exact backwards compatibility at the syscall abi level.

## File extensions

While file extensions are a bad way of detecting mimetypes they remain useful for human interaction, wherever
possible a file extension that matches the mimetype should be applied to files.

## Toggles for validation

Alot of hardware/firmware is broken and buggy, often checksums or sanity tests will fail but the hardware itself
will be functional. Theres nothing worse than needing to rebuild your kernel to get hardware working so always
provide flags to disable checksums and sanity tests. This also applies to skipping detection of hardware entirely
in the case that interacting with it causes system faults.
