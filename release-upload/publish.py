import hashlib
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

EXPECTED_SHA = "fa944ca59a1936f81d67ac120e6f3fc135bf832e75d9673ded6316205a64475a"
EXPECTED_SIZE = 20096728
TAG = "v0.1.1-standalone"
SOURCE = "4836613ce8c49718eab96c095189886636096f85"
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
        "name": "动漫共和国 v0.1.1 · 内置配置与证书",
        "prerelease": True,
        "body": "独立 NSP，内置默认 OMOFUN 配置与 CA 证书，无需在 SD 卡保留 config.ini 或 cacert.pem。\n\n"
                "Switch 原生构建、桌面测试和安装包结构校验通过；实机安装、启动及真实站点视频仍待验证。"
                "需使用支持 Homebrew 的 Switch 环境。源码对应提交 " + SOURCE + "。\n\n"
                "SHA-256: `" + EXPECTED_SHA + "`",
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
