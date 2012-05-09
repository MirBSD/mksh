/**
 * conditionals to test for with -DDEBUG every once in a while:
 * MKSH_SMALL
 */

#if defined(EMACSFN_DEFNS)
__RCSID("$MirOS: src/bin/mksh/emacsfn.h,v 1.5.4.1 2012/05/09 21:34:29 tg Exp $");
#define FN(cname,sname)		static int x_##cname(int);
#elif defined(EMACSFN_ENUMS)
#define FN(cname,sname)		XFUNC_##cname,
#define F0(cname,sname)		XFUNC_##cname = 0,
#elif defined(EMACSFN_PTRS)
#define FN(cname,sname)		x_##cname,
#elif defined(EMACSFN_STRS)
#define FN(cname,sname)		sname,
#endif

#ifndef F0
#define F0 FN
#endif

F0(abort, "abort")
FN(beg_hist, "beginning-of-history")
FN(cls, "clear-screen")
FN(comment, "comment")
FN(comp_comm, "complete-command")
FN(comp_file, "complete-file")
FN(comp_list, "complete-list")
FN(complete, "complete")
FN(del_back, "delete-char-backward")
FN(del_bword, "delete-word-backward")
FN(del_char, "delete-char-forward")
FN(del_fword, "delete-word-forward")
FN(del_line, "kill-line")
FN(draw_line, "redraw")
#ifndef MKSH_SMALL
FN(edit_line, "edit-line")
#endif
FN(end_hist, "end-of-history")
FN(end_of_text, "eot")
FN(enumerate, "list")
FN(eot_del, "eot-or-delete")
FN(error, "error")
FN(expand, "expand-file")
#ifndef MKSH_SMALL
FN(fold_capitalise, "capitalize-word")
FN(fold_lower, "downcase-word")
FN(fold_upper, "upcase-word")
#endif
FN(goto_hist, "goto-history")
#ifndef MKSH_SMALL
FN(ins_string, "macro-string")	//XXX die!
#endif
FN(insert, "auto-insert")
FN(kill, "kill-to-eol")
FN(kill_region, "kill-region")
FN(list_comm, "list-command")
FN(list_file, "list-file")
FN(literal, "quote")
FN(meta1, "prefix-1")
FN(meta2, "prefix-2")
FN(meta_yank, "yank-pop")
FN(mv_back, "backward-char")
FN(mv_begin, "beginning-of-line")
FN(mv_bword, "backward-word")
FN(mv_end, "end-of-line")
FN(mv_forw, "forward-char")
FN(mv_fword, "forward-word")
FN(newline, "newline")
FN(next_com, "down-history")
FN(nl_next_com, "newline-and-next")
FN(noop, "no-op")
FN(prev_com, "up-history")
FN(prev_histword, "prev-hist-word")
FN(search_char_back, "search-character-backward")
FN(search_char_forw, "search-character-forward")
FN(search_hist, "search-history")
#ifndef MKSH_SMALL
FN(search_hist_dn, "search-history-down")
FN(search_hist_up, "search-history-up")
#endif
FN(set_arg, "set-arg")
FN(set_mark, "set-mark-command")
FN(transpose, "transpose-chars")
FN(version, "version")
FN(xchg_point_mark, "exchange-point-and-mark")
FN(yank, "yank")

#if defined(EMACSFN_DEFNS)
/* these must be kept sorted and must not contain macros */
#define XB(fn)		XF_CONST, XFUNC_##fn
static const uint8_t x_defbindings0[] = {
	CTRL('A'), 0, XB(mv_begin),
	CTRL('B'), 0, XB(mv_back),
	CTRL('D'), 0, XB(eot_del),
	CTRL('E'), 0, XB(mv_end),
	CTRL('F'), 0, XB(mv_forw),
	CTRL('G'), 0, XB(abort),
	CTRL('H'), 0, XB(del_back),
	CTRL('I'), 0, XB(comp_list),
	CTRL('J'), 0, XB(newline),
	CTRL('K'), 0, XB(kill),
	CTRL('L'), 0, XB(draw_line),
	CTRL('M'), 0, XB(newline),
	CTRL('N'), 0, XB(next_com),
	CTRL('O'), 0, XB(nl_next_com),
	CTRL('P'), 0, XB(prev_com),
	CTRL('R'), 0, XB(search_hist),
	CTRL('T'), 0, XB(transpose),
	CTRL('V'), 0, XB(literal),
	CTRL('W'), 0, XB(kill_region),
	CTRL('X'), 0, XF_PREFIX | XB(meta2),
	CTRL('Y'), 0, XB(yank),
	CTRL('['), 0, XF_PREFIX | XB(meta1),
	CTRL(']'), 0, XB(search_char_forw),
	CTRL('^'), 0, XB(literal),
	CTRL('_'), 0, XB(end_of_text),
	CTRL('?'), 0, XB(del_back),
	0 /* max. 26 elements */
};

