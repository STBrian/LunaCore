struct _lcex_header {
    unsigned int magic;
    unsigned int _lcr_target_ver_maj;
    unsigned int _lcr_target_ver_min;
    unsigned int _lcr_target_ver_pat;
};

__attribute__((used, section(".header")))
static const struct _lcex_header header = {
    'l'|('c' << 8)|('e' << 16)|('x' << 24),
    LC_TARGET_VERSION_MAJOR, 
    LC_TARGET_VERSION_MINOR,
    LC_TARGET_VERSION_PATCH
};