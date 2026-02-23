#pragma once
#include "lisp.h"

extern Lisp_Object Vminibuffer_list;
extern Lisp_Object last_minibuf_string;
extern void move_minibuffers_onto_frame (struct frame *, Lisp_Object, bool);
extern bool is_minibuffer (EMACS_INT, Lisp_Object);
extern EMACS_INT this_minibuffer_depth (Lisp_Object);
extern EMACS_INT minibuf_level;
extern Lisp_Object get_minibuffer (EMACS_INT);
extern void init_minibuf_once (void);
extern void set_initial_minibuffer_mode (void);
extern void syms_of_minibuf (void);
extern void barf_if_interaction_inhibited (void);
