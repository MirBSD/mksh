# $MirOS: src/bin/mksh/check.pl,v 1.58 2025/04/26 22:42:59 tg Exp $
# $OpenBSD: th,v 1.1 2013/12/02 20:39:44 millert Exp $
#-
# Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011,
#		2012, 2013, 2014, 2015, 2017, 2021, 2025
#	mirabilos <m$(date +%Y)@mirbsd.de>
#
# Provided that these terms and disclaimer and all copyright notices
# are retained or reproduced in an accompanying document, permission
# is granted to deal in this work without restriction, including un-
# limited rights to use, publicly perform, distribute, sell, modify,
# merge, give away, or sublicence.
#
# This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
# the utmost extent permitted by applicable law, neither express nor
# implied; without malicious intent or gross negligence. In no event
# may a licensor, author or contributor be held liable for indirect,
# direct, other damage, loss, or other issues arising in any way out
# of dealing in the work, even if advised of the possibility of such
# damage or existence of a defect, except proven that it results out
# of said person's immediate fault when using the work as intended.
#-
# Example test:
#		name: a-test
#		description:
#			a test to show how tests are done
#		arguments: !-x!-f!
#		stdin:
#			echo -n *
#			false
#		expected-stdout: !
#			*
#		expected-stderr:
#			+ echo -n *
#			+ false
#		expected-exit: 1
#		---
#	This runs the test-program (eg, mksh) with the arguments -x and -f,
#	standard input is a file containing "echo hi*\nfalse\n". The program
#	is expected to produce "hi*" (no trailing newline) on standard output,
#	"+ echo hi*\n+false\n" on standard error, and an exit code of 1.
#
# The test-program is invoked with a new environment vector by default which
# is composed of the following parameters from the inherited environ if set:
#	HOME, LD_LIBRARY_PATH, LOCPATH, LOGNAME,
#	PATH, PERLIO, SHELL, UNIXMODE, UNIXROOT, USER
# Additionally, some parameters are set as follows (without the quotes):
#	CYGWIN to "nodosfilewarning"
#	ENV to "/nonexisting"
#	LANG to "C"
#	__perlname to $^X (perlexe)
# Any -e option arguments are added, or, if no equals sign is given, removed.
# Each test's env-setup (see below) is processed in the same way affecting
# the environment of that test. Finally, in the per-test environ, __progname
# is set to the -p argument or (if -P) its first word.
#
# Format of test files:
# - blank lines and lines starting with # are ignored
# - a test file contains a series of tests
# - a test is a series of tag:value pairs ended with a "---" line
#   (leading/trailing spaces are stripped from the first line of value)
# - test tags are:
#	Tag			Flag	Description
#	-----			----	-----------
#	name			r	The name of the test; should be unique
#	description		m	What test does
#	arguments		M	Arguments to pass to the program;
#					default is no arguments.
#	script			m	Value is written to a file which
#					is passed as an argument to the program
#					(after the arguments from arguments)
#	stdin			m	Value is written to a file which is
#					used as standard-input for the program;
#					default is to use /dev/null.
#	perl-setup		m	Value is a perl script which is executed
#					just before the test is run. Try to
#					avoid using this...
#	perl-cleanup		m	Value is a perl script which is executed
#					just after the test is run. Try to
#					avoid using this...
#	env-setup		M	Value is a list of elements that are
#					removed from the per-test environment
#					(if no equals sign present) or added
#					to it if of the NAME=VALUE form, after
#					substituting @utflocale@ for -U option.
#					For more about test environ see above.
#	file-setup		mps	Used to create files, directories
#					and symlinks. First word is either
#					file, dir or symlink; second word is
#					permissions; this is followed by a
#					quoted word that is the name of the
#					file; the end-quote should be followed
#					by a newline, then the file data
#					(if any). The first word may be
#					preceded by a ! to strip the trailing
#					newline in a symlink.
#	file-result		mps	Used to verify a file, symlink or
#					directory is created correctly.
#					The first word is either
#					file, dir or symlink; second word is
#					expected permissions; third word
#					is user-id; fourth is group-id;
#					fifth is "exact" or "pattern"
#					indicating whether the file contents
#					which follow is to be matched exactly
#					or if it is a regular expression.
#					The fifth argument is the quoted name
#					of the file that should be created.
#					The end-quote should be followed
#					by a newline, then the file data
#					(if any). The first word may be
#					preceded by a ! to strip the trailing
#					newline in the file contents.
#					The permissions, user and group fields
#					may be * meaning accept any value.
#	time-limit			Time limit - the program is sent a
#					SIGKILL N seconds. Default is no
#					limit.
#	expected-fail			'yes' if the test is expected to fail.
#	expected-exit			expected exit code. Can be a number,
#					or a C expression using the variables
#					e, s and w (exit code, termination
#					signal, and status code).
#	expected-stdout		m	What the test should generate on stdout;
#					default is to expect no output.
#	expected-stdout-pattern	m	A perl pattern which matches the
#					expected output.
#	expected-stderr		m	What the test should generate on stderr;
#					default is to expect no output.
#	expected-stderr-pattern	m	A perl pattern which matches the
#					expected standard error.
#	category		m	Specify a comma separated list of
#					'categories' of program that the test
#					is to be run for. A category can be
#					negated by prefixing the name with a !.
#					The idea is that some tests in a
#					test suite may apply to a particular
#					program version and shouldn't be run
#					on other versions. The category(s) of
#					the program being tested can be
#					specified on the command line.
#					One category os:XXX is predefined
#					(XXX is the operating system name,
#					eg, linux, dec_osf).
#	need-ctty			'yes' if the test needs a ctty, run
#					with -C regress:no-ctty to disable.
#	info-pre			shown before the test
#	info-post			shown after the test
# Flag meanings:
#	r	tag is required (eg, a test must have a name tag).
#	m	value can be multiple lines. Lines must be prefixed with
#		a tab. If the value part of the initial tag:value line is
#			- empty: the initial blank line is stripped.
#			- a lone !: the last newline in the value is stripped;
#	M	value can be multiple lines (prefixed by a tab) and consists
#		of multiple fields, delimited by a field separator character.
#		The value must start and end with the f-s-c.
#	p	tag takes parameters (used with m).
#	s	tag can be used several times.

