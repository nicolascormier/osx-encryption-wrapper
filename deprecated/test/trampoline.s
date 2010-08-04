/*
 * trampoline.s
 * test
 *
 * Created by nico on 2/10/08.
 * Copyright 2008 Nicolas CORMIER. All rights reserved.
 */


.section __enkript,trampoline
_toto:
    call _enkript_prologue
    jmp start