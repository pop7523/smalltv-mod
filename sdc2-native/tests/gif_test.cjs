const fs=require('node:fs'),assert=require('node:assert/strict');
const codec=require('/tmp/smalltv-gif-codec.cjs');
const dir='/tmp/smalltv-gifs';
for(const disposal of [1,2,3]){
  const source=fs.readFileSync(`${dir}/disposal-${disposal}.gif`);
  const converted=codec.convertGif(source.buffer.slice(source.byteOffset,source.byteOffset+source.byteLength));
  fs.writeFileSync(`${dir}/disposal-${disposal}.s8g`,converted);
}
const wrong=fs.readFileSync(`${dir}/wrong-size.gif`);
assert.throws(()=>codec.convertGif(wrong.buffer.slice(wrong.byteOffset,wrong.byteOffset+wrong.byteLength)),/80/);
assert.throws(()=>codec.encodeFrames([]));
const rgba=new Uint8ClampedArray(25600);for(let i=0;i<6400;i++){rgba[i*4]=(i*31)&255;rgba[i*4+1]=(i*79)&255;rgba[i*4+2]=(i*113)&255;rgba[i*4+3]=255}
assert.throws(()=>codec.encodeFrames(Array.from({length:300},()=>({rgba,duration:20}))),/384/);
console.log('PASS: real animated GIF conversion, dimension checks, frame and storage limits');
