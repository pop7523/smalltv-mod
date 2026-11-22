"""Generate real GIF inputs and independent Pillow compositing references."""
from pathlib import Path
from PIL import Image,ImageDraw
out=Path('/tmp/smalltv-gifs');out.mkdir(exist_ok=True)
for disposal in (1,2,3):
    frames=[]
    for i in range(4):
        im=Image.new('P',(80,80),0);im.putpalette([0,0,0,255,0,0,0,255,0,0,0,255]+[0,0,0]*252)
        ImageDraw.Draw(im).rectangle((i*12+1,8,i*12+20,40),fill=i%3+1)
        frames.append(im)
    path=out/f'disposal-{disposal}.gif'
    frames[0].save(path,save_all=True,append_images=frames[1:],duration=[40,100,60,80],loop=0,transparency=0,disposal=disposal,optimize=False)
    ref=bytearray()
    with Image.open(path) as gif:
        for i in range(gif.n_frames):gif.seek(i);ref+=gif.convert('RGBA').tobytes()
    path.with_suffix('.rgba').write_bytes(ref)
Image.new('RGB',(79,80),'red').save(out/'wrong-size.gif')
print('Generated disposal 1/2/3 animated GIFs and independent RGBA references')
