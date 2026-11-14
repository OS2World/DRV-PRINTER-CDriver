        	page    55,132

;******************************************************************************
;
; Device Driver Stub module for the Byers IKON 10092 plotter interface
;
;	This module provides an interface between OS/2 and a C driver module.
;	It is intended to be used with BIKONC.C.  The 'C' module must supply 
;	the function @DriverStrategy:near to handle all Driver Strategy 
;	Packets.
;
;	history:
;	01-08-91 ras - SCR P0487, initial entry
;
;		Copyright 1991, Byers Engineering Company
;******************************************************************************

		page
; definitions area

		.286c				;processor model

; define external 'C' modules
		extrn	@DriverStrategy:near

; define public symbols available in this module
		public	_Drv_qDevHlpAddr
		public	HLPVERIFYACCESS
		public	HLPPHYSTOUVIRT
		public	HLPSETIRQ
		public	HLPUNSETIRQ
		public	HLPEOI
		public	HLPREGISTERSTACK
		public	HLPQUEUEINIT
		public	HLPQUEUEREAD
		public	HLPQUEUEWRITE
		public	HLPQUEUEFLUSH
		public	HLPBLOCK
		public	HLPRUN
		public	HLPPULLREQPKT
		public	HLPPUSHREQPKT
		public	HLPVIRTTOPHYS
		public	HLPLOCK
		public	HLPUNLOCK
		public	HLPALLOCGDTSEL
		public	HLPPHYSTOGDTSEL

; define driver literals
BPT			equ	byte ptr
WPT			equ	word ptr
DPT			equ	dword ptr

; define Device Helper literals
DEVHLP_VERIFYACCESS	equ	027h
DEVHLP_PHYSTOUVIRT	equ	017h
DEVHLP_SETIRQ		equ	01bh
DEVHLP_UNSETIRQ		equ	01ch
DEVHLP_EOI		equ	031h
DEVHLP_REGISTERSTACK	equ	03ah
DEVHLP_QUEUEINIT	equ	00fh
DEVHLP_QUEUEREAD	equ	012h
DEVHLP_QUEUEWRITE	equ	011h
DEVHLP_QUEUEFLUSH	equ	010h
DEVHLP_BLOCK		equ	004h
DEVHLP_RUN		equ	005h
DEVHLP_PULLREQPKT	equ	00ah
DEVHLP_PUSHREQPKT	equ	009h
DEVHLP_VIRTTOPHYS	equ	016h
DEVHLP_LOCK		equ	013h
DEVHLP_UNLOCK		equ	014h
DEVHLP_ALLOCGDTSEL	equ	02dh
DEVHLP_PHYSTOGDTSEL	equ	02eh

		page
;******************************************************************************
;
; Start Loadable Driver Segment
;
;******************************************************************************

DGROUP		group	_DATA

_DATA		segment	word public 'DATA'

L_Header	dd      -1              	; link to next device driver
        	dw      09900h			; dev attr word for:
						;   CHAR,level 2,single OPEN
        	dw      Strat           	; "Strategy" routine entry
        	dw      0	            	; no "Inter-Driver" routine
        	db      'BYIKON$$'		; logical device name
		db	8 dup (0)		; reserved

_Drv_qDevHlpAddr dd	0			; device helper address,
						; set from @DriverStrategy
_DATA		ends
		page

_TEXT		segment word public 'CODE'

        	assume  cs:_TEXT,ds:DGROUP,es:DGROUP

;******************************************************************************
;
;	Start of Device Driver Strategy Mode Dispatch Code
;
;******************************************************************************

; start Strategy Mode Code
Strat   	proc    far     		; device Strategy routine,
						; called by OS2 kernel with
						; ES:BX = request header addr
						; DS    = DGROUP

	;call C module to process request packet
		push	es			;push request header param
		push	bx
		call	@DriverStrategy

	        ret                     	; back to OS/2 kernel

Strat   	endp

;******************************************************************************
;
;	Start of Device Driver Helper Routines
;
;******************************************************************************

;
; HlpVerifyAccess( fpMem, nLen, nType )
;
HlpVerifyAccess proc	far

		enter	0, 	0			;enter, save regs
		push	di

		mov	ax,	WPT [bp+12]		;mem segment
		mov	di,	WPT [bp+10]		;    offset
		mov	cx,	WPT [bp+8]		;    length
		mov	dx,	WPT [bp+6]		;    access type
		mov	dh,	dl			;put type in dh
		mov	dl,	DEVHLP_VERIFYACCESS	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		pop	di				;restore
		leave
		ret	8

HlpVerifyAccess endp