# require Config only if it exists
# pull EINTR from POSIX.pm or Errno.pm if they exist
# otherwise just skip it
BEGIN {
	eval {
		require Config;
		import Config;
		1;
	};
	$EINTR = 0;
	eval {
		require POSIX;
		$EINTR = POSIX::EINTR();
	};
	if ($@) {
		eval {
			require Errno;
			$EINTR = Errno::EINTR();
		} or do {
			$EINTR = 0;
		};
	}
};

use Getopt::Std;

$os = defined $^O ? $^O : 'unknown';

($prog = $0) =~ s#.*/##;

$Usage = <<EOF ;
Usage: $prog [-Pv] [-C cat] [-e e=v] [-p prog] [-s fn] [-T dir] \
       [-t tmo] [-U lcl] name ...
	-C c	Specify the comma separated list of categories the program
		belongs to (see category field).
	-e e=v	Set the environment variable e to v for all tests
		(if no =v is given, the current value is used)
		Only one -e option can be given at the moment, sadly.
	-P	program (-p) string has multiple words, and the program is in
		the path (kludge option)
	-p p	Use p as the program to test
	-s s	Read tests from file s; if s is a directory, it is recursively
		scanned for test files (which end in .t).
	-T dir	Use dir instead of /tmp to hold temporary files
	-t t	Use t as default time limit for tests (default is unlimited)
	-U lcl	Specify UTF-8 locale (e.g. en_US.UTF-8) instead of the default
	-v	Verbose mode: print reason test failed.
	name	specifies the name of the test(s) to run; if none are
		specified, all tests are run.
EOF

# See comment above for flag meanings
%test_fields = (
	'name',				'r',
	'description',			'm',
	'arguments',			'M',
	'script',			'm',
	'stdin',			'm',
	'perl-setup',			'm',
	'perl-cleanup',			'm',
	'env-setup',			'M',
	'file-setup',			'mps',
	'file-result',			'mps',
	'time-limit',			'',
	'expected-fail',		'',
	'expected-exit',		'',
	'expected-stdout',		'm',
	'expected-stdout-pattern',	'm',
	'expected-stderr',		'm',
	'expected-stderr-pattern',	'm',
	'category',			'm',
	'need-ctty',			'',
	'need-pass',			'',
	'info-pre',			'',
	'info-post',			'',
	);
# Filled in by read_test()
%internal_test_fields = (
	':full-name', 1,		# file:name
	':long-name', 1,		# dir/file:lineno:name
	);

# Categories of the program under test. Provide the current
# os by default.
%categories = (
	"os:$os", '1'
	);

$nfailed = 0;
$nifailed = 0;
$nxfailed = 0;
$npassed = 0;
$nxpassed = 0;

%known_tests = ();

if (!getopts('C:Ee:Pp:s:T:t:U:v')) {
    print STDERR $Usage;
    exit 1;
}

die "$prog: no program specified (use -p)\n" if !defined $opt_p;
die "$prog: no test set specified (use -s)\n" if !defined $opt_s;
$test_prog = $opt_p;
$verbose = defined $opt_v && $opt_v;
$is_ebcdic = defined $opt_E && $opt_E;
$test_set = $opt_s;
$temp_base = $opt_T || "/tmp";
$utflocale = $opt_U || (($os eq "hpux") ? "en_US.utf8" : ($os eq "darwin") ? "UTF-8" : "C.UTF-8");
if (defined $opt_t) {
    die "$prog: bad -t argument (should be number > 0): $opt_t\n"
	if $opt_t !~ /^\d+$/ || $opt_t <= 0;
    $default_time_limit = $opt_t;
}
$program_kludge = defined $opt_P ? $opt_P : 0;

if ($is_ebcdic) {
	$categories{'shell:ebcdic-yes'} = 1;
	$categories{'shell:ascii-no'} = 1;
} else {
	$categories{'shell:ebcdic-no'} = 1;
	$categories{'shell:ascii-yes'} = 1;
}

if (defined $opt_C) {
    foreach $c (split(',', $opt_C)) {
	$c =~ s/\s+//;
	die "$prog: categories can't be negated on the command line\n"
	    if ($c =~ /^!/);
	$categories{$c} = 1;
    }
}

# Note which tests are to be run.
%do_test = ();
grep($do_test{$_} = 1, @ARGV);
$all_tests = @ARGV == 0;

# Set up a very minimal environment
%new_env = ();
foreach $env (('HOME', 'LD_LIBRARY_PATH', 'LOCPATH', 'LOGNAME',
  'PATH', 'PERLIO', 'SHELL', 'UNIXMODE', 'UNIXROOT', 'USER')) {
    $new_env{$env} = $ENV{$env} if defined $ENV{$env};
}
$new_env{'CYGWIN'} = 'nodosfilewarning';
$new_env{'ENV'} = '/nonexisting';
$new_env{'LANG'} = 'C';

