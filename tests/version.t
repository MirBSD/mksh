name: version-1
description:
	Check version of shell.
category: pdksh
stdin:
	echo $KSH_VERSION
expected-stdout-pattern:
	/PD KSH v5\.2\.14 MirOS R18 in (native )?KSH mode( as mksh)?/
---
