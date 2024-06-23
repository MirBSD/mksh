#!/bin/sh
rcsid='$MirOS: src/bin/mksh/FAQ2HTML.sh,v 1.8 2023/12/13 10:07:19 tg Exp $'
#-
# Copyright © 2020, 2023
#	mirabilos <m@mirbsd.org>
#
# Provided that these terms and disclaimer and all copyright notices
# are retained or reproduced in an accompanying document, permission
# is granted to deal in this work without restriction, including un‐
# limited rights to use, publicly perform, distribute, sell, modify,
# merge, give away, or sublicence.
#
# This work is provided “AS IS” and WITHOUT WARRANTY of any kind, to
# the utmost extent permitted by applicable law, neither express nor
# implied; without malicious intent or gross negligence. In no event
# may a licensor, author or contributor be held liable for indirect,
# direct, other damage, loss, or other issues arising in any way out
# of dealing in the work, even if advised of the possibility of such
# damage or existence of a defect, except proven that it results out
# of said person’s immediate fault when using the work as intended.
#-

die() {
	echo >&2 "E: $*"
	exit 1
}

LC_ALL=C; LANGUAGE=C
export LC_ALL; unset LANGUAGE
nl='
'
srcdir=`dirname "$0" 2>/dev/null` || die 'cannot find srcdir'

p=--posix
sed $p -e q </dev/null >/dev/null 2>&1 || p=

v=$1
if test -z "$v"; then
	v=`sed $p -n '/^#define MKSH_VERSION "\(.*\)"$/s//\1/p' "$srcdir"/sh.h`
fi
test -n "$v" || die 'cannot find version'
src_id=`sed $p -n '/^RCSID: /s///p' "$srcdir"/mksh.faq`
# sanity check
case x$src_id in
x)
	die 'cannot find RCSID'
	;;
*"$nl"*)
	die 'more than one RCSID in mksh.faq?'
	;;
esac

sed $p \
    -e '/^RCSID: \$/s/^.*$/----/' \
    -e 's!@@RELPATH@@!http://www.mirbsd.org/!g' \
    -e 's^	<span style="display:none;">	</span>' \
    "$srcdir"/mksh.faq >FAQ.tm1 || die 'sed (1)'
tr '\n' '' <FAQ.tm1 >FAQ.tm2 || die 'tr (2)'
sed $p \
    -e 'sg' \
    -e 's----g' \
    -e 's\([^]*\)\1g' \
    -e 's\([^]*\)\1g' \
    -e 's\([^]*\)*ToC: \([^]*\)Title: \([^]*\)\([^]*\)\{0,1\}</div><h2 id="\2"><a href="#\2">\3</a></h2><div>g' \
    -e 's[^]*</div><div>g' \
    -e 's^</div>*' \
    -e 's$</div>' \
    -e 's<><error><>g' \
    -e 'sg' <FAQ.tm2 >FAQ.tm3 || die 'sed (3)'
tr '' '\n' <FAQ.tm3 >FAQ.tm4 || die 'tr (4)'

exec >FAQ.tm5
cat <<EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
 "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en"><head>
 <meta http-equiv="content-type" content="text/html; charset=utf-8" />
 <title>mksh $v FAQ (local copy)</title>
 <meta name="source" content="$src_id" />
 <meta name="generator" content="$rcsid" />
 <style type="text/css"><!--/*--><![CDATA[/*><!--*/
 .boxhead {
	margin-bottom:0px;
 }

 .boxtext {
	border:4px ridge green;
	margin:0px 24px 0px 18px;
	padding:2px 3px 2px 3px;
 }

 .boxfoot {
	margin-top:0px;
 }

 h2:before {
	content:"🔗 ";
 }

 a[href^="ftp://"]:after,
 a[href^="http://"]:after,
 a[href^="https://"]:after,
 a[href^="irc://"]:after,
 a[href^="mailto:"]:after,
 a[href^="news:"]:after,
 a[href^="nntp://"]:after {
	content:"⏍";
	color:#FF0000;
	vertical-align:super;
	margin:0 0 0 1px;
 }

 pre {
	/*      ↑   →   ↓    ←   */
	margin:0px 9px 0px 15px;
 }

 tt {
	white-space:nowrap;
 }
 /*]]>*/--></style>
</head><body>
<p>Note: Links marked like <a href="irc://irc.mirbsd.org/!/bin/mksh">this
 one to the mksh IRC channel</a> connect to external resources.</p>
<p>⚠ <b>Notice:</b> the website will have <a
 href="http://www.mirbsd.org/mksh-faq.htm">the latest version of the
 mksh FAQ</a> online.</p>
<h1>Table of Contents</h1>
<ul>
EOF
sed $p -n \
    '/^<h2 id="\([^"]*"\)><a[^>]*\(>.*<\/a><\/\)h2>$/s//<li><a href="#\1\2li>/p' \
    <FAQ.tm4 || die 'sed (ToC)'
cat <<EOF
</ul>

<h1>Frequently Asked Questions</h1>
EOF
cat FAQ.tm4 - <<EOF
<h1>Imprint</h1>
<p>This offline HTML page for mksh $v was automatically generated
 from the sources.</p>
</body></html>
EOF
exec >/dev/null
rm FAQ.tm1 FAQ.tm2 FAQ.tm3 FAQ.tm4
mv FAQ.tm5 FAQ.htm || die 'final mv'
exit 0
