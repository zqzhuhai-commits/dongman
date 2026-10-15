import hashlib
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

EXPECTED_SHA = "40b07596c336c8aa682c0eb163b142664592ad83f2f609d23e5b33584efd713a"
EXPECTED_SIZE = 20221144
TAG = "v0.1.2-standalone"
SOURCE = "a1f9befdcc130c1f9bb50f33747a6af84067a0c3"
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
        "name": "动漫共和国 v0.1.2 · 播放源标签与新图标",
        "prerelease": True,
        "body": "播放源已分为独立标签，左右键或 L/R 切源，上下键选集；使用用户提供的 Logo。新增媒体预检和详细错误提示。独立 NSP，内置默认 OMOFUN 配置与 CA 证书，无需在 SD 卡保留 config.ini 或 cacert.pem。\n\n"
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
