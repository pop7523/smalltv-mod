"""Create small declarative clock/weather packages for SmallTV's own engine.

No Stock Clock code or data formats are used. Validate with the actual engine
before the filesystem image is built. Weather endpoint defaults to Seoul.
"""
import json
from pathlib import Path
import struct
from theme_native import validate_package

ROOT=Path(__file__).resolve().parents[1]
def manifest(layout):
    compact=layout=="minimal"
    accent="#65dbce"
    def text(name,value,y,size,color):
        return dict(id=name,type="text",x=120,y=y,anchor="center",value=value,size=size,color=color)
    layers=[text("date","{WD} {DD} {MON}",26,16,"#adb8cb"),
            text("time","{HH}:{MM}",92 if not compact else 108,56,"#f4f7ff"),
            text("seconds","{SS}",140 if not compact else 155,16,accent)]
    if layout=="card":
        layers.append(dict(id="card",type="shape",shape="rectangle",x=8,y=163,width=224,height=68,cornerRadius=10,fill="#152c33"))
    layers.extend([text("temperature","{wx.temp} C",187,24,accent),
                   text("humidity","Humidity {wx.hum}%",218,12,"#adb8cb")])
    return dict(spec=2,theme=dict(id="sdc2-"+layout,name="SDC2 "+layout.title(),author="smalltv-mod SDC2 port",version="1.0.0"),
        display=dict(width=240,height=240,background="#080d18"),
        data=[dict(id="wx",url="https://api.open-meteo.com/v1/forecast?latitude=37.5665&longitude=126.9780&current=temperature_2m,relative_humidity_2m",interval=600,
                   fields=[dict(id="temp",path="current.temperature_2m",decimals=0),dict(id="hum",path="current.relative_humidity_2m",decimals=0)])],layers=layers)

def main():
    dest=ROOT/"data/themes";dest.mkdir(parents=True,exist_ok=True)
    for layout in ("classic","card","minimal"):
        doc=manifest(layout)
        source=ROOT/"sdc2-themes"/layout;source.mkdir(parents=True,exist_ok=True)
        (source/"theme.json").write_text(json.dumps(doc,indent=2),encoding="utf-8")
        payload=json.dumps(doc,separators=(",",":"),ensure_ascii=True).encode()
        name=b"theme.json"
        package=b"STH1"+struct.pack("<HHHI",1,0,len(name),len(payload))+name+payload
        target=dest/(doc["theme"]["id"]+".stheme");target.write_bytes(package)
        print(target,validate_package(target))
    (ROOT/"data/config.json").write_text(json.dumps({"mode":"theme","themeId":"sdc2-classic","clock":{"tz":"Asia/Seoul","tzPosix":"KST-9","lat":37.5665,"lon":126.9780}}))

if __name__=="__main__":main()