$pn = $Config{perlpath};
if ($pn ne '' and $os ne 'VMS' and $pn !~ m/$Config{_exe}$/i) {
	$pn .= $Config{_exe};
}
if (($pn eq '' or ! -f $pn or ! -x $pn) and -f $^X and -x $^X) {
	$pn = $^X;
}
if ($pn eq '' or ! -f $pn or ! -x $pn) {
	foreach $pathelt (split /:/, $ENV{'PATH'}) {
		chomp($pathelt = `pwd`) if $pathelt eq '';
		my $x = $pathelt . '/' . $^X;
		next unless -f $x and -x $x;
		$pn = $x;
		last;
	}
}
$pn = $^X if ($pn eq '');
$new_env{'__perlname'} = $pn;

if (defined $opt_e) {
    # XXX need a way to allow many -e arguments...
    if ($opt_e =~ /^([a-zA-Z_]\w*)(|=(.*))$/) {
	$new_env{$1} = $2 eq '' ? $ENV{$1} : $3;
    } else {
	die "$0: bad -e argument: $opt_e\n";
    }
}
%old_env = %ENV;

chop($pwd = `pwd 2>/dev/null`);
die "$prog: couldn't get current working directory\n" if $pwd eq '';
die "$prog: couldn't cd to $pwd - $!\n" if !chdir($pwd);

die "$prog: couldn't cd to $temp_base - $!\n" if !chdir($temp_base);
die "$prog: couldn't get temporary directory base\n" unless -d '.';
$temps = sprintf("chk%d-%d.", $$, time());
$tempi = 0;
until (mkdir(($tempdir = sprintf("%s%03d", $temps, $tempi)), 0700)) {
    die "$prog: couldn't get temporary directory\n" if $tempi++ >= 999;
}
die "$prog: couldn't cd to $tempdir - $!\n" if !chdir($tempdir);
chop($temp_dir = `pwd 2>/dev/null`);
die "$prog: couldn't get temporary directory\n" if $temp_dir eq '';
die "$prog: couldn't cd to $pwd - $!\n" if !chdir($pwd);

if (!$program_kludge) {
    $test_prog = "$pwd/$test_prog" if (substr($test_prog, 0, 1) ne '/') &&
      ($os ne 'os2' || substr($test_prog, 1, 1) ne ':');
    die "$prog: $test_prog is not executable - bye\n"
      if (! -x $test_prog && $os ne 'os2');
}

@trap_sigs = ('TERM', 'QUIT', 'INT', 'PIPE', 'HUP');
@SIG{@trap_sigs} = ('cleanup_exit') x @trap_sigs;
$child_kill_ok = 0;
$SIG{'ALRM'} = 'catch_sigalrm';

$| = 1;

# Create temp files
$temps = "${temp_dir}/rts";
$tempi = "${temp_dir}/rti";
$tempo = "${temp_dir}/rto";
$tempe = "${temp_dir}/rte";
$tempdir = "${temp_dir}/rtd";
mkdir($tempdir, 0700) or die "$prog: couldn't mkdir $tempdir - $!\n";

if (-d $test_set) {
    $file_prefix_skip = length($test_set) + 1;
    $ret = &process_test_dir($test_set);
} else {
    $file_prefix_skip = 0;
    $ret = &process_test_file($test_set);
}
&cleanup_exit() if !defined $ret;

$tot_failed = $nfailed + $nifailed + $nxfailed;
$tot_passed = $npassed + $nxpassed;
if ($tot_failed || $tot_passed) {
    print "Total failed: $tot_failed";
    print " ($nifailed ignored)" if $nifailed;
    print " ($nxfailed unexpected)" if $nxfailed;
    print " (as expected)" if $nfailed && !$nxfailed && !$nifailed;
    print " ($nfailed expected)" if $nfailed && ($nxfailed || $nifailed);
    print "\nTotal passed: $tot_passed";
    print " ($nxpassed unexpected)" if $nxpassed;
    print "\n";
}

&cleanup_exit('ok');

sub
cleanup_exit
{
    local($sig, $exitcode) = ('', 1);

    if ($_[0] eq 'ok') {
	unless ($nxfailed) {
		$exitcode = 0;
	} else {
		$exitcode = 1;
	}
    } elsif ($_[0] ne '') {
	$sig = $_[0];
    }

    unlink($tempi, $tempo, $tempe, $temps);
    &scrub_dir($tempdir) if defined $tempdir;
    rmdir($tempdir) if defined $tempdir;
    rmdir($temp_dir) if defined $temp_dir;

    if ($sig) {
	$SIG{$sig} = 'DEFAULT';
	kill $sig, $$;
	return;
    }
    exit $exitcode;
}

sub
catch_sigalrm
{
    $SIG{'ALRM'} = 'catch_sigalrm';
    kill(9, $child_pid) if $child_kill_ok;
    $child_killed = 1;
}

sub
process_test_dir
{
    local($dir) = @_;
    local($ret, $file);
    local(@todo) = ();

    if (!opendir(DIR, $dir)) {
	print STDERR "$prog: can't open directory $dir - $!\n";
	return undef;
    }
    while (defined ($file = readdir(DIR))) {
	push(@todo, $file) if $file =~ /^[^.].*\.t$/;
    }
    closedir(DIR);

    foreach $file (@todo) {
	$file = "$dir/$file";
	if (-d $file) {
	    $ret = &process_test_dir($file);
	} elsif (-f _) {
	    $ret = &process_test_file($file);
	}
	last if !defined $ret;
    }

    return $ret;
}

sub
process_test_file
{
    local($file) = @_;
    local($ret);

    if (!open(IN, $file)) {
	print STDERR "$prog: can't open $file - $!\n";
	return undef;
    }
    binmode(IN);
    while (1) {
	$ret = &read_test($file, IN, *test);
	last if !defined $ret || !$ret;
	next if !$all_tests && !$do_test{$test{'name'}};
	next if !&category_check(*test);
	$ret = &run_test(*test);
	last if !defined $ret;
    }
    close(IN);

    return $ret;
}

