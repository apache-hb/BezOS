namespace overlay {

typedef string overlay_path<>;

struct create_sysroot_args {
    string sysroot<>;
    overlay_path overlays<>;
};

struct destroy_sysroot_args {
    string sysroot<>;
};

program OverlayFsDaemon {
    version DaemonV1 {
        int create_sysroot(create_sysroot_args) = 0;
        int destroy_sysroot(destroy_sysroot_args) = 1;
    } = 1;
} = 0x810278348;
}
