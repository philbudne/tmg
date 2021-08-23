"""
Ptthon script to postprocess tmgl7a output
to make code acceptabe to as7 and PDP-7 tmgl builtins
only handles cases generated by tmgl7a running over itself!
"""

import sys
import re

STR_RE = re.compile("<[^>]*>")

end_next = ''
for line in sys.stdin.readlines():
    if end_next:
        if not line.startswith('x '):
            line = end_next + line
        end_next = ''

    # PDP-11 uses bare address for both builtins and TMG code
    # (determines which by address range)
    # need to have a rule that recognizes all builtins???

    line = line.replace("parse", "rf parsedo")
    if 'x rf parsedo' in line:
        end_next = 'x '
        line = line.replace('x rf parsedo', 'rf parsedo')
    line = line.replace("smark", "rf mark")
    line = line.replace("ignore", "rf ign11")
    line = line.replace("octal", "rf octal")

    # split up string lits
    # wondering if lit strings were dumped out by symoct (in octal)?!
    def fixstr(m):
        s = m.string[m.start()+1:m.end()-1]
        out = []
        while len(s) > 1:
            out.append("<%s>" % s [0:2])
            s = s[2:]
        if len(s) == 1:
            out.append("<%s 0777" % s)
        return '; '.join(out)
    line = STR_RE.sub(fixstr, line)
    sys.stdout.write(line)
