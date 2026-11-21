"""Host-only checks. These never compile firmware or access the device."""
import configparser
import json
from pathlib import Path
import runpy
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    passed = []
    config = configparser.ConfigParser(interpolation=None, strict=False)
    config.read([ROOT / "platformio.ini", ROOT / "platformio.sdc2.ini"], encoding="utf-8")
    profile = config["env:smalltv_sdc2"]
    rows = []
    for line in (ROOT / profile["board_build.partitions"]).read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        name, kind, sub, offset, size, *_ = [x.strip() for x in line.split(",")]
        rows.append((name, int(offset, 0), int(size, 0)))
    for previous, current in zip(rows, rows[1:]):
        assert previous[1] + previous[2] == current[1], (previous, current)
    assert rows[-1][1] + rows[-1][2] == 0x400000
    assert [r for r in rows if r[0].startswith("app")] == [
        ("app0", 0x10000, 0xC0000), ("app1", 0xD0000, 0xC0000)
    ]
    passed.append("reported stock partition bounds and non-overlap")
    assert "FW_NO_GITHUB_UPDATE" in profile["build_flags"]
    assert "SMALLTV_WIREGUARD=1" not in profile["build_flags"]
    assert "CONFIG_XTAL_FREQ_26=y" in profile["custom_sdkconfig"]
    assert profile["upload_command"] == "python tools/refuse_sdc2_upload.py"
    passed.append("variant flags, requested crystal, upload blocker")

    # Exercise the actual post-build guard, including both sides of the limit.
    # A tiny SCons env adapter is used because PlatformIO is not installed.
    class BuildEnv:
        def __init__(self, folder):
            self.folder = folder
            self.actions = []

        def subst(self, value):
            assert value == "$BUILD_DIR"
            return str(self.folder)

        def AddPostAction(self, target, action):
            self.actions.append((target, action))

    with tempfile.TemporaryDirectory(prefix="sdc2-check-") as folder:
        build_env = BuildEnv(Path(folder))
        namespace = runpy.run_path(str(ROOT / "tools/sdc2_size_guard.py"), init_globals={
            "env": build_env, "Import": lambda name: None
        })
        assert len(build_env.actions) == 1
        action = namespace["check_size"]
        for size, should_pass in [(0xC0000 - 1, True), (0xC0000, True),
                                  (0xC0000 + 1, False), (1493184, False)]:
            image = Path(folder) / "firmware.bin"
            with image.open("wb") as f:
                f.truncate(size)
            rejected = False
            try:
                action([], [], build_env)
            except RuntimeError:
                rejected = True
            assert rejected != should_pass, (size, should_pass)
    passed.append("actual size gate: below limit, exact limit, one byte over, upstream size")
    try:
        runpy.run_path(str(ROOT / "tools/refuse_sdc2_upload.py"))
    except SystemExit as exc:
        assert "UPLOAD BLOCKED" in str(exc)
    else:
        raise AssertionError("upload blocker did not stop")
    passed.append("upload command refuses device writes")
    result = {
        "host_checks": "PASS", "checks": passed,
        "firmware_compiled": False, "hardware_tested": False,
        "installable": False, "device_written": False,
        "upstream_c2_bytes": 1493184, "stock_slot_bytes": 786432,
        "required_reduction_bytes": 706752,
        "note": "Host checks validate guard behavior, not a firmware image."
    }
    print(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    main()
