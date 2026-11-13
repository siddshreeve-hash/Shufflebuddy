#!/bin/sh
# re-run after changing any sound: audit -> trims -> rebuild, twice (the second pass mops up), then verify
set -e
cd "$(dirname "$0")/../build"
for pass in 1 2; do
  ninja Audit > /dev/null
  rm -rf /tmp/sb_aud && ./Audit_artefacts/Release/Audit /tmp/sb_aud > /dev/null
  python3 ../tools/level_audit.py /tmp/sb_aud
done
ninja Audit > /dev/null
rm -rf /tmp/sb_audv && ./Audit_artefacts/Release/Audit /tmp/sb_audv verify > /dev/null
python3 ../tools/loudness.py /tmp/sb_audv all > /tmp/sb_audv/report.txt
python3 - <<'PY'
import re, statistics as st
k=[];o=[]
for l in open('/tmp/sb_audv/report.txt'):
    v=float(re.search(r'L\s+(-?[\d.]+)',l).group(1)); (k if (l.startswith('kick') or 'Kicks' in l) else o).append(v)
print('VERIFY kicks %.1f..%.1f | sounds median %.1f, %d of %d within 1 dB of -22' % (min(k), max(k), st.median(o), sum(abs(x+22)<=1 for x in o), len(o)))
PY