sub
run_test
{
    local(*test) = @_;
    local($name) = $test{':full-name'};

    if ($test{'info-pre'} ne '') {
	print "info v[$test{'info-pre'}]\n";
    }

    return undef if !&scrub_dir($tempdir);

    if (defined $test{'stdin'}) {
	return undef if !&write_file($tempi, $test{'stdin'});
	$ifile = $tempi;
    } else {
	$ifile = '/dev/null';
    }

    if (defined $test{'script'}) {
	return undef if !&write_file($temps, $test{'script'});
    }

    if (!chdir($tempdir)) {
	print STDERR "$prog: couldn't cd to $tempdir - $!\n";
	return undef;
    }

    if (defined $test{'file-setup'}) {
	local($i);
	local($type, $perm, $rest, $c, $len, $name);

	for ($i = 0; $i < $test{'file-setup'}; $i++) {
	    $val = $test{"file-setup:$i"};

	    # format is: type perm "name"
	    ($type, $perm, $rest) =
		split(' ', $val, 3);
	    $c = substr($rest, 0, 1);
	    $len = index($rest, $c, 1) - 1;
	    $name = substr($rest, 1, $len);
	    $rest = substr($rest, 2 + $len);
	    $perm = oct($perm) if $perm =~ /^\d+$/;
	    if ($type eq 'file') {
		return undef if !&write_file($name, $rest);
		if (!chmod($perm, $name)) {
		    print STDERR
		  "$prog:$test{':long-name'}: can't chmod $perm $name - $!\n";
		    return undef;
		}
	    } elsif ($type eq 'dir') {
		if (!mkdir($name, $perm)) {
		    print STDERR
		  "$prog:$test{':long-name'}: can't mkdir $perm $name - $!\n";
		    return undef;
		}
	    } elsif ($type eq 'symlink') {
		local($oumask) = umask($perm);
		local($ret) = symlink($rest, $name);
		umask($oumask);
		if (!$ret) {
		    print STDERR
	    "$prog:$test{':long-name'}: couldn't create symlink $name - $!\n";
		    return undef;
		}
	    }
	}
    }

    if (defined $test{'perl-setup'}) {
	eval $test{'perl-setup'};
	if ($@ ne '') {
	    print STDERR "$prog:$test{':long-name'}: error running perl-setup - $@\n";
	    return undef;
	}
    }

    $pid = fork;
    if (!defined $pid) {
	print STDERR "$prog: can't fork - $!\n";
	return undef;
    }
    if (!$pid) {
	@SIG{@trap_sigs} = ('DEFAULT') x @trap_sigs;
	$SIG{'ALRM'} = 'DEFAULT';
	if (defined $test{'env-setup'}) {
	    local($var, $val, $i);

	    foreach $var (split(substr($test{'env-setup'}, 0, 1),
		$test{'env-setup'}))
	    {
		$i = index($var, '=');
		next if $i == 0 || $var eq '';
		if ($i < 0) {
		    delete $new_env{$var};
		} else {
		    $new_env{substr($var, 0, $i)} = substr($var, $i + 1);
		}
	    }
	}
	if (!open(STDIN, "< $ifile")) {
		print STDERR "$prog: couldn't open $ifile in child - $!\n";
		kill('TERM', $$);
	}
	binmode(STDIN);
	if (!open(STDOUT, "> $tempo")) {
		print STDERR "$prog: couldn't open $tempo in child - $!\n";
		kill('TERM', $$);
	}
	binmode(STDOUT);
	if (!open(STDERR, "> $tempe")) {
		print STDOUT "$prog: couldn't open $tempe in child - $!\n";
		kill('TERM', $$);
	}
	binmode(STDERR);
	if ($program_kludge) {
	    @argv = split(' ', $test_prog);
	} else {
	    @argv = ($test_prog);
	}
	if (defined $test{'arguments'}) {
		push(@argv,
		     split(substr($test{'arguments'}, 0, 1),
			   substr($test{'arguments'}, 1)));
	}
	push(@argv, $temps) if defined $test{'script'};

	#XXX realpathise, use command -v/whence -p/which, or sth. like that
	#XXX if !$program_kludge, we get by with not doing it for now tho
	$new_env{'__progname'} = $argv[0];

	# The following doesn't work with perl5...  Need to do it explicitly - yuck.
	#%ENV = %new_env;
	foreach $k (keys(%ENV)) {
	    delete $ENV{$k};
	}
	$ENV{$k} = $v while ($k,$v) = each %new_env;

	exec { $argv[0] } @argv;
	print STDERR "$prog: couldn't execute $test_prog - $!\n";
	kill('TERM', $$);
	exit(95);
    }
    $child_pid = $pid;
    $child_killed = 0;
    $child_kill_ok = 1;
    alarm($test{'time-limit'}) if defined $test{'time-limit'};
    while (1) {
	$xpid = waitpid($pid, 0);
	$child_kill_ok = 0;
	if ($xpid < 0) {
	    if ($EINTR) {
		next if $! == $EINTR;
	    }
	    print STDERR "$prog: error waiting for child - $!\n";
	    return undef;
	}
	last;
    }
    $status = $?;
    alarm(0) if defined $test{'time-limit'};

    $failed = 0;
    $why = '';

    if ($child_killed) {
	$failed = 1;
	$why .= "\ttest timed out (limit of $test{'time-limit'} seconds)\n";
    }

    $ret = &eval_exit($test{'long-name'}, $status, $test{'expected-exit'});
    return undef if !defined $ret;
    if (!$ret) {
	local($expl);

	$failed = 1;
	if (($status & 0xff) == 0x7f) {
	    $expl = "stopped";
	} elsif (($status & 0xff)) {
	    $expl = "signal " . ($status & 0x7f);
	} else {
	    $expl = "exit-code " . (($status >> 8) & 0xff);
	}
	$why .=
	"\tunexpected exit status $status ($expl), expected $test{'expected-exit'}\n";
    }

    $tmp = &check_output($test{'long-name'}, $tempo, 'stdout',
		$test{'expected-stdout'}, $test{'expected-stdout-pattern'});
    return undef if !defined $tmp;
    if ($tmp ne '') {
	$failed = 1;
	$why .= $tmp;
    }

    $tmp = &check_output($test{'long-name'}, $tempe, 'stderr',
		$test{'expected-stderr'}, $test{'expected-stderr-pattern'});
    return undef if !defined $tmp;
    if ($tmp ne '') {
	$failed = 1;
	$why .= $tmp;
    }

    $tmp = &check_file_result(*test);
    return undef if !defined $tmp;
    if ($tmp ne '') {
	$failed = 1;
	$why .= $tmp;
    }

    if (defined $test{'perl-cleanup'}) {
	eval $test{'perl-cleanup'};
	if ($@ ne '') {
	    print STDERR "$prog:$test{':long-name'}: error running perl-cleanup - $@\n";
	    return undef;
	}
    }

    if (!chdir($pwd)) {
	print STDERR "$prog: couldn't cd to $pwd - $!\n";
	return undef;
    }

    if ($failed) {
	if (!$test{'expected-fail'}) {
	    if ($test{'need-pass'}) {
		print "FAIL $name\n";
		$nxfailed++;
	    } else {
		print "FAIL $name (ignored)\n";
		$nifailed++;
	    }
	} else {
	    print "fail $name (as expected)\n";
	    $nfailed++;
	}
	$why = "\tDescription"
		. &wrap_lines($test{'description'}, " (missing)\n")
		. $why;
    } elsif ($test{'expected-fail'}) {
	print "PASS $name (unexpectedly)\n";
	$nxpassed++;
    } else {
	print "pass $name\n";
	$npassed++;
    }
    print $why if $verbose;
    if ($test{'info-post'} ne '') {
	print "info ^[$test{'info-post'}]\n";
    }
    return 0;
}

