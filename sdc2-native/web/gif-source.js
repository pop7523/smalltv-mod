import {parseGIF, decompressFrames} from 'gifuct-js';
export const MAX_BYTES=384*1024,MAX_FRAMES=300;
function color(p,i){return ((p[i]&248)<<8)|((p[i+1]&252)<<3)|(p[i+2]>>3)}
function encodedFrame(rgba){
  const pixels=new Uint32Array(6400);
  for(let i=0;i<6400;i++)pixels[i]=rgba[i*4+3]>=128?(color(rgba,i*4)|(255<<16)):0;
  const bytes=[];
  const push=p=>bytes.push(p&255,(p>>8)&255,p>>>16);
  let i=0;
  while(i<6400){
    let repeat=1;while(repeat<128&&i+repeat<6400&&pixels[i+repeat]===pixels[i])repeat++;
    if(repeat>=3){bytes.push(128+repeat-1);push(pixels[i]);i+=repeat;continue}
    const start=i;i+=repeat;
    while(i<6400&&i-start<128){
      repeat=1;while(repeat<3&&i+repeat<6400&&pixels[i+repeat]===pixels[i])repeat++;
      if(repeat>=3)break;i+=Math.min(repeat,128-(i-start));
    }
    bytes.push(i-start-1);for(let n=start;n<i;n++)push(pixels[n]);
  }
  return new Uint8Array(bytes);
}
export function composeGif(buffer){
  if(buffer.byteLength>2*1024*1024)throw Error('원본 GIF는 2MiB 이하로 선택하세요.');
  const gif=parseGIF(buffer);
  if(gif.lsd.width!==80||gif.lsd.height!==80)throw Error('정확히 80×80인 GIF만 사용할 수 있습니다.');
  const images=gif.frames.filter(f=>f.image);
  if(!images.length||images.length>MAX_FRAMES)throw Error('GIF는 1~300프레임이어야 합니다.');
  for(const f of images){const d=f.image.descriptor;if(!d.width||!d.height||d.left+d.width>80||d.top+d.height>80)throw Error('GIF 프레임 범위가 잘못됐습니다.')}
  const decoded=decompressFrames(gif,true),canvas=new Uint8ClampedArray(6400*4),frames=[];
  const bg=gif.gct?.[gif.lsd.backgroundColorIndex]||[0,0,0];
  function clear(rect,transparent){for(let y=rect.top;y<rect.top+rect.height;y++)for(let x=rect.left;x<rect.left+rect.width;x++){let p=(y*80+x)*4;canvas[p]=transparent?0:bg[0];canvas[p+1]=transparent?0:bg[1];canvas[p+2]=transparent?0:bg[2];canvas[p+3]=transparent?0:255}}
  if(decoded[0].transparentIndex===undefined)clear({left:0,top:0,width:80,height:80},false);
  let previous=null,restore=null;
  for(const frame of decoded){
    if(previous?.disposalType===2)clear(previous.dims,previous.transparentIndex!==undefined);
    else if(previous?.disposalType===3&&restore)canvas.set(restore);
    restore=frame.disposalType===3?canvas.slice():null;
    const d=frame.dims;
    if(frame.patch.length!==d.width*d.height*4)throw Error('GIF 프레임 데이터가 잘못됐습니다.');
    for(let y=0;y<d.height;y++)for(let x=0;x<d.width;x++){
      const a=(y*d.width+x)*4;if(!frame.patch[a+3])continue;
      const b=((d.top+y)*80+d.left+x)*4;canvas.set(frame.patch.subarray(a,a+4),b);
    }
    frames.push({rgba:canvas.slice(),duration:Math.max(20,Math.min(10000,frame.delay||100))});previous=frame;
  }
  return frames;
}
export function encodeFrames(frames){
  if(!frames.length||frames.length>MAX_FRAMES)throw Error('프레임 수를 줄여주세요.');
  const head=new Uint8Array(12),h=new DataView(head.buffer);head.set([83,56,71,49]);h.setUint16(4,80,true);h.setUint16(6,80,true);h.setUint16(8,frames.length,true);
  const parts=[head];let total=12;
  for(const f of frames){
    if(f.rgba.length!==25600)throw Error('80×80 프레임이 아닙니다.');
    const data=encodedFrame(f.rgba),header=new Uint8Array(6),view=new DataView(header.buffer);
    view.setUint16(0,Math.max(20,Math.min(10000,Math.round(f.duration)||100)),true);view.setUint32(2,data.length,true);
    total+=6+data.length;if(total>MAX_BYTES)throw Error('변환 후 384KiB를 초과합니다. 프레임 수나 색상을 줄여주세요.');parts.push(header,data);
  }
  const out=new Uint8Array(total);let at=0;for(const p of parts){out.set(p,at);at+=p.length}return out;
}
export function convertGif(buffer){return encodeFrames(composeGif(buffer))}
