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