sub
category_check
{
    local(*test) = @_;
    local($c);

    return 0 if ($test{'need-ctty'} && defined $categories{'regress:no-ctty'});
    return 1 if (!defined $test{'category'});
    local($ok) = 0;
    foreach $c (split(',', $test{'category'})) {
	$c =~ s/\s+//;
	if ($c =~ /^!/) {
	    $c = $';
	    return 0 if (defined $categories{$c});
	    $ok = 1;
	} else {
	    $ok = 1 if (defined $categories{$c});
	}
    }
    return $ok;
}

sub
scrub_dir
{
    local($dir) = @_;
    local(@todo) = ();
    local($file);

    if (!opendir(DIR, $dir)) {
	print STDERR "$prog: couldn't open directory $dir - $!\n";
	return undef;
    }
    while (defined ($file = readdir(DIR))) {
	push(@todo, $file) if $file ne '.' && $file ne '..';
    }
    closedir(DIR);
    foreach $file (@todo) {
	$file = "$dir/$file";
	if (-d $file) {
	    return undef if !&scrub_dir($file);
	    if (!rmdir($file)) {
		print STDERR "$prog: couldn't rmdir $file - $!\n";
		return undef;
	    }
	} else {
	    if (!unlink($file)) {
		print STDERR "$prog: couldn't unlink $file - $!\n";
		return undef;
	    }
	}
    }
    return 1;
}

sub
write_file
{
    local($file, $str) = @_;

    if (!open(TEMP, "> $file")) {
	print STDERR "$prog: can't open $file - $!\n";
	return undef;
    }
    binmode(TEMP);
    print TEMP $str;
    if (!close(TEMP)) {
	print STDERR "$prog: error writing $file - $!\n";
	return undef;
    }
    return 1;
}

sub
check_output
{
    local($name, $file, $what, $expect, $expect_pat) = @_;
    local($got) = '';
    local($why) = '';
    local($ret);

    if (!open(TEMP, "< $file")) {
	print STDERR "$prog:$name($what): couldn't open $file after running program - $!\n";
	return undef;
    }
    binmode(TEMP);
    while (<TEMP>) {
	$got .= $_;
    }
    close(TEMP);
    return compare_output($name, $what, $expect, $expect_pat, $got);
}

sub
compare_output
{
    local($name, $what, $expect, $expect_pat, $got) = @_;
    local($why) = '';

    if (defined $expect_pat) {
	$_ = $got;
	$ret = eval "$expect_pat";
	if ($@ ne '') {
	    print STDERR "$prog:$name($what): error evaluating $what pattern: $expect_pat - $@\n";
	    return undef;
	}
	if (!$ret) {
	    $why = "\tunexpected $what - wanted pattern";
	    $why .= &wrap_lines($expect_pat);
	    $why .= "\tgot";
	    $why .= &wrap_lines($got);
	}
    } else {
	$expect = '' if !defined $expect;
	if ($got ne $expect) {
	    $why .= "\tunexpected $what - " . &first_diff($expect, $got) . "\n";
	    $why .= "\twanted";
	    $why .= &wrap_lines($expect);
	    $why .= "\tgot";
	    $why .= &wrap_lines($got);
	}
    }
    return $why;
}

sub
wrap_lines
{
    local($str, $empty) = @_;
    local($nonl) = substr($str, -1, 1) ne "\n";

    return (defined $empty ? $empty : " nothing\n") if $str eq '';
    substr($str, 0, 0) = ":\n";
    $str =~ s/\n/\n\t\t/g;
    if ($nonl) {
	$str .= "\n\t[incomplete last line]\n";
    } else {
	chop($str);
	chop($str);
    }
    return $str;
}

