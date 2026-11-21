"""Export the actual PlatformIO flash inputs; this script never writes a device."""
import json
from pathlib import Path
Import("env")

def export_inputs(source, target, env):
    folder=Path(env.subst("$BUILD_DIR"))
    image=folder/"firmware.bin"
    if image.stat().st_size > 0x180000:
        raise RuntimeError("SmallTV SDC2 USB app exceeds its new 1.5 MiB OTA slot")
    inputs=[{"offset":int(str(offset),0),"path":env.subst(str(path))}
            for offset,path in env.get("FLASH_EXTRA_IMAGES",[])]
    inputs.append({"offset":0x10000,"path":str(image.resolve())})
    (folder/"sdc2-flash-inputs.json").write_text(json.dumps(inputs,indent=2))
    print("SDC2 USB flash inputs exported; not compatible with the stock OTA layout.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin",export_inputs)
