import numpy as np, soundfile as sf, sys, csv
from scipy.signal import lfilter, butter, sosfilt
D=sys.argv[1] if len(sys.argv)>1 else '/tmp/aud'
def kw(x):
    x=lfilter([1.53512485958697,-2.69169618940638,1.19839281085285],[1,-1.69065929318241,0.73248077421585],x,axis=0)
    return lfilter([1,-2,1],[1,-1.99004745483398,0.99007225036621],x,axis=0)
def loud(a,sr=48000):
    k=kw(a); p=(k**2).sum(1); w=int(0.2*sr); h=int(0.01*sr)
    c=np.concatenate([[0],np.cumsum(p)]); best=max((c[i+w]-c[i])/w for i in range(0,max(1,len(p)-w),h))
    return -0.691+10*np.log10(best+1e-12)
def kickinfo(a,sr=48000):
    m=a.mean(1); env=np.abs(m); pk=env.max()
    # decay: last time above -30 dB (smoothed)
    e=np.convolve(env,np.ones(240)/240,'same'); idx=np.where(e>pk*10**(-30/20))[0]; dec=idx[-1]/sr if len(idx) else 0
    lp=sosfilt(butter(4,400,'low',fs=sr,output='sos'),m)
    zc=np.where((lp[:-1]<0)&(lp[1:]>=0))[0]; tr=[]
    for i in range(1,len(zc)):
        t=zc[i]/sr
        if t>0.4: break
        tr.append((t, sr/(zc[i]-zc[i-1])))
    tr=np.array(tr) if tr else np.zeros((1,2))
    fin=np.median(tr[(tr[:,0]>0.12)&(tr[:,0]<0.3),1]) if ((tr[:,0]>0.12)&(tr[:,0]<0.3)).any() else tr[-1,1]
    st=tr[0,1] if len(tr) else 0
    settle=next((t for t,f in tr if f<fin*1.12), 0)
    hp=sosfilt(butter(4,2000,'high',fs=sr,output='sos'),m)
    click=10*np.log10((hp[:int(0.012*sr)]**2).sum()/((m**2).sum()+1e-12)+1e-12)
    sub=sosfilt(butter(4,70,'low',fs=sr,output='sos'),m)
    subr=10*np.log10((sub**2).sum()/((m**2).sum()+1e-12)+1e-12)
    peak=20*np.log10(np.abs(a).max()+1e-12)
    return dict(start=st,final=fin,settle=settle*1000,decay=dec*1000,click=click,sub=subr,peak=peak)
rows=[l.split('\t') for l in open(D+'/index.tsv').read().strip().split('\n')]
out=[]
for f,cat,name in rows:
    import glob, os
    takes=sorted(glob.glob(f'{D}/{f}_r*.wav')) or [f'{D}/{f}.wav']
    Ls=[]; pks=[]
    for t in takes:
        a,sr=sf.read(t); Ls.append(loud(a)); pks.append(20*np.log10(np.abs(a).max()+1e-12))
    L=10*np.log10(np.mean([10**(x/10) for x in Ls]))
    a,sr=sf.read(takes[0])
    r=dict(file=f,cat=cat,name=name,L=L,peak=max(pks),spread=max(Ls)-min(Ls))
    if cat=='Kick' or cat=='Kicks': r.update(kickinfo(a))
    out.append(r)
import json; json.dump(out,open(D+'/audit.json','w'),indent=0)
mode=sys.argv[2] if len(sys.argv)>2 else 'all'
for r in out:
    if mode=='kicks' and not r['cat'].startswith('Kick'): continue
    s=f"{r['file']:12s} {r['cat'][:20]:20s} {r['name'][:22]:22s} L {r["L"]:6.1f} sp {r.get("spread",0):4.1f} pk {r['peak']:6.1f}"
    if 'final' in r: s+=f"  f {r['start']:5.0f}->{r['final']:5.1f}Hz settle {r['settle']:4.0f}ms decay {r['decay']:4.0f}ms click {r['click']:5.1f} sub {r['sub']:5.1f}"
    print(s)