sub
first_diff
{
    local($exp, $got) = @_;
    local($lineno, $char) = (1, 1);
    local($i, $exp_len, $got_len);
    local($ce, $cg);

    $exp_len = length($exp);
    $got_len = length($got);
    if ($exp_len != $got_len) {
	if ($exp_len < $got_len) {
	    if (substr($got, 0, $exp_len) eq $exp) {
		return "got too much output";
	    }
	} elsif (substr($exp, 0, $got_len) eq $got) {
	    return "got too little output";
	}
    }
    for ($i = 0; $i < $exp_len; $i++) {
	$ce = substr($exp, $i, 1);
	$cg = substr($got, $i, 1);
	last if $ce ne $cg;
	$char++;
	if ($ce eq "\n") {
	    $lineno++;
	    $char = 1;
	}
    }
    return "first difference: line $lineno, char $char (wanted " .
	&format_char($ce) . ", got " . &format_char($cg);
}

sub
format_char
{
    local($ch, $s, $q);

    $ch = ord($_[0]);
    $q = "'";

    if ($is_ebcdic) {
	if ($ch == 0x15) {
		return $q . '\n' . $q;
	} elsif ($ch == 0x16) {
		return $q . '\b' . $q;
	} elsif ($ch == 0x05) {
		return $q . '\t' . $q;
	} elsif ($ch < 64 || $ch == 255) {
		return sprintf("X'%02X'", $ch);
	}
	return sprintf("'%c' (X'%02X')", $ch, $ch);
    }

    $s = sprintf("0x%02X (", $ch);
    if ($ch == 10) {
	return $s . $q . '\n' . $q . ')';
    } elsif ($ch == 13) {
	return $s . $q . '\r' . $q . ')';
    } elsif ($ch == 8) {
	return $s . $q . '\b' . $q . ')';
    } elsif ($ch == 9) {
	return $s . $q . '\t' . $q . ')';
    } elsif ($ch > 127) {
	$ch -= 128;
	$s .= "M-";
    }
    if ($ch < 32) {
	return sprintf("%s^%c)", $s, $ch + ord('@'));
    } elsif ($ch == 127) {
	return $s . "^?)";
    }
    return sprintf("%s'%c')", $s, $ch);
}

sub
eval_exit
{
    local($name, $status, $expect) = @_;
    local($expr);
    local($w, $e, $s) = ($status, ($status >> 8) & 0xff, $status & 0x7f);

    $e = -1000 if $status & 0xff;
    $s = -1000 if $s == 0x7f;
    if (!defined $expect) {
	$expr = '$w == 0';
    } elsif ($expect =~ /^(|-)\d+$/) {
	$expr = "\$e == $expect";
    } else {
	$expr = $expect;
	$expr =~ s/\b([wse])\b/\$$1/g;
	$expr =~ s/\b(SIG[A-Z][A-Z0-9]*)\b/&$1/g;
    }
    $w = eval $expr;
    if ($@ ne '') {
	print STDERR "$prog:$test{':long-name'}: bad expected-exit expression: $expect ($@)\n";
	return undef;
    }
    return $w;
}

