# BezOS Filesystem hierarchy

* `/` vfs root
    * `/System` system folder for internal data. `C:\Windows` or `/sbin/`
        * `/System/Options` global options and driver options
        * `/System/NT` windows NT compatibility
        * `/System/UNIX` unix compatibility
    * `/Devices` device objects. `/dev/`, global fs namespace on NT.
    * `/Computer` installed hardware. `/sys/`
    * `/Processes` process filesystem. `/proc/`
    * `/Users` per user data.
        * `/Users/Guest`
            * `/Users/Name/Programs`
            * `/Users/Name/Options`
                * `/Users/Name/Options/Local`
                * `/Users/Name/Options/Domain`
            * `/Users/Name/Documents`
        * `/Users/Operator`
    * `/Programs` globally available programs. `C:\Program Files` or `/bin/`
    * `/Volatile` temporary files. `/tmp/`
