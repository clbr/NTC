;
; File generated by cc65 v 2.18 - Git 2f3955d
;
	.fopt		compiler,"cc65 v 2.18 - Git 2f3955d"
	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
	.importzp	tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
	.macpack	longbranch
	.export		_NTC_decompress

IDX = _idxarr

; defines that vary per index, the compressor gives you these
LOWOFF = 16
COUNTSOFF = 22
IDXBASEOFF = 113

HUFFS = IDX + 1
LOW = IDX + LOWOFF
NUM = LOW + 1
HIGHBYTES = NUM + 1
COUNTS = IDX + COUNTSOFF
IDXBASE = IDX + IDXBASEOFF

.segment	"BSS"

_huffcode = ptr1
_bits = regsave+2
_len = ptr2
_prev = ptr2+1
_huffpos = tmp3
_hufflen = tmp2
_readbits = tmp4
_srcbyte = ptr3 ; ptr3 high byte unused
_huffmod = tmp1
_idx = ptr4
_found = _idx ; idx is not used where found is
_storedbits = _idx+1 ; idx is not used where storedbits is
_src = sreg
_dst = regsave

; ---------------------------------------------------------------
; void __near__ NTC_decompress (const unsigned char *src, char *dst)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_NTC_decompress: near

	sta	_dst
	stx	_dst+1

	jsr	popax
	sta	_src
	stx	_src+1
;
; len = *src++;
;
	ldy	#0
	lda     (_src),y
	sta     _len

	inc	_src
	bne	:+
	inc	_src+1
:
;
; len--;
;
	dec     _len
;
; prev = *src++;
; *dst++ = prev;
;
	lda     (_src),y
	sta     _prev
	sta	(_dst),y

	inc	_src
	bne	:+
	inc	_src+1
:
	inc	_dst
	bne	:+
	inc	_dst+1
:
;
; readbits = 0;
;
	sty     _readbits
;
; srcbyte = *src;
;
	lda     (_src),y
	sta     _srcbyte
;
; while (len--) {
;
	jmp     L0092
;
; huffpos = 0;
;
L0023:	lda     #$00
	sta     _huffpos
;
; huffmod = 0;
;
	sta     _huffmod
;
; huffcode = 0;
;
	tax
	sta     _huffcode
	sta     _huffcode+1
;
; storedbits = 0;
;
	sta     _storedbits
;
; bits = 0;
;
	sta     _bits
	sta     _bits+1
;
; hufflen = 1;
;
	lda     #$01
	sta     _hufflen
;
; while (!huffs[hufflen - 1])
;
	jmp     L008F
;
; hufflen++;
;
L0033:	inc     _hufflen
;
; while (!huffs[hufflen - 1])
;
L008F:	ldy     _hufflen
	dey
	lda     HUFFS,y
	beq     L0033
;
; bits <<= 1;
;
L003A:
	asl	_bits
	rol	_bits+1
;
; bits |= srcbyte & 1;
;
	lda     _srcbyte
	and     #$01
	ora     _bits
	sta     _bits
;
; srcbyte >>= 1;
;
	lsr     _srcbyte
;
; storedbits++;
;
	inc     _storedbits
;
; readbits++;
;
	inc     _readbits
;
; if (readbits == 8) {
;
	lda     _readbits
	cmp     #$08
	bne     L0090
;
; src++;
;
	inc	_src
	bne	:+
	inc	_src+1
:
;
; srcbyte = *src;
;
	ldy     #$00
	lda     (_src),y
	sta     _srcbyte
;
; readbits = 0;
;
	sty     _readbits
;
; } while (storedbits < hufflen);
;
L0090:	lda     _storedbits
	cmp     _hufflen
	bcc     L003A
;
; found = 0;
;
	lda     #$00
	sta     _found
;
; if (bits == huffcode) {
;
L0054:	lda     _huffcode
	ldx     _huffcode+1
	cpx     _bits+1
	bne     L0058
	cmp     _bits
	bne     L0058
;
; found = 1;
;
	lda     #$01
	sta     _found
;
; break;
;
	jmp     L0055
;
; huffpos++;
;
L0058:	inc     _huffpos
;
; huffmod++;
;
	inc     _huffmod
;
; if (huffs[hufflen - 1] == huffmod) {
;
	ldy     _hufflen
	dey
	lda	HUFFS,y
	cmp     _huffmod
	bne     L005E
;
; prevlen = hufflen;
;
	ldx     #0
;
; hufflen++;
;
L008E:	inc     _hufflen
	inx
;
; while (!huffs[hufflen - 1])
;
	ldy     _hufflen
	dey
	lda     HUFFS,y
	beq     L008E
;
; huffmod = 0;
;
	lda     #$00
	sta     _huffmod
;
; huffcode++;
;
	inc	_huffcode
	bne :+
	inc	_huffcode+1
:
;
; huffcode <<= hufflen - prevlen;
;
@shift:
	asl	_huffcode
	rol	_huffcode+1
	dex
	bne	@shift
;
; break;
;
	jmp     L0055
;
; huffcode++;
;
L005E:
	inc	_huffcode
	bne	:+
	inc	_huffcode+1
:
;
; while (1) {
;
	jmp     L0054
;
; if (found)
;
L0055:	lda     _found
	jeq     L003A
;
; for (i = 0; ; i++) {
;
	ldy	#0
;
; if (prev <= highbytes[i]) {
;
L00A0:	lda     _prev
	cmp	HIGHBYTES,y
	beq     L009D
	bcs     L0084
;
; idx += i << 8;
;
L009D:
	sty	_idx+1
;
; break;
;
	jmp     L0083
;
; for (i = 0; ; i++) {
;
L0084:	iny
	jmp     L00A0
;
; idx += idxbase;
;
L0083:
	lda     #<IDXBASE
	sta     _idx

	lda     #>IDXBASE
	clc
	adc	_idx+1
	sta     _idx+1
;
; idx += counts[prev - low];
;
	lda     _prev
	sec
	sbc     LOW
	tay
	lda	COUNTS,y
	clc
	adc     _idx
	sta     _idx
	lda	#0
	adc     _idx+1
	sta     _idx+1
;
; *dst++ = prev = idx[huffpos];
;
L0079:
	ldy     _huffpos
	lda     (_idx),y
	sta	_prev

	ldy	#0
	sta	(_dst),y

	inc	_dst
	bne	:+
	inc	_dst+1
:
;
; while (len--) {
;
L0092:	lda     _len
	dec     _len
	tax
	jne     L0023
;
; *dst = '\0';
;
	txa
	tay
	sta     (_dst),y
;
; }
;
	rts

.endproc