sub
read_test
{
    local($file, $in, *test) = @_;
    local($field, $val, $flags, $do_chop, $need_redo, $start_lineno);
    local(%cnt, $sfield);

    %test = ();
    %cnt = ();
    while (<$in>) {
	chop;
	next if /^\s*$/;
	next if /^ *#/;
	last if /^\s*---\s*$/;
	$start_lineno = $. if !defined $start_lineno;
	if (!/^([-\w]+):\s*(|\S|\S.*\S)\s*$/) {
	    print STDERR "$prog:$file:$.: unrecognised line \"$_\"\n";
	    return undef;
	}
	($field, $val) = ($1, $2);
	$sfield = $field;
	$flags = $test_fields{$field};
	if (!defined $flags) {
	    print STDERR "$prog:$file:$.: unrecognised field \"$field\"\n";
	    return undef;
	}
	if ($flags =~ /s/) {
	    local($cnt) = $cnt{$field}++;
	    $test{$field} = $cnt{$field};
	    $cnt = 0 if $cnt eq '';
	    $sfield .= ":$cnt";
	} elsif (defined $test{$field}) {
	    print STDERR "$prog:$file:$.: multiple \"$field\" fields\n";
	    return undef;
	}
	$do_chop = $flags !~ /m/;
	$need_redo = 0;
	if ($val eq '' || $val eq '!' || $flags =~ /p/) {
	    if ($flags =~ /[Mm]/) {
		if ($flags =~ /p/) {
		    if ($val =~ /^!/) {
			$do_chop = 1;
			$val = $';
		    } else {
			$do_chop = 0;
		    }
		    if ($val eq '') {
			print STDERR
		"$prog:$file:$.: no parameters given for field \"$field\"\n";
			return undef;
		    }
		} else {
		    if ($val eq '!') {
			$do_chop = 1;
		    }
		    $val = '';
		}
		while (<$in>) {
		    last if !/^\t/;
		    $val .= $';
		}
		chop $val if $do_chop;
		$do_chop = 1;
		$need_redo = 1;

		# Syntax check on fields that can several instances
		# (can give useful line numbers this way)

		if ($field eq 'file-setup') {
		    local($type, $perm, $rest, $c, $len, $name);

		    # format is: type perm "name"
		    if ($val !~ /^[ \t]*(\S+)[ \t]+(\S+)[ \t]+([^ \t].*)/) {
			print STDERR
		    "$prog:$file:$.: bad parameter line for file-setup field\n";
			return undef;
		    }
		    ($type, $perm, $rest) = ($1, $2, $3);
		    if ($type !~ /^(file|dir|symlink)$/) {
			print STDERR
		    "$prog:$file:$.: bad file type for file-setup: $type\n";
			return undef;
		    }
		    if ($perm !~ /^\d+$/) {
			print STDERR
		    "$prog:$file:$.: bad permissions for file-setup: $type\n";
			return undef;
		    }
		    $c = substr($rest, 0, 1);
		    if (($len = index($rest, $c, 1) - 1) <= 0) {
			print STDERR
    "$prog:$file:$.: missing end quote for file name in file-setup: $rest\n";
			return undef;
		    }
		    $name = substr($rest, 1, $len);
		    if ($name =~ /^\// || $name =~ /(^|\/)\.\.(\/|$)/) {
			# Note: this is not a security thing - just a sanity
			# check - a test can still use symlinks to get at files
			# outside the test directory.
			print STDERR
"$prog:$file:$.: file name in file-setup is absolute or contains ..: $name\n";
			return undef;
		    }
		}
		if ($field eq 'file-result') {
		    local($type, $perm, $uid, $gid, $matchType,
			  $rest, $c, $len, $name);

		    # format is: type perm uid gid matchType "name"
		    if ($val !~ /^\s*(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S.*)/) {
			print STDERR
		    "$prog:$file:$.: bad parameter line for file-result field\n";
			return undef;
		    }
		    ($type, $perm, $uid, $gid, $matchType, $rest)
			= ($1, $2, $3, $4, $5, $6);
		    if ($type !~ /^(file|dir|symlink)$/) {
			print STDERR
		    "$prog:$file:$.: bad file type for file-result: $type\n";
			return undef;
		    }
		    if ($perm !~ /^\d+$/ && $perm ne '*') {
			print STDERR
		    "$prog:$file:$.: bad permissions for file-result: $perm\n";
			return undef;
		    }
		    if ($uid !~ /^\d+$/ && $uid ne '*') {
			print STDERR
		    "$prog:$file:$.: bad user-id for file-result: $uid\n";
			return undef;
		    }
		    if ($gid !~ /^\d+$/ && $gid ne '*') {
			print STDERR
		    "$prog:$file:$.: bad group-id for file-result: $gid\n";
			return undef;
		    }
		    if ($matchType !~ /^(exact|pattern)$/) {
			print STDERR
		"$prog:$file:$.: bad match type for file-result: $matchType\n";
			return undef;
		    }
		    $c = substr($rest, 0, 1);
		    if (($len = index($rest, $c, 1) - 1) <= 0) {
			print STDERR
    "$prog:$file:$.: missing end quote for file name in file-result: $rest\n";
			return undef;
		    }
		    $name = substr($rest, 1, $len);
		    if ($name =~ /^\// || $name =~ /(^|\/)\.\.(\/|$)/) {
			# Note: this is not a security thing - just a sanity
			# check - a test can still use symlinks to get at files
			# outside the test directory.
			print STDERR
"$prog:$file:$.: file name in file-result is absolute or contains ..: $name\n";
			return undef;
		    }
		}
	    } elsif ($val eq '') {
		print STDERR
		    "$prog:$file:$.: no value given for field \"$field\"\n";
		return undef;
	    }
	}
	$val .= "\n" if !$do_chop;
	$test{$sfield} = $val;
	redo if $need_redo;
    }
    if ($_ eq '') {
	if (%test) {
	    print STDERR
	      "$prog:$file:$start_lineno: end-of-file while reading test\n";
	    return undef;
	}
	return 0;
    }

    while (($field, $val) = each %test_fields) {
	if ($val =~ /r/ && !defined $test{$field}) {
	    print STDERR
	      "$prog:$file:$start_lineno: required field \"$field\" missing\n";
	    return undef;
	}
    }

    $test{':full-name'} = substr($file, $file_prefix_skip) . ":$test{'name'}";
    $test{':long-name'} = "$file:$start_lineno:$test{'name'}";

    # Syntax check on specific fields
    if (defined $test{'expected-fail'}) {
	if ($test{'expected-fail'} !~ /^(yes|no)$/) {
	    print STDERR
	      "$prog:$test{':long-name'}: bad value for expected-fail field\n";
	    return undef;
	}
	$test{'expected-fail'} = $1 eq 'yes';
    } else {
	$test{'expected-fail'} = 0;
    }
    if (defined $test{'need-ctty'}) {
	if ($test{'need-ctty'} !~ /^(yes|no)$/) {
	    print STDERR
	      "$prog:$test{':long-name'}: bad value for need-ctty field\n";
	    return undef;
	}
	$test{'need-ctty'} = $1 eq 'yes';
    } else {
	$test{'need-ctty'} = 0;
    }
    if (defined $test{'need-pass'}) {
	if ($test{'need-pass'} !~ /^(yes|no)$/) {
	    print STDERR
	      "$prog:$test{':long-name'}: bad value for need-pass field\n";
	    return undef;
	}
	$test{'need-pass'} = $1 eq 'yes';
    } else {
	$test{'need-pass'} = 1;
    }
    if (defined $test{'arguments'}) {
	local($firstc) = substr($test{'arguments'}, 0, 1);

	if (substr($test{'arguments'}, -1, 1) ne $firstc) {
	    print STDERR "$prog:$test{':long-name'}: arguments field doesn't start and end with the same character\n";
	    return undef;
	}
    }
    if (defined $test{'env-setup'}) {
	local($firstc) = substr($test{'env-setup'}, 0, 1);

	if (substr($test{'env-setup'}, -1, 1) ne $firstc) {
	    print STDERR "$prog:$test{':long-name'}: env-setup field doesn't start and end with the same character\n";
	    return undef;
	}

	$test{'env-setup'} =~ s/\@utflocale\@/$utflocale/g;
    }
    if (defined $test{'expected-exit'}) {
	local($val) = $test{'expected-exit'};

	if ($val =~ /^(|-)\d+$/) {
	    if ($val < 0 || $val > 255) {
		print STDERR "$prog:$test{':long-name'}: expected-exit value $val not in 0..255\n";
		return undef;
	    }
	} elsif ($val !~ /^([\s\d<>+=*%\/&|!()-]|\b[wse]\b|\bSIG[A-Z][A-Z0-9]*\b)+$/) {
	    print STDERR "$prog:$test{':long-name'}: bad expected-exit expression: $val\n";
	    return undef;
	}
    } else {
	$test{'expected-exit'} = 0;
    }
    if (defined $test{'expected-stdout'}
	&& defined $test{'expected-stdout-pattern'})
    {
	print STDERR "$prog:$test{':long-name'}: can't use both expected-stdout and expected-stdout-pattern\n";
	return undef;
    }
    if (defined $test{'expected-stderr'}
	&& defined $test{'expected-stderr-pattern'})
    {
	print STDERR "$prog:$test{':long-name'}: can't use both expected-stderr and expected-stderr-pattern\n";
	return undef;
    }
    if (defined $test{'time-limit'}) {
	if ($test{'time-limit'} !~ /^\d+$/ || $test{'time-limit'} == 0) {
	    print STDERR
	      "$prog:$test{':long-name'}: bad value for time-limit field\n";
	    return undef;
	}
    } elsif (defined $default_time_limit) {
	$test{'time-limit'} = $default_time_limit;
    }

    if (defined $known_tests{$test{'name'}}) {
	print STDERR "$prog:$test{':long-name'}: warning: duplicate test name ${test{'name'}}\n";
    }
    $known_tests{$test{'name'}} = 1;

    return 1;
}