static const uint8_t x_defbindings1[] = {
	CTRL('H'), 0, XB(del_bword),
	CTRL('L'), 0, XB(cls),
	CTRL('V'), 0, XB(version),
	CTRL('X'), 0, XB(comp_file),
	CTRL('['), 0, XB(complete),
	CTRL(']'), 0, XB(search_char_back),
	' ', 0, XB(set_mark),
	'#', 0, XB(comment),
	'*', 0, XB(expand),
	'.', 0, XB(prev_histword),
	'0', 0, XF_NOBIND | XB(set_arg),
	'1', 0, XF_NOBIND | XB(set_arg),
	'2', 0, XF_NOBIND | XB(set_arg),
	'3', 0, XF_NOBIND | XB(set_arg),
	'4', 0, XF_NOBIND | XB(set_arg),
	'5', 0, XF_NOBIND | XB(set_arg),
	'6', 0, XF_NOBIND | XB(set_arg),
	'7', 0, XF_NOBIND | XB(set_arg),
	'8', 0, XF_NOBIND | XB(set_arg),
	'9', 0, XF_NOBIND | XB(set_arg),
	'<', 0, XB(beg_hist),
	'=', 0, XB(comp_list),
	'>', 0, XB(end_hist),
	'?', 0, XB(enumerate),
#ifndef MKSH_SMALL
	'C', 0, XB(fold_capitalise),
	'L', 0, XB(fold_lower),
#endif
	'O', 0, XF_PREFIX | XB(meta2),
#ifndef MKSH_SMALL
	'U', 0, XB(fold_upper),
#endif
	'[', 0, XF_PREFIX | XB(meta2),
	'_', 0, XB(prev_histword),
	'b', 0, XB(mv_bword),
#ifndef MKSH_SMALL
	'c', 0, XB(fold_capitalise),
#endif
	'd', 0, XB(del_fword),
	'f', 0, XB(mv_fword),
	'g', 0, XB(goto_hist),
	'h', 0, XB(del_bword),
#ifndef MKSH_SMALL
	'l', 0, XB(fold_lower),
	'u', 0, XB(fold_upper),
#endif
	'y', 0, XB(meta_yank),
	CTRL('?'), 0, XB(del_bword),
	0 /* max. 40 elements */
};

static const uint8_t x_defbindings2[] = {
	CTRL('X'), 0, XB(xchg_point_mark),
	CTRL('Y'), 0, XB(list_file),
	CTRL('['), 0, XB(comp_comm),
#ifndef MKSH_SMALL
	'1', ';', '3', 'C', 0, XB(mv_fword),
	'1', ';', '3', 'D', 0, XB(mv_bword),
	'1', ';', '5', 'C', 0, XB(mv_fword),
	'1', ';', '5', 'D', 0, XB(mv_bword),
	'1', '~', 0, XB(mv_begin),
	'3', '~', 0, XB(del_char),
	'4', '~', 0, XB(mv_end),
	'5', '~', 0, XB(search_hist_up),
	'6', '~', 0, XB(search_hist_dn),
	'7', '~', 0, XB(mv_begin),
	'8', '~', 0, XB(mv_end),
#endif
	'?', 0, XB(list_comm),
	'A', 0, XB(prev_com),
	'B', 0, XB(next_com),
	'C', 0, XB(mv_forw),
	'D', 0, XB(mv_back),
#ifndef MKSH_SMALL
	'F', 0, XB(mv_end),
	'H', 0, XB(mv_begin),
	'e', 0, XB(edit_line),
#endif
	0 /* max. 22 elements */
};
#undef XB
#endif /* EMACSFN_DEFNS */

#undef FN
#undef F0
#undef EMACSFN_DEFNS
#undef EMACSFN_ENUMS
#undef EMACSFN_PTRS
#undef EMACSFN_STRS
