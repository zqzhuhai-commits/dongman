import hashlib
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

EXPECTED_SHA = "1b4bc98829f390a38d939e4e154d2bbd189d81ec75eb3b973a2164475c496fcd"
EXPECTED_SIZE = 6634200
TAG = "v0.2.2-fontfix"
SOURCE = "677558b4fe0da5580a20c3979fad48964f4082a3"
repository = os.environ["GITHUB_REPOSITORY"]
assert repository == "zqzhuhai-commits/dongman"
data = b"".join(Path(f"release-upload/dongman.nsp.part{i:02}").read_bytes()
                for i in range(1, 4))
assert len(data) == EXPECTED_SIZE
assert hashlib.sha256(data).hexdigest() == EXPECTED_SHA
assert data[:4] == b"PFS0"


def request(url, payload=None, mime="application/json"):
    body = json.dumps(payload).encode() if isinstance(payload, dict) else payload
    headers = {"Authorization": "Bearer " + os.environ["GH_TOKEN"],
               "Accept": "application/vnd.github+json",
               "X-GitHub-Api-Version": "2022-11-28",
               "Content-Type": mime}
    req = urllib.request.Request(url, data=body, headers=headers)
    with urllib.request.urlopen(req, timeout=180) as response:
        return json.load(response)


base = "https://api.github.com/repos/" + repository
try:
    release = request(base + "/releases/tags/" + TAG)
except urllib.error.HTTPError as error:
    if error.code != 404:
        raise
    release = request(base + "/releases", {
        "tag_name": TAG, "target_commitish": SOURCE,
        "name": "动漫共和国 v0.2.2 · 修复实机按键提示乱码",
        "prerelease": True,
        "body": "修复 Switch 实机上横竖切换前减号显示乱码的问题：将字体可能缺失的 Unicode 减号替换为标准 ASCII `[-] 横竖切换`。\n\n"
                "继续使用 − 按键切换横竖屏，方向保存、翻页与阅读进度功能保持原样。独立 NSP 内置证书和配置，无需额外 config.ini 或 cacert.pem。\n\n"
                "Switch 原生构建、桌面构建、旋转/翻页/续读流程测试与 NSP 内容校验通过。\n\n"
                "源码提交：" + SOURCE + "。\n\nSHA-256: `" + EXPECTED_SHA + "`",
    })
upload = release["upload_url"].split("{")[0]
existing = {asset["name"]: asset for asset in release.get("assets", [])}
for name, body, mime in [
    ("dongman.nsp", data, "application/octet-stream"),
    ("SHA256SUMS.txt", (EXPECTED_SHA + "  dongman.nsp\n").encode(), "text/plain"),
]:
    if name in existing:
        assert existing[name]["size"] == len(body)
        if name == "dongman.nsp":
            assert existing[name].get("digest") == "sha256:" + EXPECTED_SHA
        continue
    asset = request(upload + "?" + urllib.parse.urlencode({"name": name}), body, mime)
    assert asset["size"] == len(body)
    print(asset["browser_download_url"])
print(release["html_url"])
