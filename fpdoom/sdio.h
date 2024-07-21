
#ifndef SDIO_VERBOSE
#define SDIO_VERBOSE 1
#endif

extern unsigned sdio_shl;

void sdio_init(void);
int sdcard_init(void);
int sdio_read_block(uint32_t idx, uint8_t *buf);
int sdio_write_block(uint32_t idx, uint8_t *buf);

#if SDIO_VERBOSE > 1
extern int sdio_verbose;
#define SDIO_VERBOSITY(level) (sdio_verbose = (level))
#else
#define SDIO_VERBOSITY(level) (void)0
#endif
