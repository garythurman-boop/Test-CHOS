/* Controlled validation payloads; zero-sized in ordinary builds. */
.section .rodata.growth,"a"
.global growth_ro_start,growth_ro_end
growth_ro_start:
.fill CHOS_TEST_RO,1,0xa5
growth_ro_end:
.section .bss.growth,"aw",%nobits
.balign 4096
.global growth_bss_start,growth_bss_end
growth_bss_start:
.skip CHOS_TEST_BSS
growth_bss_end:

/* Probe the last growth page, beyond the former bootstrap ceiling. Zero-sized
 * production builds alias the old probe addresses and emit identical bytes. */
.global growth_ro_probe,growth_bss_probe
.if CHOS_TEST_RO >= 4096
.set growth_ro_probe,growth_ro_start+CHOS_TEST_RO-4096
.else
.set growth_ro_probe,growth_ro_start
.endif
.if CHOS_TEST_BSS >= 4096
.set growth_bss_probe,growth_bss_start+CHOS_TEST_BSS-4096
.else
.set growth_bss_probe,growth_bss_start
.endif
