import numpy as np, soundfile as sf, json, sys
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
D=sys.argv[1]; sel=sys.argv[2]; outp=sys.argv[3]; tmax=float(sys.argv[4]) if len(sys.argv)>4 else 0.6
A=[r for r in json.load(open(D+'/audit.json')) if eval(sel)]
n=len(A); cols=4; rows=(n+cols-1)//cols
fig,ax=plt.subplots(rows,cols,figsize=(cols*4,rows*2.4))
for i,r in enumerate(A):
    import glob; a,sr=sf.read((sorted(glob.glob(f"{D}/{r['file']}_r0.wav")) or [f"{D}/{r['file']}.wav"])[0]); m=a.mean(1)[:int(tmax*sr)]
    x=ax.flat[i]; x.specgram(m,NFFT=2048,Fs=sr,noverlap=1900,cmap='magma',vmin=-130,vmax=-20)
    x.set_ylim(20,6000 if 'Kick' in r['cat'] else 16000); x.set_yscale('log')
    t=np.arange(len(m))/sr; x2=x.twinx(); x2.plot(t,20*np.log10(np.abs(m)+1e-6),lw=0.4,c='c'); x2.set_ylim(-80,0); x2.set_yticks([])
    x.set_title(f"{r['name']}  L{r['L']:.1f}",fontsize=8); x.tick_params(labelsize=6)
for j in range(i+1,rows*cols): ax.flat[j].axis('off')
plt.tight_layout(); plt.savefig(outp,dpi=70)
