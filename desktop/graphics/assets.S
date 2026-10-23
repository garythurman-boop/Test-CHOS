.section .rodata
.balign 4
.global royale_fonts, royale_fonts_end
royale_fonts:
.incbin "assets/royale/fonts.bin"
royale_fonts_end:
.global font_license, font_license_end
font_license:
.incbin "assets/fonts/OFL.txt"
font_license_end:
.section .note.GNU-stack,"",%progbits