sub
tty_msg
{
    local($msg) = @_;

    open(TTY, "> /dev/tty") || return 0;
    print TTY $msg;
    close(TTY);
    return 1;
}

sub
never_called_funcs
{
	return 0;
	&tty_msg("hi\n");
	&never_called_funcs();
	&catch_sigalrm();
	$old_env{'foo'} = 'bar';
	$internal_test_fields{'foo'} = 'bar';
}

sub
check_file_result
{
    local(*test) = @_;

    return '' if (!defined $test{'file-result'});

    local($why) = '';
    local($i);
    local($type, $perm, $uid, $gid, $rest, $c, $len, $name);
    local(@stbuf);

    for ($i = 0; $i < $test{'file-result'}; $i++) {
	$val = $test{"file-result:$i"};

	# format is: type perm "name"
	($type, $perm, $uid, $gid, $matchType, $rest) =
	    split(' ', $val, 6);
	$c = substr($rest, 0, 1);
	$len = index($rest, $c, 1) - 1;
	$name = substr($rest, 1, $len);
	$rest = substr($rest, 2 + $len);
	$perm = oct($perm) if $perm =~ /^\d+$/;

	@stbuf = lstat($name);
	if (!@stbuf) {
	    $why .= "\texpected $type \"$name\" not created\n";
	    next;
	}
	if ($perm ne '*' && ($stbuf[2] & 07777) != $perm) {
	    $why .= "\t$type \"$name\" has unexpected permissions\n";
	    $why .= sprintf("\t\texpected 0%o, found 0%o\n",
		    $perm, $stbuf[2] & 07777);
	}
	if ($uid ne '*' && $stbuf[4] != $uid) {
	    $why .= "\t$type \"$name\" has unexpected user-id\n";
	    $why .= sprintf("\t\texpected %d, found %d\n",
		    $uid, $stbuf[4]);
	}
	if ($gid ne '*' && $stbuf[5] != $gid) {
	    $why .= "\t$type \"$name\" has unexpected group-id\n";
	    $why .= sprintf("\t\texpected %d, found %d\n",
		    $gid, $stbuf[5]);
	}

	if ($type eq 'file') {
	    if (-l _ || ! -f _) {
		$why .= "\t$type \"$name\" is not a regular file\n";
	    } else {
		local $tmp = &check_output($test{'long-name'}, $name,
			    "$type contents in \"$name\"",
			    $matchType eq 'exact' ? $rest : undef
			    $matchType eq 'pattern' ? $rest : undef);
		return undef if (!defined $tmp);
		$why .= $tmp;
	    }
	} elsif ($type eq 'dir') {
	    if ($rest !~ /^\s*$/) {
		print STDERR "$prog:$test{':long-name'}: file-result test for directory $name should not have content specified\n";
		return undef;
	    }
	    if (-l _ || ! -d _) {
		$why .= "\t$type \"$name\" is not a directory\n";
	    }
	} elsif ($type eq 'symlink') {
	    if (!-l _) {
		$why .= "\t$type \"$name\" is not a symlink\n";
	    } else {
		local $content = readlink($name);
		if (!defined $content) {
		    print STDERR "$prog:$test{':long-name'}: file-result test for $type $name failed - could not readlink - $!\n";
		    return undef;
		}
		local $tmp = &compare_output($test{'long-name'},
			    "$type contents in \"$name\"",
			    $matchType eq 'exact' ? $rest : undef
			    $matchType eq 'pattern' ? $rest : undef);
		return undef if (!defined $tmp);
		$why .= $tmp;
	    }
	}
    }

    return $why;
}

sub
HELP_MESSAGE
{
    print STDERR $Usage;
    exit 0;
}
