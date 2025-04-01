# Project source layout

* `repo` - package recipes
* `tool` - package build tool
* `data` - misc data
* `sources` - source code
    * `kernel` - kernel mode code
    * `overlays` - build overlays used by repobld
    * `sysapi` - os syscall interface and posix implementation
    * `system` - usermode programs
    * `userlib` - utility library and wrappers around `sysapi`