;
; HlpPhysToUVirt( qPhyAddr, nLen, nType, nTagType )
;
HlpPhysToUVirt proc	far

		enter	0, 	0			;enter, save regs
		push	si

		mov	ax,	WPT [bp+14]		;seg segment
		mov	bx,	WPT [bp+12]		;    offset
		mov	cx,	WPT [bp+10]		;    length
		mov	dx,	WPT [bp+8]		;    type
		mov	si,	WPT [bp+6]		;tag type
		mov	dh,	dl			;move type to dh
		mov	dl,	DEVHLP_PHYSTOUVIRT	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		mov	ax,	bx			;result offset
		mov	dx,	es			;	segment
		jnc	@F				;carry set if error
		mov	ax,	0			;error, set null
		mov	dx,	ax

@@:		pop	si				;restore
		leave
		ret	10

HlpPhysToUVirt endp


;
; HlpSetIRQ( nOffService, nIRQNum, nShareFlag )
;
HlpSetIRQ	proc	far

		enter	0, 	0			;enter, save regs

		mov	ax,	WPT [bp+10]		;int routine offset
		mov	bx,	WPT [bp+8]		;    number
		mov	dx,	WPT [bp+6]		;    share flag
		mov	dh,	dl			;move flag to dh
		mov	dl,	DEVHLP_SETIRQ		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it
		mov	ax,	0			;assume no error
		jnc	@F				;carry set on error
		mov	ax,	1			;error set flag

@@:		leave					;return to caller
		ret	6

HlpSetIRQ	endp

;
; HlpRegisterStack( nOffUsage )
;
HlpRegisterStack proc	far

		enter	0, 	0			;enter, save regs

		mov	bx,	WPT [bp+6]		;stack usage offset
		mov	dl,	DEVHLP_REGISTERSTACK	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it
		mov	ax,	0			;assume no error
		jnc	@F				;carry set on error
		mov	ax,	1			;error set flag

@@:		leave					;return to caller
		ret	2

HlpRegisterStack endp


;
; HlpUnSetIRQ( nIRQNum )
;
HlpUnSetIRQ	proc	far

		enter	0, 	0			;enter, save regs

		mov	bx,	WPT [bp+6]		;int number
		mov	dl,	DEVHLP_UNSETIRQ		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		leave					;return to caller
		ret	2

HlpUnSetIRQ	endp

;
; HlpEOI( nIRQNum )
;
HlpEOI		proc	far

		enter	0, 	0			;enter, save regs

		mov	ax,	WPT [bp+6]		;int number
		mov	dl,	DEVHLP_EOI		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		leave					;return to caller
		ret	2

HlpEOI		endp

;
; HlpQueueInit( nOffQueue )
;
HlpQueueInit	proc	far

		enter	0, 	0			;enter, save regs

		mov	bx,	WPT [bp+6]		;offset of queue
		mov	dl,	DEVHLP_QUEUEINIT	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		leave					;return to caller
		ret	2

HlpQueueInit	endp

;
; HlpQueueFlush( nOffQueue )
;
HlpQueueFlush	proc	far

		enter	0, 	0			;enter, save regs

		mov	bx,	WPT [bp+6]		;offset of queue
		mov	dl,	DEVHLP_QUEUEFLUSH	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		leave					;return to caller
		ret	2

HlpQueueFlush	endp

;
; HlpQueueRead( nOffQueue )
;
HlpQueueRead	proc	far

		enter	0, 	0			;enter, save regs

		mov	bx,	WPT [bp+6]		;offset of queue
		mov	dl,	DEVHLP_QUEUEREAD	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it
		jnc	@F				;carry set if error
		mov	ax,	-1			;set no char found

@@:		leave					;return to caller
		ret	2

HlpQueueRead	endp

;
; HlpQueueWrite( nOffQueue, nChr )
;
HlpQueueWrite	proc	far

		enter	0, 	0			;enter, save regs

		mov	bx,	WPT [bp+8]		;offset of queue
		mov	ax,	WPT [bp+6]		;char to insert
		mov	dl,	DEVHLP_QUEUEWRITE	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it
		jnc	@F				;carry set if error
		mov	ax,	-1			;set queue full

@@:		leave					;return to caller
		ret	4

HlpQueueWrite	endp

;
; HlpBlock( qEventId, qTimeout, nIntFlag )
;
HlpBlock	proc	far

		enter	0, 	0			;enter, save regs
		push	di

		mov	ax,	WPT [bp+14]		;hi id word
		mov	bx,	WPT [bp+12]		;lo
		mov	di,	WPT [bp+10]		;hi timeout word
		mov	cx,	WPT [bp+8]		;lo
		mov	dx,	WPT [bp+6]		;interrupt mode flag
		mov	dh,	dl			;move to dh
		mov	dl,	DEVHLP_BLOCK		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		lahf					;put flags in ah
							;al has error code

		pop	di
		leave					;return to caller
		ret	10

