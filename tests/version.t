name: version-1
description:
	Check version of shell.
category: pdksh
stdin:
	echo $KSH_VERSION
expected-stdout-pattern:
	/PD KSH v5\.2\.14 MirOS \$Revision: 1.15 $ in (native )?KSH mode( as mksh)?/
---