HlpBlock	endp

;
; HlpRun( qEventId )
;
HlpRun		proc	far

		enter	0, 	0			;enter, save regs

		mov	ax,	WPT [bp+8]		;hi id word
		mov	bx,	WPT [bp+6]		;lo
		mov	dl,	DEVHLP_RUN		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		leave					;return to caller
		ret	4

HlpRun		endp

;
; HlpPullReqPkt( nOffset )
;
HlpPullReqPkt	proc	far

		enter	0, 	0			;enter, save regs
		push	si

		mov	si,	WPT [bp+6]		;offset queue head
		mov	dl,	DEVHLP_PULLREQPKT	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		mov	ax,	bx			;result offset
		mov	dx,	es			;	segment
		jnc	@F				;carry set if error
		mov	ax,	0			;error, set null
		mov	dx,	ax

@@:		pop	si				;restore regs
		leave					;return to caller
		ret	2

HlpPullReqPkt	endp

;
; HlpPushReqPkt( nOffset, qPktAddr )
;
HlpPushReqPkt	proc	far

		enter	0, 	0			;enter, save regs
		push	si

		mov	si,	WPT [bp+10]		;offset queue head
		les	bx,	DPT [bp+6]		;es:bx pkt addr
		mov	dl,	DEVHLP_PUSHREQPKT	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		pop	si				;restore regs
		leave					;return to caller
		ret	6

HlpPushReqPkt	endp

;
; HlpVirtToPhys( qVirtAddr )
;
HlpVirtToPhys	proc	far

		enter	0, 	0			;enter, save regs
		push	ds
		push	si

		mov	ax,	ds			;set es=ds
		mov	es,	ax
		lds	si,	DPT [bp+6]		;ds:si virt addr
		mov	dl,	DEVHLP_VIRTTOPHYS	;get device help type
		call	es:[_Drv_qDevHlpAddr]		;call it

		mov	dx,	ax			;hi result
		mov	ax,	bx			;lo
		jnc	@F				;carry set if error
		mov	ax,	0			;error, set null
		mov	dx,	ax

@@:		pop	si				;restore regs
		pop	ds
		leave
 		ret	4

HlpVirtToPhys	endp

;
; HlpLock( nSelector, nType, nWait )
;
HlpLock		proc	far

		enter	0, 	0			;enter, save regs

		mov	ax,	WPT [bp+10]		;selector
		mov	bh,	BPT [bp+8]		;type
		mov	bl,	BPT [bp+6]		;wait flag
		mov	dl,	DEVHLP_LOCK		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		mov	dx,	ax			;hi result
		mov	ax,	bx			;lo
		jnc	@F				;carry set if error
		mov	ax,	0			;error, set null
		mov	dx,	ax

@@:		leave
 		ret	6

HlpLock		endp

;
; HlpUnLock( qHandle )
;
HlpUnLock	proc	far

		enter	0, 	0			;enter, save regs

		mov	ax,	WPT [bp+8]		;hi handle
		mov	bx,	WPT [bp+6]		;lo
		mov	dl,	DEVHLP_UNLOCK		;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		mov	ax,	0			;assume ok
		jnc	@F				;carry set if error
		mov	ax,	1			;error, set non-zero

@@:		leave
 		ret	4

HlpUnLock	endp

;
; HlpAllocGDTSel( qSelAddr, nCnt )
;
HlpAllocGDTSel 	proc	far

		enter	0, 	0			;enter, save regs
		push	di

		les	di,	DPT [bp+8]		;selector array ptr
		mov	cx,	WPT [bp+6]		;	  count
		mov	dl,	DEVHLP_ALLOCGDTSEL	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		jc	@F				;carry set if error
		xor	ax,	ax			;no error, set zero

@@:		pop	di				;restore regs
		leave
 		ret	6

HlpAllocGDTSel	endp

;
; HlpPhysToGDTSel( qPhysAddr, nLength, nSelector )
;
HlpPhysToGDTSel	proc	far

		enter	0, 	0			;enter, save regs
		push	si

		mov	ax,	WPT [bp+12]		;hi physical address
		mov	bx,	WPT [bp+10]		;lo
		mov	cx,	WPT [bp+8]		;length
		mov	si,	WPT [bp+6]		;selector
		mov	dl,	DEVHLP_PHYSTOGDTSEL	;get device help type
		call	[_Drv_qDevHlpAddr]		;call it

		jc	@F				;carry set if error
		xor	ax,	ax			;no error, set zero

@@:		pop	si				;restore regs
		leave
 		ret	8

HlpPhysToGDTSel	endp

_TEXT	ends

	end
